import logging
import os
from typing import Any, SupportsFloat

import gymnasium as gym
import ns3ai_gym_env  # noqa: F401  # import to register env
import pandas as pd
from ray.tune.registry import register_env

NS3_HOME = os.getenv("NS3_HOME") or "/code/ns3"
logger = logging.getLogger(__name__)


def env_creator(_: str) -> gym.Env:
    return env


propagation_models = [
    "ns3::FriisPropagationLossModel",
    "ns3::TwoRayGroundPropagationLossModel",
    "ns3::LogDistancePropagationLossModel",
    "ns3::ThreeLogDistancePropagationLossModel",
    "ns3::RandomPropagationLossModel",
    "ns3::FixedRssLossModel",
    "ns3::RangePropagationLossModel",
]

cpp_settings = {
    "speed": 100,
    "topology": "simple",
    "noiseFigure": 9.0,
    "propagationModel": "ns3::FriisPropagationLossModel",
}
python_settings = {"txPowers": [15]}

threshold = 2000

if __name__ == "__main__":
    global_info: dict[str, list[Any]] = {
        "propagationModel": [],
        "action": [],
        "sinrs": [],
        "distances": [],
    }

    env = gym.make(
        "ns3ai_gym_env/Ns3-v0",
        targetName="defiance-lte-learning",
        ns3Path=NS3_HOME,
        ns3Settings=cpp_settings,
    )
    register_env("defiance", env_creator)

    try:
        for propagation_model in propagation_models:
            cpp_settings["propagationModel"] = propagation_model
            logger.info("Propagation Model: %s", propagation_model)

            for power in python_settings["txPowers"]:
                obs, info = env.reset()
                logger.info("Initial Observations after Env was reset %s", obs)
                logger.info("Initial Info after Env was reset %s", info)

                reward: SupportsFloat = 0
                done = False

                steps = 0
                while True:
                    if steps % 100 == 0:
                        logger.info("Progress %s", steps)
                        logger.info("Info %s", info)

                    action = [power]
                    global_info["action"].append(action[0])

                    obs, reward, done, _, info = env.step(action)

                    global_info["sinrs"].append(info["sinr_bs0ue0"])
                    global_info["distances"].append(info["distance_0"])
                    global_info["propagationModel"].append(propagation_model)

                    steps += 1
                    if steps == threshold:
                        break

    except Exception:
        logger.exception("Exception occurred")

    finally:
        logger.info("Finally exiting...")
        env.close()

    pd.DataFrame(global_info).to_csv(NS3_HOME + "/contrib/defiance/experiments/results/test2.csv")
