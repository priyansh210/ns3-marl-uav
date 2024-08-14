Overview
--------

The *ns-3* DEFIANCE module is a reinforcement learning (RL) framework for *ns-3*. It allows the simulation of distributed RL in networks. It can handle single-agent and multi-agent RL scenarios.

The user performs the following steps to carry out the RL experiment:

1. Implement the network topology and traffic using standard *ns-3* code.
2. To define how observations and reward signals are collected, how actions are executed in the environment and how the agents perform inference and training, subclass from the provided :ref:`RL-Applications <defiance-rl-applications>`. Abstract classes for the different subtasks are provided via the :ref:`AgentApplication <defiance-agent-application>`, :ref:`ObservationApplication <defiance-observation-application>`, :ref:`RewardApplication <defiance-reward-application>` and :ref:`ActionApplication <defiance-action-application>`. These applications are installed in the simulation via the :ref:`RlApplicationHelper <defiance-application-helper>`.
3. Specify how data is exchanged between these components and specify the communication structure via channels. The lowest level of abstraction our framework proposes is using :ref:`ChannelInterfaces <defiance-channel-interface>` for this. The framework also provides a :ref:`CommunicationHelper <defiance-communication-helper>` class to simplify the communication setup.
4. Finally, use the utilities provided by :ref:`ns3-ai <ns3-ai>` to interact with the simulation as an RL environment.

.. note::
    An in-depth documentation of the multi-agent interface we added to *ns3-ai* can be found `here <https://github.com/DEFIANCE-project/ns3-ai/blob/main/docs/multi-agent.md>`_. This interface is also used by the DEFIANCE framework. In case you are interested in how the framework functions or think about extending DEFIANCE, we recommend to take a look at these docs or the `blog post <https://medium.com/@oliver.zimmermann/reinforcement-learning-in-ns3-part-1-698b9c30c0cd>` we wrote about it.
