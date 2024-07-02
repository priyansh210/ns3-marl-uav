"""Agent leveraging ray to train an agent for a certain ns3 environment."""

import logging
from collections import defaultdict
from pathlib import Path
from typing import Any

import numpy as np
import ray
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv
from ray.air import CheckpointConfig, RunConfig
from ray.rllib import Policy, RolloutWorker, SampleBatch
from ray.rllib.algorithms import PPOConfig
from ray.rllib.algorithms.callbacks import DefaultCallbacks
from ray.rllib.env import BaseEnv
from ray.rllib.evaluation import Episode
from ray.rllib.policy.policy import PolicySpec
from ray.tune import Tuner, register_env
from typing_extensions import override

from defiance import NS3_HOME
from defiance.test.test_marl_interface import only

logger = logging.getLogger(__name__)


class DefianceCallbacks(DefaultCallbacks):
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
    env = Ns3MultiAgentEnv(targetName=env_name, ns3Path=".", ns3Settings=ns3_settings)
    reset = env.reset()
    agent, observation = only(reset[0].items())
    _, info = only(reset[1].items())
    terminated, truncated = False, False
    time_running = 0.0

    while True:
        if "shared_policy" in policies:
            action = policies["shared_policy"].compute_single_action(observation)[0]
        else:
            # this changes based on policy_mapping
            action = policies[agent].compute_single_action(observation)[0]
        states = env.step({agent: action})

        terminated = terminated or states[2]["__all__"]
        truncated = truncated or states[3]["__all__"]

        if terminated or truncated:
            break
        agent = only(states[0])
        observation, reward, terminated, truncated, info = (state[agent] for state in states)

    # Get the termination time if terminated
    if terminated:
        agent = only(states[0])
        observation, reward, terminated, truncated, info = (state[agent] for state in states)
        time_running += float(info["terminateTime"])
    else:
        # Current simulation time is 5 seconds, should be automatically set
        time_running += float(5)

    env.close()
    logger.info("Average time running: %s", time_running)


def start_training(
    env_name: str,
    max_episode_steps: int,
    iterations: int,
    training_params: dict[str, Any],
    load_checkpoint_path: str | None = None,
    **ns3_settings: str,
) -> None:
    logger.info("max_episode_steps %s not supported for multi-agent!", max_episode_steps)
    ns3_settings.pop("visualize", None)
    env = Ns3MultiAgentEnv(targetName=env_name, ns3Path=NS3_HOME, ns3Settings=ns3_settings)

    register_env("defiance", lambda _: env)
    training_defaults: dict[str, Any] = {
        "gamma": 0.99,
        "lr": 0.0003,
        "num_sgd_iter": 6,
        "vf_loss_coeff": 0.01,
        "use_kl_loss": True,
        "model": {
            "fcnet_hiddens": [32],
            "fcnet_activation": "linear",
            "vf_share_layers": True,
        }
        | training_params,
    }

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

    training_defaults.pop("policy", None)
    try:
        ray.init(num_gpus=0)
        config = (
            PPOConfig()
            .environment(env="defiance", env_config={"num_agents": len(env.observation_space.keys())})
            .training(**training_defaults)
            .callbacks(DefianceCallbacks)
            .resources(num_gpus=0)
            .framework("tf")
            .rollouts(num_envs_per_worker=1, num_rollout_workers=1, create_env_on_local_worker=False)
            .multi_agent(policies=policies, policy_mapping_fn=policy_mapping_fn)
        )

        if load_checkpoint_path:
            tuner = Tuner.restore(load_checkpoint_path, "PPO", param_space=config.to_dict())
            logger.info("Checkpoint restored!")
        else:
            tuner = Tuner(
                "PPO",
                run_config=RunConfig(
                    stop={"training_iteration": iterations},
                    checkpoint_config=CheckpointConfig(
                        checkpoint_frequency=1,
                        checkpoint_at_end=True,
                    ),
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
