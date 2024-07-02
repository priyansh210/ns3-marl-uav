import logging
from collections.abc import Iterable
from typing import cast

import geopandas as gpd
import matplotlib.pyplot as plt
import osmnx as ox
import pandas as pd
from defusedxml import ElementTree
from matplotlib.animation import FuncAnimation
from matplotlib.artist import Artist
from matplotlib.axes import Axes

logger = logging.getLogger(__name__)

xml = ElementTree.parse("config_files/network.net.xml")
location = xml.find(".//location")
if not location:
    msg = "invalid xml!"
    raise ValueError(msg)
orig_boundary_value = location.attrib["origBoundary"]
logger.info(orig_boundary_value)
bbox = list(map(float, orig_boundary_value.split(",")))
new_bbox = (bbox[3], bbox[1], bbox[0], bbox[2])

# Create the graph from the bounding box
graph = ox.graph_from_bbox(bbox=new_bbox[0:4], network_type="walk")
nodes, streets = ox.graph_to_gdfs(graph)

positions = pd.read_csv("./results/vehicle_position.csv")
gdf = gpd.GeoDataFrame(positions, geometry=gpd.points_from_xy(positions.long, positions.lat), crs="EPSG:4326")
gdf = gdf.to_crs(streets.crs)

# Set color of streets
streets["color"] = "blue"  # default color

# Base stations

base_stations_df = pd.read_csv("./results/base_stations.csv")
bs_gdf = gpd.GeoDataFrame(
    base_stations_df, geometry=gpd.points_from_xy(base_stations_df.long, base_stations_df.lat), crs="EPSG:4326"
)
bs_gdf = bs_gdf.to_crs(streets.crs)

# Plotting
fig, ax = plt.subplots(figsize=(30, 30))
ax_streets = fig.add_subplot(111)

streets_plot = streets.plot(ax=ax_streets, linewidth=1, edgecolor=streets["color"], zorder=1)
bs_plot = bs_gdf.plot(ax=ax_streets, marker="o", color="green", markersize=200, zorder=2)
ax_cars: Axes = cast(Axes, ax_streets.twinx())
ax_cars.set_ylim(ax_streets.get_ylim())
ax_lines: Axes = cast(Axes, ax_streets.twinx())
ax_lines.set_ylim(ax_streets.get_ylim())


def init() -> Iterable[Artist]:
    cars = gdf[gdf["step"] == 0]
    cars_plot = cars.plot(ax=ax_cars, marker="o", color="red", markersize=100, zorder=3)
    for _, car in cars.iterrows():
        ax_lines.plot(
            car.geometry.x,
            car.geometry.y,
            bs_gdf.iloc[0].geometry.x,
            bs_gdf.iloc[0].geometry.y,
            color="black",
            linewidth=100,
            zorder=2,
        )
    return cars_plot.collections


def update(frame: int, axis: Axes = ax_cars) -> Iterable[Artist]:
    if frame % 50 == 0:
        logger.info("Frame %s", frame)
    for artist in axis.collections:
        artist.remove()
    for lines in axis.lines:
        lines.remove()
    cars = gdf[gdf["step"] == frame]
    for _, car in cars.iterrows():
        # cars_plot =
        ax_lines.plot(
            car.geometry.x,
            car.geometry.y,
            bs_gdf.iloc[0].geometry.x,
            bs_gdf.iloc[0].geometry.y,
            color="black",
            linewidth=100,
            zorder=2,
        )
        axis.scatter(car.geometry.x, car.geometry.y, color="red", s=75, zorder=3)

        car_x, car_y = car.geometry.x, car.geometry.y
        base_station_x, base_station_y = bs_gdf.iloc[0].geometry.x, bs_gdf.iloc[0].geometry.y

        axis.plot([car_x, base_station_x], [car_y, base_station_y], color="black", linewidth=1, zorder=5)
    return axis.collections


anim = FuncAnimation(fig, update, frames=100, init_func=init, blit=True)
anim.save("animation.mp4", fps=10, writer="ffmpeg")
logger.info("done")
