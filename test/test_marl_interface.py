"""Python part to marl-interface-test. Typical testcases: Start a Ns3MultiAgentEnv and assert communication was successful."""

from collections.abc import Collection, Iterator
from typing import TypeVar

import pytest
from _pytest.mark import ParameterSet
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv

from defiance import NS3_HOME
from defiance.utils import first

NS3AI_TEST_SUITES = ["marl-interface", "marl-echo-action-interface", "marl-agent-interface"]

T = TypeVar("T")


def only(it: Collection[T]) -> T:
    assert len(it) == 1
    return first(it)


@pytest.fixture()
def env(request: ParameterSet) -> Iterator[Ns3MultiAgentEnv]:
    env = Ns3MultiAgentEnv(
        "test-runner",
        NS3_HOME,
        ns3Settings={
            "test-name": request.param,
            "stop-on-failure": None,
            "fullness": "QUICK",
            "verbose": None,
        },
    )
    yield env
    env.close()
    del env


@pytest.mark.parametrize("env", NS3AI_TEST_SUITES, indirect=True)
def test_ai_interface(env: Ns3MultiAgentEnv) -> None:
    states = env.reset()
    assert all(isinstance(state, dict) for state in states)
    agent = only({only(state) for state in states})
    terminated = False
    truncated = False
    while not terminated:
        assert truncated is False
        action = env.action_space[agent].sample()
        states = env.step({agent: action})
        assert all(isinstance(state, dict) for state in states)
        terminated, truncated = (state["__all__"] for state in states[2:4])
        terminated = terminated or truncated
        if not (terminated or truncated):
            agent = only({only(state) for state in states[:2]})
            observation, reward = (only(state.values()) for state in states[0:2])


@pytest.mark.parametrize("env", ["marl-echo-action-interface", "marl-agent-interface"], indirect=True)
def test_echo_interface(env: Ns3MultiAgentEnv) -> None:
    agent, observation = only(env.reset()[0].items())
    terminated = False
    while not terminated:
        action = env.action_space[agent].sample()
        states = env.step({agent: action})

        terminated, truncated = (state["__all__"] for state in states[2:4])
        terminated = terminated or truncated

        if not (terminated or truncated):
            agent = only({only(state) for state in states[:2]})
            observation, reward = (only(state.values()) for state in states[0:2])

        if not terminated:
            assert all(action == observation)
            assert action[0] == reward
