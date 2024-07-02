import argparse
import json
import os
import re

import libsumo as traci
import osmBuild
import osmGet
import randomTrips
import traceExporter
from geopy import distance
from sumolib import checkBinary
from defiance import NS3_HOME

sumoBinary = checkBinary("sumo")
build_dir: str = NS3_HOME + "/contrib/defiance/scenarios/examples/topology-creation"


def create_directory(path):
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"Directory '{path}' created successfully.")


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
    (
        parser.add_argument(
            "-d",
            "--dimensions",
            type=float,
            default=[2.0, 2.0],
            help="Dimensions of the area to be downloaded in km. Width,Height.",
            nargs=2,
        ),
    )
    parser.add_argument("-p", "--persons", type=float, default=10, help="Number of persons to be generated.")
    parser.add_argument(
        "-o",
        "--config_dir",
        type=str,
        default=f"{build_dir}/config_files",
        help="Output directory for the generated files.",
    )
    parser.add_argument(
        "-t",
        "--trace_directory",
        type=str,
        default=f"{build_dir}/traces",
        help="Output directory for the generated trace files.",
    )
    return parser.parse_args()


def generate_random_routes(persons, config_dir, center, dimensions):
    t0 = 0
    t1 = 1
    # Some routes get discarded, so we need to generate some extra
    n = persons + round(persons * 0.2) + 1
    create_directory(config_dir)
    # scenario_size = args.scenario_size
    bbox_string = bounding_box_from_center_and_dimensions(center[0], center[1], dimensions[0], dimensions[1])
    osmGet.get(f"--bbox {bbox_string} -d {config_dir} -p map")
    osmBuild.build(f"-f {config_dir}/map_bbox.osm.xml -d {config_dir} -p network --pedestrians")
    randomTrips.main(
        randomTrips.get_options(
            f"-n {config_dir}/network.net.xml --seed 42 -o {config_dir}/trips.trips.xml  -b {t0} -e {t1} -p {(t1 - t0) / n} --intermediate 1 --route-file {config_dir}/routes.rou.xml --pedestrians"
        )
    )


def add_base_stations(trace_dir):
    """Adds relevant base stations as points of interest."""

    # find the bounding box of the network
    (west, south), (east, north) = traci.simulation.getNetBoundary()
    counter = 0

    with (
        open(f"{NS3_HOME}/contrib/defiance/utils/data/positions.jsonl") as source_file,
        open(trace_dir + "/base_stations.tcl", "w") as mobility_trace,
    ):
        for line_number, line in enumerate(source_file):
            # Load the JSON object from the line
            entry = json.loads(line)
            if entry["kind"] == "GetStandorteFreigabe":
                id = f"cell_tower_{entry['standortbescheinigung_nr']}"
                position = entry["position"]
                long = position["Lng"]
                lat = position["Lat"]
                x, y = traci.simulation.convertGeo(long, lat, fromGeo=True)
                mobilfunk_antennas = list(filter(lambda x: x["type"] == "Mobilfunk", entry["antennas"]))

                if west < x < east and south < y < north and len(mobilfunk_antennas) > 0:
                    traci.poi.add(
                        poiID=str(counter), x=x, y=y, color=(255, 0, 0), width=1, height=1, poiType="cell_tower"
                    )
                    mobilfunk_antenna_heights = list(map(lambda x: x["height"], mobilfunk_antennas))
                    avg_antenna_height = sum(mobilfunk_antenna_heights) / len(mobilfunk_antenna_heights)

                    mobility_trace.write(f"$node_({counter}) set X_ {x}\n")
                    mobility_trace.write(f"$node_({counter}) set Y_ {y}\n")
                    mobility_trace.write(f"$node_({counter}) set Z_ {avg_antenna_height}\n")

                    counter += 1
    return counter


def start_sim(trace_dir, config_dir):
    """Starts the simulation."""
    traci.start(
        [
            sumoBinary,
            "--net-file",
            f"{config_dir}/network.net.xml",
            "--route-files",
            f"{config_dir}/routes.rou.xml",
            "--delay",
            "200",
            "--fcd-output",
            f"{trace_dir}/sumoTrace.xml",
        ]
    )


def should_continue_sim() -> bool:
    """Checks that the simulation should continue running.
    Returns:
        bool: `True` if vehicles exist on network. `False` otherwise.
    """
    num_vehicles = traci.simulation.getMinExpectedNumber()
    return True if num_vehicles > 0 else False


def modify_tracefile(input_filename, output_filename, k):
    """Update the node indices in the UE tracefile according to the number of base stations."""
    # Define the regular expression pattern to match node indices
    node_pattern = re.compile(r"node_\((\d+)\)")

    with open(input_filename) as infile:
        with open(output_filename, "w") as outfile:
            for line in infile:
                matches = node_pattern.findall(line)

                if matches:
                    for match in matches:
                        # Extract the node index as a string
                        node_index_str = match
                        node_index = int(node_index_str)
                        node_index += k
                        line = line.replace(f"node_({node_index_str})", f"node_({node_index})")

                outfile.write(line)


def generate_traces(trace_dir, config_dir):
    """Generate base station trace file and user equipment trace file from SUMO network."""
    create_directory(trace_dir)

    step = 0
    start_sim(trace_dir, config_dir)
    num_bs = add_base_stations(trace_dir)
    while should_continue_sim():
        traci.simulationStep()
        step += 1

    traci.close()
    # export to ns2 tracefile
    traceExporter.main(
        [
            "--fcd-input",
            f"{trace_dir}/sumoTrace.xml",
            "--ns2mobility-output",
            f"{trace_dir}/traffic.tcl",
            "--persons",
            "true",
            "--seed",
            "42",
        ]
    )
    # Define the input and output filenames and the increment value for node indices
    input_filename = trace_dir + "/traffic.tcl"
    output_filename = trace_dir + "/modified_traffic.tcl"
    # Modify the tracefile
    modify_tracefile(input_filename, output_filename, num_bs)

    print(f"Node indices have been incremented by {num_bs}. Modified file saved as {output_filename}")


def main(args=None):
    args = parse_args()
    generate_random_routes(args.persons, args.config_dir, args.center, args.dimensions)
    generate_traces(args.trace_directory, args.config_dir)


if __name__ == "__main__":
    main()
