#!/usr/bin/env python3
"""
Uses the defiance ns3-ai interface to showcase a multi-agent environment for addition.

Alternativly, use `run-agent random -n defiance-addition-example` for a random version.
"""

import logging

from defiance import NS3_HOME
from defiance.utils import first
from ns3ai_gym_env.envs.ns3_multi_agent_environment import Ns3MultiAgentEnv

logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger(__name__)


env = Ns3MultiAgentEnv(targetName="defiance-addition-example", ns3Path=NS3_HOME, ns3Settings={})
iterations = 10

for iteration in range(iterations):
    truncated = False
    terminated = False
    reset = env.reset()
    agent, observation = first(reset[0].items())
    _, info = first(reset[1].items())
    logger.info("initial observation: %s, info: %s", observation, info)
    i = 0
    while not terminated and not truncated:
        i += 1
        action = int(observation + float(agent.split("_")[-1]))
        logger.info("taking action %s", action)
        states = env.step({agent: action})
        logger.info("got states %s", states)
        if states[3]["__all__"]:
            break
        agent = first(states[0])
        logger.debug([state[agent] for state in states])
        observation, reward, terminated, truncated, info = (state[agent] for state in states)
        logger.info(
            "got observation %s, reward %f, terminated %s, truncated %s, info %s",
            observation,
            reward,
            terminated,
            truncated,
            info,
        )
    logger.critical("iteration %i completed in %i total steps.", iteration, i)
