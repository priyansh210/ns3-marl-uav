import argparse
import json
import os

import libsumo
import logging
import osmBuild
import osmGet
import pyproj
import randomTrips
import traceExporter
from geopy import distance
from sumolib import checkBinary
import xml.etree.ElementTree as ET
from defiance import NS3_HOME

sumoBinary = checkBinary("sumo")
build_dir: str = NS3_HOME + "/contrib/defiance/scenarios"
config_dir: str = "config_files"
trace_dir: str = "traces"
logger = logging.getLogger(__name__)


def directory_exists(path):
    return os.path.exists(path)


def convert_coordinates(longitude, latitude, net_offset, utm_proj):
    x, y = utm_proj(longitude, latitude)
    # Apply net offset
    x += net_offset[0]
    y += net_offset[1]
    return x, y


def create_directory(path):
    if not directory_exists(path):
        os.makedirs(path)
        logger.debug(f"Directory '{path}' created successfully.")
        return True
    return False


def get_directory_name(center, dimensions):
    return f"scenario-{center[0]}-{center[1]}-{dimensions[0]}-{dimensions[1]}"


def distance_from_coordinates(lat1, lon1, lat2, lon2):
    return distance.distance((lat1, lon1), (lat2, lon2)).km


def distance_to_lat_offset(distance, center_lat, center_lon):
    eps = 0.05
    conversion_factor = eps / distance_from_coordinates(center_lat, center_lon, center_lat + eps, center_lon)
    return distance * conversion_factor


def distance_to_lon_offset(distance, center_lat, center_lon):
    eps = 0.05
    conversion_factor = eps / distance_from_coordinates(center_lat, center_lon, center_lat, center_lon + eps)
    return distance * conversion_factor


def bounding_box_from_center_and_dimensions(center_lat, center_lon, width, height):
    lat_offset = distance_to_lat_offset(height / 2, center_lat, center_lon)
    lon_offset = distance_to_lon_offset(width / 2, center_lat, center_lon)
    return f"{center_lon - lon_offset},{center_lat - lat_offset},{center_lon + lon_offset},{center_lat + lat_offset}"


def create_config_files(center, dimensions) -> str:
    scenario_dir = get_directory_name(center, dimensions)
    if directory_exists(f"{build_dir}/{scenario_dir}"):
        logger.info("Scenario directory already exists.")
        return scenario_dir
    create_directory(f"{build_dir}/{scenario_dir}")
    create_directory(f"{build_dir}/{scenario_dir}/{config_dir}")
    create_directory(f"{build_dir}/{scenario_dir}/{trace_dir}")
    bbox_string = bounding_box_from_center_and_dimensions(center[0], center[1], dimensions[0], dimensions[1])
    osmGet.get(f"--bbox {bbox_string} -d {build_dir}/{scenario_dir}/{config_dir} -p map")
    osmBuild.build(
        f"-f {build_dir}/{scenario_dir}/{config_dir}/map_bbox.osm.xml -d {build_dir}/{scenario_dir}/{config_dir} -p network --pedestrians"
    )
    logger.debug("Config files created.")

    scenario_data = {
        "scenario": {
            "name": scenario_dir,
            "centerX": center[0],
            "centerY": center[1],
            "sizeX": dimensions[0],
            "sizeY": dimensions[1],
            "baseStations": -1,
        }
    }
    json_path = os.path.join(build_dir, scenario_dir, "scenario.json")
    with open(json_path, "w") as json_file:
        json.dump(scenario_data, json_file)

    return scenario_dir


def parse_args():
    parser = argparse.ArgumentParser(description="Automatically generate SUMO scenario from OSM data.")
    parser.add_argument(
        "-c",
        "--center",
        type=float,
        default=[52.54603100769271, 13.49809958325796],
        help="Center of the area to be downloaded.",
        nargs=2,
    )
    parser.add_argument(
        "-d",
        "--dimensions",
        type=float,
        default=[2.0, 2.0],
        help="Dimensions of the area to be downloaded in km. Width,Height.",
        nargs=2,
    )
    parser.add_argument(
        "-f",
        "--function_name",
        type=str,
        default="config",
        help="Which function to execute.",
    )
    parser.add_argument("-p", "--persons", type=float, default=10, help="Number of persons to be generated.")
    parser.add_argument("-o", "--scenario_dir", type=str, default="unset", help="Directory of the scenario.")
    parser.add_argument("-s", "--seed", type=int, default=42, help="Seed for user equipment trace generation.")
    parser.add_argument(
        "-W", "--no-warnings", action="store_true", help="Disable or enable warnings produced by netconvert"
    )
    return parser.parse_args()


def getMapParams(scenario_dir):
    tree = ET.parse(f"{build_dir}/{scenario_dir}/{config_dir}/network.net.xml")
    root = tree.getroot()
    location_elem = root.find("location")
    net_offset = location_elem.get("netOffset")
    conv_boundary = location_elem.get("convBoundary")

    return tuple(map(float, net_offset.split(","))), tuple(map(float, conv_boundary.split(",")))


def add_base_stations(scenario_dir):
    """Adds relevant base stations as points of interest."""
    if directory_exists(f"{build_dir}/{scenario_dir}/{trace_dir}/base_stations.tcl"):
        logger.info("Base stations already added.")
        json_path = os.path.join(build_dir, scenario_dir, "scenario.json")
        with open(json_path, "r") as json_file:
            scenario_data = json.load(json_file)
            bs = scenario_data["scenario"]["baseStations"]
        return bs

    logger.debug("Attempting to add base stations...")

    # fetch center from scenario.json
    with open(f"{build_dir}/{scenario_dir}/scenario.json", "r") as json_file:
        scenario_data = json.load(json_file)
        longitude = scenario_data["scenario"]["centerY"]
        # this is error-prone as the scenario might be on the border of two UTM zones
        if longitude > 12:
            utm_proj = pyproj.Proj(proj="utm", zone=33, ellps="WGS84")
        elif longitude > 6:
            utm_proj = pyproj.Proj(proj="utm", zone=32, ellps="WGS84")
        else:
            utm_proj = pyproj.Proj(proj="utm", zone=31, ellps="WGS84")

    net_offset, conv_boundary = getMapParams(scenario_dir)
    (west, south, east, north) = conv_boundary
    logger.debug(f"Boundary: {west}, {south}, {east}, {north}")

    node_counter = 0

    with (
        open(NS3_HOME + "/contrib/defiance/utils/data/positions.jsonl") as source_file,
        open(f"{build_dir}/{scenario_dir}/{trace_dir}/base_stations.tcl", "w") as mobility_trace,
    ):
        for line_number, line in enumerate(source_file):
            # Load the JSON object from the line
            entry = json.loads(line)
            if entry["kind"] == "GetStandorteFreigabe":
                position = entry["position"]
                long = position["Lng"]
                lat = position["Lat"]
                x, y = convert_coordinates(long, lat, net_offset, utm_proj)
                mobilfunk_antennas = list(filter(lambda x: x["type"] == "Mobilfunk", entry["antennas"]))

                if west < x < east and south < y < north and len(mobilfunk_antennas) > 0:
                    mobilfunk_antenna_heights = list(map(lambda x: x["height"], mobilfunk_antennas))
                    avg_antenna_height = sum(mobilfunk_antenna_heights) / len(mobilfunk_antenna_heights)

                    mobility_trace.write(f"$node_({node_counter}) set X_ {x}\n")
                    mobility_trace.write(f"$node_({node_counter}) set Y_ {y}\n")
                    mobility_trace.write(f"$node_({node_counter}) set Z_ {avg_antenna_height}\n")

                    node_counter += 1

    if node_counter == 0:
        logger.warning("No base stations found at this location.")
    else:
        logger.info(f"Added {node_counter} base stations.")
        print(f"Added {node_counter} base stations.")

    json_path = os.path.join(build_dir, scenario_dir, "scenario.json")
    with open(json_path, "r") as json_file:
        scenario_data = json.load(json_file)

    scenario_data["scenario"]["baseStations"] = node_counter

    with open(json_path, "w") as json_file:
        json.dump(scenario_data, json_file)

    return node_counter


def generate_random_routes(persons, center, dimensions, scenario_dir, seed=42, no_warnings=False):
    logger.debug("Attempting to generate random routes...")
    t0 = 0
    t1 = 1
    n = persons + round(persons * 0.2) + 1
    # scenario_dir = create_config_files(center, dimensions)
    randomTrips.main(
        randomTrips.get_options(
            f"-n {build_dir}/{scenario_dir}/{config_dir}/network.net.xml --seed {seed} -o {build_dir}/{scenario_dir}/{config_dir}/trips.trips.xml  -b {t0} -e {t1} -p {(t1 - t0) / n} --intermediate 1 --route-file {build_dir}/{scenario_dir}/{config_dir}/routes.rou.xml --pedestrians"
        )
    )
    logger.debug("Random routes generated.")


def start_sim(scenario_dir):
    """Starts the simulation."""
    logger.debug("Starting simulation...")
    libsumo.start(
        cmd=[
            sumoBinary,
            "--net-file",
            f"{build_dir}/{scenario_dir}/{config_dir}/network.net.xml",
            "--route-files",
            f"{build_dir}/{scenario_dir}/{config_dir}/routes.rou.xml",
            "--delay",
            "200",
            "--fcd-output",
            f"{build_dir}/{scenario_dir}/{trace_dir}/sumoTrace.xml",
        ]
    )
    logger.debug("Simulation started.")


def should_continue_sim():
    """Checks whether the simulation should continue running.
    Returns:
        bool: `True` if not all vehicles have arrived. `False` otherwise.
    """
    numVehicles = libsumo.simulation.getMinExpectedNumber()
    return True if numVehicles > 0 else False


def generate_traces(scenario_dir, seed=42):
    """Generate base station trace file and user equipment trace file from SUMO network."""
    logger.debug("Generating traces...")
    dir = f"{build_dir}/{scenario_dir}/{trace_dir}"

    step = 0
    start_sim(scenario_dir)
    while should_continue_sim():
        libsumo.simulationStep()
        step += 1

    libsumo.close()
    # export to ns2 tracefile
    traceExporter.main(
        [
            "--fcd-input",
            dir + "/sumoTrace.xml",
            "--ns2mobility-output",
            dir + "/traffic.tcl",
            "--persons",
            "true",
            "--seed",
            seed,
        ]
    )
    logger.debug("Traces generated.")


def main(args=None) -> None | int:
    args = parse_args()
    logger.warning(f"Calling main() with scenario_dir={args.scenario_dir} and function_name={args.function_name}.")
    logger.warning("This functionality may be removed soon.")
    generate_traces(args.scenario_dir, args.seed)
    return 0


if __name__ == "__main__":
    main()
