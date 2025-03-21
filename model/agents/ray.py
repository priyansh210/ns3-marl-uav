"""!Agent leveraging ray to train an agent for a certain ns3 environment."""

import logging
from collections import defaultdict
from functools import partial
from pathlib import Path
from typing import Any

import numpy as np
import ray
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv
from ray.air import CheckpointConfig, RunConfig
from ray.air.integrations.wandb import WandbLoggerCallback
from ray.rllib import Policy, RolloutWorker, SampleBatch
from ray.rllib.algorithms import AlgorithmConfig, DQNConfig, PPOConfig
from ray.rllib.algorithms.callbacks import DefaultCallbacks
from ray.rllib.env import BaseEnv
from ray.rllib.evaluation import Episode
from ray.rllib.policy.policy import PolicySpec
from ray.tune import Tuner, register_env
from typing_extensions import override

from defiance import NS3_HOME
from defiance.utils import first

logger = logging.getLogger(__name__)


class DefianceCallbacks(DefaultCallbacks):
    """!Ray callbacks for multi-agent ns3-ai integration into tensorflow"""

    @override
    def on_episode_start(
        self,
        *,
        episode: Episode,
        **kwargs: Any,
    ) -> None:
        logger.info("episode %s started", episode.episode_id)
        episode.user_data = defaultdict(list, episode.user_data)
        episode.hist_data = defaultdict(list, episode.hist_data)

    @override
    def on_episode_step(self, *, base_env: BaseEnv, episode: Episode, **kwargs: Any) -> None:
        logger.info("ON EPISODE STEP CALLBACK WAS EXECUTED")
        for agent in base_env.get_agent_ids():
            info = episode.last_info_for(agent) or {}
            for k, raw in info.items():
                try:
                    v = float(raw)
                except ValueError:
                    logger.exception("Could not log %s: %s", k, raw)
                    continue
                episode.user_data[k].append(v)
                episode.hist_data[k].append(v)

    @override
    def on_episode_end(
        self,
        *,
        episode: Episode,
        **kwargs: Any,
    ) -> None:
        logger.info("ON EPISODE END CALLBACK WAS EXECUTED")
        for k, v in episode.user_data.items():
            episode.custom_metrics[f"{k}_mean"] = np.mean(v)

    @override
    def on_sample_end(self, *, samples: SampleBatch, **kwargs: Any) -> None:
        logger.info("returned sample batch of size %s", samples.count)

    @override
    def on_postprocess_trajectory(
        self,
        worker: RolloutWorker,
        episode: Episode,
        agent_id: str,
        policy_id: str,
        policies: dict[str, Policy],
        postprocessed_batch: SampleBatch,
        original_batches: dict[str, SampleBatch],
        **kwargs: Any,
    ) -> None:
        if "num_batches" not in episode.custom_metrics:
            episode.custom_metrics["num_batches"] = 0
        episode.custom_metrics["num_batches"] += 1


def start_inference(env_name: str, load_checkpoint_path: str | Path, **ns3_settings: str) -> None:
    load_checkpoint_path = Path(load_checkpoint_path)
    if not load_checkpoint_path.exists():
        msg = "load_checkpoint_path is required for inference"
        raise ValueError(msg)

    if (load_checkpoint_path / "best_checkpoint").exists():
        policies = Policy.from_checkpoint(str(load_checkpoint_path / "best_checkpoint"))
    else:
        policies = Policy.from_checkpoint(str(load_checkpoint_path))

    ns3_settings["visualize"] = ""
    env = Ns3MultiAgentEnv(targetName=env_name, ns3Path=NS3_HOME, ns3Settings=ns3_settings, trial_name="bootup")
    reset = env.reset()
    agent, observation = first(reset[0].items())
    _, info = first(reset[1].items())
    terminated, truncated = False, False
    time_running = 0.0

    while True:
        if "shared_policy" in policies:
            action = policies["shared_policy"].compute_single_action(observation)[0]
        else:
            flat_obs = {}
            for key in observation:
                flat_obs[key] = np.concatenate([observation[key][key2] for key2 in observation[key]])
            flattened_obs = np.concatenate([flat_obs[key] for key in flat_obs])
            # this changes based on policy_mapping
            action = policies[agent].compute_single_action(obs=flattened_obs)[0]
        states = env.step({agent: action})

        terminated = terminated or states[2]["__all__"]
        truncated = truncated or states[3]["__all__"]

        if terminated or truncated:
            break
        agent = first(states[0])
        observation, reward, terminated, truncated, info = (state[agent] for state in states)

    # Get the termination time if terminated
    if terminated:
        agent = first(states[0])
        observation, reward, terminated, truncated, info = (state[agent] for state in states)
        time_running += float(info["terminateTime"])
        logger.info("Average time running: %s", info["terminateTime"])
    else:
        # Current simulation time is 100 seconds, should be set automatically
        time_running += float(100)

    env.close()


def create_env(context: Any, env_name: str, ns3_settings: dict[str, Any]) -> Ns3MultiAgentEnv:
    return Ns3MultiAgentEnv(
        targetName=env_name,
        ns3Path=NS3_HOME,
        ns3Settings=ns3_settings | {"parallel": context.worker_index},
        trial_name=f"training{context.worker_index}_{context.vector_index}",
    )


def create_example_training_config(
    env_name: str,
    max_episode_steps: int,
    training_params: dict[str, Any],
    rollout_fragment_length: int,
    trainable: str = "PPO",
    **ns3_settings: Any,
) -> AlgorithmConfig:
    """!Create an example algorithm config for use with multiagent training."""
    logger.info("max_episode_steps %s not supported for multi-agent!", max_episode_steps)
    ns3_settings.pop("visualize", None)

    env = Ns3MultiAgentEnv(targetName=env_name, ns3Path=NS3_HOME, ns3Settings=ns3_settings.copy(), trial_name="init")
    env.close()

    register_env("defiance", partial(create_env, env_name=env_name, ns3_settings=ns3_settings.copy()))

    if "policy" in training_params and training_params["policy"] == "shared":
        logger.info("started training with shared Policy")
        policies = {
            "shared_policy": PolicySpec(
                observation_space=env.observation_space["agent_0"], action_space=env.action_space["agent_0"]
            )
        }

        def policy_mapping_fn(agent_id: str, *_args: Any, **_kwargs: Any) -> str:  # noqa: ARG001
            return "shared_policy"
    else:
        logger.info("started training with individual Policy")
        policies = {
            agent_id: PolicySpec(
                observation_space=env.observation_space[agent_id], action_space=env.action_space[agent_id]
            )
            for agent_id in env.observation_space
        }

        def policy_mapping_fn(agent_id: str, *_args: Any, **_kwargs: Any) -> str:
            return agent_id

    match trainable:
        case "PPO":
            config: AlgorithmConfig = PPOConfig()
        case "DQN":
            config = DQNConfig()
        case _:
            msg = f"trainable {trainable} not supported, use PPO or DQN instead!"
            raise ValueError(msg)
    return (
        config.training()
        .callbacks(DefianceCallbacks)
        .resources(num_gpus=0)
        .framework("tf")
        .rollouts(
            num_envs_per_worker=1,
            num_rollout_workers=ns3_settings["parallel"],
            create_env_on_local_worker=False,
            rollout_fragment_length=rollout_fragment_length or "auto",
        )
        .multi_agent(policies=policies, policy_mapping_fn=policy_mapping_fn)
        .reporting(metrics_num_episodes_for_smoothing=1)
        .environment(env="defiance", env_config={"num_agents": len(env.observation_space.keys()), **ns3_settings})
    )


def start_training(
    iterations: int,
    config: AlgorithmConfig,
    trainable: str = "PPO",
    load_checkpoint_path: str | None = None,
    wandb_logger: WandbLoggerCallback | None = None,
) -> None:
    """!Start a ray training session with the given multiagent algorithm config."""
    try:
        ray.init(num_gpus=0)

        if load_checkpoint_path:
            tuner = Tuner.restore(load_checkpoint_path, trainable, param_space=config.to_dict())
            logger.info("Checkpoint restored!")
        else:
            tuner = Tuner(
                trainable,
                run_config=RunConfig(
                    stop={"training_iteration": iterations},
                    checkpoint_config=CheckpointConfig(
                        checkpoint_frequency=1,
                        checkpoint_at_end=True,
                    ),
                    callbacks=[wandb_logger] if wandb_logger else [],
                ),
                param_space=config.to_dict(),
            )

        logger.info("Training...")
        result = tuner.fit()

        logger.info("Training done!")
        (Path(result.experiment_path) / "best_checkpoint").mkdir(exist_ok=True, parents=True)

        res = result.get_best_result(metric="episode_reward_mean", mode="max")
        res.get_best_checkpoint(metric="episode_reward_mean", mode="max").to_directory(
            result.experiment_path + "/best_checkpoint"
        )
        logger.info("Best checkpoint saved at: %s", result.experiment_path)

        ray.shutdown()

    except Exception:
        logger.exception("Exception occurred!")
