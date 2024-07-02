import json
import os

import libsumo as traci
import traceExporter
from sumolib import checkBinary
from defiance import NS3_HOME

sumoBinary = checkBinary("sumo")
build_dir: str = NS3_HOME + "/contrib/defiance/scenarios/examples/sumo-test"
out_dir: str = "traces"


# Export base stations to ns2 tracefile
def add_base_stations():
    """Adds relevant base stations as points of interest."""

    # find the bounding box of the network
    (west, south), (east, north) = traci.simulation.getNetBoundary()
    counter = 0

    with (
        open(f"{NS3_HOME}/contrib/defiance/utils/data/positions.jsonl") as source_file,
        open(f"{build_dir}/{out_dir}/base_stations.tcl", "w") as mobility_trace,
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


def start_sim():
    """Starts the simulation."""
    traci.start(
        [
            sumoBinary,
            "--net-file",
            f"{build_dir}/config_files/network.net.xml",
            "--route-files",
            f"{build_dir}/config_files/routes.rou.xml",
            # "--additional-files",
            # "./config_files/network.add.xml",
            "--delay",
            "200",
            "--fcd-output",
            f"{build_dir}/{out_dir}/sumoTrace.xml",
        ]
    )


def should_continue_sim():
    """Checks that the simulation should continue running.
    Returns:
        bool: `True` if vehicles exist on network. `False` otherwise.
    """
    numVehicles = traci.simulation.getMinExpectedNumber()
    return True if numVehicles > 0 else False


def main():
    if not os.path.exists(f"{build_dir}/{out_dir}"):
        os.makedirs(f"{build_dir}/{out_dir}")

    step = 0
    start_sim()
    add_base_stations()
    while should_continue_sim():
        traci.simulationStep()
        step += 1

    traci.close()
    # export to ns2 tracefile
    traceExporter.main(
        [
            "--fcd-input",
            f"{build_dir}/{out_dir}/sumoTrace.xml",
            "--ns2mobility-output",
            f"{build_dir}/{out_dir}/traffic.tcl",
            "--persons",
            "true",
            "--seed",
            "42",
        ]
    )


if __name__ == "__main__":
    main()
