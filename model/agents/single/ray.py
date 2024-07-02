"""Agent leveraging ray to train a single agent for a certain ns3 environment."""

import logging
from typing import Any

import gymnasium as gym
import ns3ai_gym_env  # noqa: F401  # import to register env
import ray
from gymnasium.wrappers import TimeLimit
from ray.air import CheckpointConfig, RunConfig
from ray.rllib.algorithms import PPOConfig
from ray.rllib.evaluation import Episode
from ray.tune import Tuner, register_env
from typing_extensions import override

from defiance.model.agents.ray import DefianceCallbacks

logger = logging.getLogger(__name__)


class SingleDefianceCallbacks(DefianceCallbacks):
    @override
    def on_episode_step(self, *, episode: Episode, **kwargs: Any) -> None:
        info = episode.last_info_for() or {}
        for k, raw in info.items():
            try:
                v = float(raw)
            except ValueError:
                logger.exception("Could not log %s: %s", k, raw)
                continue
            episode.user_data[k].append(v)
            episode.hist_data[k].append(v)


def start_training(
    env_name: str,
    max_episode_steps: int,
    iterations: int,
    training_params: dict[str, Any],
    load_checkpoint_path: str | None = None,
    **ns3_settings: str,
) -> None:
    logger.info("Loading checkpoints is not supported for single agent: %s", load_checkpoint_path)
    env = TimeLimit(
        gym.make("ns3ai_gym_env/Ns3-v0", targetName=env_name, ns3Path=".", ns3Settings=ns3_settings),
        max_episode_steps=max_episode_steps,
    )

    register_env("defiance", lambda _: env)

    try:
        ray.init()
        training_params = {"lr": 5e-5, "clip_param": 0.3, "train_batch_size": 128} | training_params
        config = PPOConfig().training(**training_params)
        config = config.resources(num_gpus=0).rollouts(num_rollout_workers=1, num_envs_per_worker=1)
        config.framework_str = "tf"
        config = config.environment(env="defiance")
        config = config.callbacks(SingleDefianceCallbacks)

        logger.info("Training...")
        Tuner(
            "PPO",
            run_config=RunConfig(
                stop={"training_iteration": iterations},
                checkpoint_config=CheckpointConfig(checkpoint_at_end=True),
            ),
            param_space=config.to_dict(),
        ).fit()

        ray.shutdown()

    except Exception:
        logger.exception("Exception occurred!")
    finally:
        env.close()
