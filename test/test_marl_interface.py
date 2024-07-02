"""Python part to marl-interface-test. Typical testcases: Start a Ns3MultiAgentEnv and assert communication was successful."""

from collections.abc import Collection, Iterator
from typing import TypeVar

import pytest
from _pytest.mark import ParameterSet
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv

from defiance import NS3_HOME
from defiance.utils import first

NS3AI_TEST_SUITES = ["marl-interface-test", "marl-echo-action-interface-test", "marl-agent-interface-test"]

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
    while not terminated:
        action = env.action_space[agent].sample()
        states = env.step({agent: action})

        assert all(isinstance(state, dict) for state in states)
        agent = only({only(state) for state in states})
        observation, reward, terminated, truncated, info = (only(state.values()) for state in states)
        assert truncated is False


@pytest.mark.parametrize("env", ["marl-echo-action-interface-test", "marl-agent-interface-test"], indirect=True)
def test_echo_interface(env: Ns3MultiAgentEnv) -> None:
    agent, observation = only(env.reset()[0].items())
    terminated = False
    while not terminated:
        action = env.action_space[agent].sample()
        states = env.step({agent: action})
        agent = only({only(state) for state in states})
        observation, reward, terminated, truncated, info = (only(state.values()) for state in states)
        if not terminated:
            assert all(action == observation)
            assert action[0] == reward
