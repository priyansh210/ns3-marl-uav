import argparse
import os

import libsumo  # noqa: F401  # import to be able to import osmBuild
import logging
import osmBuild
import osmGet
import randomTrips
from geopy import distance
from defiance import NS3_HOME

build_dir: str = NS3_HOME + "/contrib/defiance/scenarios/examples/sumo-test"
logger = logging.getLogger(__name__)


def create_directory(path):
    try:
        # Create the directory
        os.makedirs(path)
        logger.debug(f"Directory '{path}' created successfully.")
    # if directory already exists
    except OSError as error:
        logger.error(f"Error creating directory '{path}': {error}")


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
    parser.add_argument(
        "-d",
        "--dimensions",
        type=float,
        default=[2.0, 2.0],
        help="Dimensions of the area to be downloaded in km. Width,Height.",
        nargs=2,
    )
    parser.add_argument("-p", "--persons", type=float, default=10, help="Number of persons to be generated.")
    parser.add_argument(
        "-o",
        "--output_directory",
        type=str,
        default=f"{build_dir}/config_files",
        help="Output directory for the generated files.",
    )
    parser.add_argument("-s", "--scenario_size", type=int, default=10, help="Size of the scenario.")
    return parser.parse_args()


def main(args=None):
    args = parse_args()
    t0 = 0
    t1 = 1
    n = args.persons + round(args.persons * 0.2) + 1
    create_directory(args.output_directory)
    center = args.center
    dimensions = args.dimensions
    scenario_size = args.scenario_size
    bbox_string = bounding_box_from_center_and_dimensions(center[0], center[1], dimensions[0], dimensions[1])
    osmGet.get(f"--bbox {bbox_string} -d {build_dir}/config_files -p map")
    osmBuild.build(f"-f {build_dir}/config_files/map_bbox.osm.xml -d {build_dir}/config_files -p network --pedestrians")
    randomTrips.main(
        randomTrips.get_options(
            f"-n {build_dir}/config_files/network.net.xml --seed 42 -o {build_dir}/config_files/trips.trips.xml  -b {t0} -e {t1} -p {(t1 - t0) / n} --intermediate 1 --route-file {build_dir}/config_files/routes.rou.xml --pedestrians"
        )
    )


if __name__ == "__main__":
    main()
