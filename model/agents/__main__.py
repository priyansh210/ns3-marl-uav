"""!Program code for the `run-agent` cli program."""

import logging
import sys
from argparse import Action, ArgumentParser, Namespace
from collections.abc import Sequence
from typing import Any

from typing_extensions import assert_never, override

logging.basicConfig(level=logging.DEBUG)


class ParseKwargs(Action):
    """!Parse key=value cli parameters into a dictionary."""

    @override
    def __call__(
        self,
        parser: ArgumentParser,
        namespace: Namespace,
        values: str | Sequence[Any] | None,
        option_string: str | None = None,
    ) -> None:
        if values is None or isinstance(values, str):
            return
        setattr(namespace, self.dest, dict(value.split("=") for value in values))


arg_parser = ArgumentParser("run-agent", description="cli tool to launch ns3 coupled with python agents.")
arg_parser.add_argument(
    "type",
    choices=["debug", "train", "infer", "random"],
    help="which python agent should interact with the environments",
)
arg_parser.add_argument(
    "--env-name", "-n", type=str, default="defiance-5g-gym", help="name of the ns3 environment to start"
)
arg_parser.add_argument(
    "--max-episode-steps", "-s", type=int, default=100, help="number of environment steps per iteration"
)
arg_parser.add_argument("--iterations", "-i", type=int, default=50, help="number of environment cycles")
arg_parser.add_argument(
    "--single",
    "-sg",
    action="store_true",
    default=False,
    help="if enabled, assume a single agent environment instead of a multi-agent environment.",
)
arg_parser.add_argument(
    "--training-params",
    "-p",
    default={},
    action=ParseKwargs,
    nargs="*",
    help="key=value pairs overriding the training parameter",
)
arg_parser.add_argument(
    "--checkpoint-path", "-a", type=str, default=None, help="path to the checkpoint to load before training"
)
arg_parser.add_argument(
    "--ns3-settings",
    "-c",
    default={},
    action=ParseKwargs,
    nargs="*",
    help="key=value pairs specifying the ns3 settings",
)

ns = arg_parser.parse_args()

if "runId" in ns.ns3_settings:
    ns.ns3_settings["runId"] = int(ns.ns3_settings["runId"])

match ns.type:
    case "debug" | "random":
        if not ns.single:
            from .debug import make_debug_env, make_env, start_random_agent
        else:
            from .single.debug import make_debug_env, make_env, start_random_agent

        match ns.type:
            case "debug":
                env = make_debug_env(ns.env_name, ns.max_episode_steps, ns.ns3_settings)
            case "random":
                env = make_env(ns.env_name, ns.max_episode_steps, ns.ns3_settings)
            case _:
                assert_never(ns.type)

        start_random_agent(env, ns.iterations)
    case "train":
        if not ns.single:
            from .ray import start_training
        else:
            from .single.ray import start_training

        start_training(
            ns.env_name, ns.max_episode_steps, ns.iterations, ns.training_params, ns.checkpoint_path, **ns.ns3_settings
        )
    case "infer":
        from .ray import start_inference

        start_inference(ns.env_name, ns.checkpoint_path, **ns.ns3_settings)
    case _:
        assert_never(ns.type)
sys.exit(0)
