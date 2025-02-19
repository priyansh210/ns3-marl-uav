import subprocess
from io import BytesIO
from pathlib import Path
from shutil import rmtree

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from defiance import NS3_HOME

data = pd.read_csv(NS3_HOME + "/stats.csv", names=["NodeID", "PendulumLength", "Angle", "XPos"])

node_ids = []


def plot_frame(frame: int) -> None:
    fig_anim, ax_anim = plt.subplots()
    node_id = data.iloc[frame]["NodeID"]
    if node_id not in node_ids:
        node_ids.append(node_id)
    position = data.iloc[frame]["XPos"]
    circle = plt.Circle((float(position), 0), 1, color="b", fill=False)
    ax_anim.add_artist(circle)
    # Calculate coordinates of the point on the unit circle
    # Quick fix: negative angle
    angle_rad = data.iloc[frame]["Angle"]
    # rotate the angle by 90 degrees to match the transformation -> as theta is typically measured from the x-axis but we do it from the y-axis
    angle_rad = angle_rad + np.pi / 2
    x = position + np.cos(angle_rad)
    y = np.sin(angle_rad)
    # Plot the point
    ax_anim.plot([position, x], [0, y], color="r", marker="o")
    # Set plot limits and aspect ratio
    min_position = min(-2, *data["XPos"])
    max_position = max(2, *data["XPos"])
    ax_anim.set_xlim(min_position, max_position)
    ax_anim.set_ylim(-1.5, 1.5)
    ax_anim.set_aspect("equal")

    io = BytesIO()
    plt.savefig(io)
    plt.savefig(output / f"image_{node_id}_{str(frame).zfill(8)}.png")
    plt.close()


if __name__ == "__main__":
    output = Path(NS3_HOME) / "plots"
    if output.exists():
        rmtree(output)
    output.mkdir()
    for i in range(len(data)):
        plot_frame(i)
    for node_id in node_ids:
        subprocess.run(
            [  # noqa: S607, S603  command is properly escaped
                "ffmpeg",
                "-framerate",
                "30",
                "-pattern_type",
                "glob",
                "-i",
                f"{output / f'image_{node_id}_*.png'}",
                "-c:v",
                "libx264",
                "-pix_fmt",
                "yuv420p",
                f"out{node_id}.mp4",
            ],
            check=False,
        )
