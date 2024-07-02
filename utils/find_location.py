import argparse
import json
import logging
import math
import random
import rtree
import os
from defiance import NS3_HOME

logger = logging.getLogger(__name__)


def calculate_distance(lat1, lon1, lat2, lon2):
    # Convert latitude and longitude to radians
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)

    # Haversine formula
    dlon = lon2_rad - lon1_rad
    dlat = lat2_rad - lat1_rad
    a = math.sin(dlat / 2) ** 2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(dlon / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    radius = 6371  # Radius of the Earth in kilometers

    return c * radius + 0.0001  # Add a small value to avoid division by zero


def find_center_point(included_stations):
    num_stations = len(included_stations)
    total_lat = sum(station["position"]["Lat"] for station in included_stations)
    total_lon = sum(station["position"]["Lng"] for station in included_stations)
    center_lat = total_lat / num_stations
    center_lon = total_lon / num_stations
    return center_lat, center_lon


def find_area_with_base_stations(target_base_stations, seed, idx, base_stations, min_density, max_density):
    random.seed(seed)
    eps = 0.001  # Approximately 1m

    attempts = 0
    while True:
        attempts += 1
        # Start with a random base station
        start_station = random.choice(base_stations)
        center_lat, center_lon = start_station["position"]["Lat"], start_station["position"]["Lng"]

        min_radius = eps
        max_radius = 1000.0  # Many km

        while True:
            radius = (min_radius + max_radius) / 2
            min_lat, max_lat = center_lat - radius, center_lat + radius
            min_lon, max_lon = center_lon - radius, center_lon + radius

            included_stations_idx = list(idx.intersection((min_lon, min_lat, max_lon, max_lat)))

            if len(included_stations_idx) == target_base_stations:
                break
            elif len(included_stations_idx) < target_base_stations:
                min_radius = radius
            else:
                max_radius = radius
            if max_radius - min_radius < 0.0001:
                break

        # Retry if not converged to the target number of base stations
        if len(included_stations_idx) != target_base_stations:
            continue
        included_stations = [base_stations[i] for i in included_stations_idx]

        # center_lat, center_lon = find_center_point(included_stations)
        min_lat, max_lat = (
            min(station["position"]["Lat"] for station in included_stations),
            max(station["position"]["Lat"] for station in included_stations),
        )
        min_lon, max_lon = (
            min(station["position"]["Lng"] for station in included_stations),
            max(station["position"]["Lng"] for station in included_stations),
        )
        height = calculate_distance(min_lat, center_lon, max_lat, center_lon)
        width = calculate_distance(center_lat, min_lon, center_lat, max_lon)
        area_sq_km = height * width
        density = len(included_stations) / area_sq_km

        # Expand the area using a linear search while maintaining the number of base stations
        if density > max_density:
            # Large step size for the first iteration(s)
            max_step_size = 70.0
            min_step_size = eps
            while True:
                step_size = (min_step_size + max_step_size) / 2
                min_lat_new = min_lat - step_size
                max_lat_new = max_lat + step_size
                min_lon_new = min_lon - step_size
                max_lon_new = max_lon + step_size
                included_stations_idx = list(idx.intersection((min_lon_new, min_lat_new, max_lon_new, max_lat_new)))

                if (max_step_size - min_step_size) < eps:
                    break
                # Check if the number of base stations is still within the target range
                if len(included_stations_idx) > target_base_stations:
                    max_step_size = step_size
                    continue

                height = calculate_distance(min_lat_new, center_lon, max_lat_new, center_lon)
                width = calculate_distance(center_lat, min_lon_new, center_lat, max_lon_new)
                area_sq_km = height * width
                density = len(included_stations) / area_sq_km
                logger.info(f"New density: {round(density, 2)}")
                if density < min_density:
                    max_step_size = step_size
                else:
                    min_step_size = step_size

            height = calculate_distance(min_lat_new, center_lon, max_lat_new, center_lon)
            width = calculate_distance(center_lat, min_lon_new, center_lat, max_lon_new)
            area_sq_km = height * width
            density = len(included_stations) / area_sq_km

        if min_density <= density <= max_density and len(included_stations_idx) == target_base_stations:
            logger.info(f"Found area with density of {round(density, 2)} after {attempts} attempts!")
            print(f"Found area with density of {round(density, 2)} after {attempts} attempts!")
            break
        else:
            logger.debug(
                f"Failed to find area with density of {min_density} <= {round(density, 2)} <= {max_density} and {len(included_stations_idx)} == {target_base_stations}! Retrying..."
            )
            continue

    return center_lat, center_lon, height, width, density


def read_data():
    with open(NS3_HOME + "/contrib/defiance/utils/data/positions.jsonl", "r") as file:
        lines = file.readlines()
    filtered_data = []
    for line in lines:
        obj = json.loads(line)
        # Check if the object has the type "GetStandorteFreigabe"
        if obj.get("kind") == "GetStandorteFreigabe":
            filtered_data.append(obj)

    return filtered_data


def parse_args():
    parser = argparse.ArgumentParser(description="Find location with given number of base stations")

    (
        parser.add_argument(
            "-b", "--base-stations", type=int, default=1, help="Number of base stations to be considered"
        ),
    )
    parser.add_argument("-s", "--seed", type=int, default=42, help="Seed for random number generator")
    parser.add_argument("-s", "--seed", type=int, default=48, help="Seed for random number generator")
    parser.add_argument(
        "-d",
        "--density",
        type=float,
        default=[1, 2],
        help="Minimum and maximum density of base stations per square kilometer",
        nargs=2,
    )

    return parser.parse_args()


def find_loc(num_bs, seed, min_density=20, max_density=25):
    if min_density > max_density:
        raise ValueError("Minimum density cannot be greater than maximum density.")
    base_stations = read_data()
    idx = rtree.index.Index()

    for i, station in enumerate(base_stations):
        idx.insert(
            i,
            (
                station["position"]["Lng"],
                station["position"]["Lat"],
                station["position"]["Lng"],
                station["position"]["Lat"],
            ),
        )

    res = find_area_with_base_stations(num_bs, seed, idx, base_stations, min_density, max_density)
    return res


def main(args=None):
    args = parse_args()
    base_stations = read_data()
    idx = rtree.index.Index()

    for i, station in enumerate(base_stations):
        idx.insert(
            i,
            (
                station["position"]["Lng"],
                station["position"]["Lat"],
                station["position"]["Lng"],
                station["position"]["Lat"],
            ),
        )

    res = find_area_with_base_stations(
        args.base_stations, args.seed, idx, base_stations, args.density[0], args.density[1]
    )
    logger.info(res)
    return res


if __name__ == "__main__":
    main()
