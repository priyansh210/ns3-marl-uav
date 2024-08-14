.. _defiance-rl-applications:

RL-Applications
---------------

RlApplication
*************

All presented applications inherit from :code:`RlApplication`. Therefore, all of them can access the following functionality.

RlApplicationId
===============

A common use case is to identify the :code:`RlApplication` information has been sent to or received from.
To accomplish this, each :code:`RlApplication` has a unique identifer, their :code:`RlApplicationId`.
This ID can be set by passing an :code:`RlApplicationId` to :code:`RlApplication::SetId`.
:code:`RlApplicationId` is a struct consisting of an :code:`uint32_t applicationId` and an :code:`ApplicationType applicationType`.

This introduces four different types of :code:`RlApplication`:
    * :code:`OBSERVATION`
    * :code:`REWARD`
    * :code:`AGENT`
    * :code:`ACTION`

If the provided :code:`CommunicationHelper` is not used, one must set all :code:`RlApplicationId`\ s manually to be able to use inter-application communication.

.. _defiance-agent-application:

AgentApplication
****************

The :code:`AgentApplication` is where inference is performed. This code
is an example of a possible implementation followed by an explanation.


..  code-block:: c++
    :linenos:

    class InferenceAgentApp : public AgentApplication
    {
    public:
        InferenceAgentApp()
            : AgentApplication()
        {
        };

        ~InferenceAgentApp() override{};

        static TypeId GetTypeId()
        {
            static TypeId tid = TypeId("ns3::InferenceAgentApp")
                                    .SetParent<AgentApplication>()
                                    .SetGroupName("defiance")
                                    .AddConstructor<InferenceAgentApp>();
            return tid;
        }

        void Setup() override
        {
            AgentApplication::Setup();
            m_observation = GetResetObservation();
            m_reward = GetResetReward();
        }

        void OnRecvObs(uint id) override
        {
            m_observation = m_obsDataStruct.GetNewestByID(id)
                       ->data->Get("floatObs")
                       ->GetObject<OpenGymBoxContainer<float>>();
            InferAction();
        }

        void OnRecvReward(uint id) override
        {
            m_reward = m_rewardDataStruct.GetNewestByID(0)
                       ->data->Get("reward")
                       ->GetObject<OpenGymBoxContainer<float>>()
                       ->GetValue(0);
        }

        Ptr<OpenGymDataContainer> GetResetObservation()
        {
            // This method returns the initial observation that is used after resetting the environment.
            uint32_t shape = 4;
            std::vector<uint32_t> vShape = {shape};
            auto obj =  CreateObject<OpenGymBoxContainer<float>>(vShape);
            for(auto i = 0; i < shape; i++ ){obj->AddValue(0);}
            return obj;
        }

        float GetResetReward()
        {
            // This method returns the initial reward that is used after resetting the environment.
            return 0.0;
        }

    private:
        Ptr<OpenGymSpace> GetObservationSpace() override
        {
            uint32_t shape = 4;
            std::vector<uint32_t> vShape = {shape};
            std::string dtype = TypeNameGet<float>();

            std::vector<float> low = {-4.8 * 2, -INFINITY, -0.418 * 2, -INFINITY};
            std::vector<float> high = {4.8 * 2, INFINITY, 0.418 * 2, INFINITY};

            return CreateObject<OpenGymBoxSpace>(low, high, vShape, dtype);
        }

        Ptr<OpenGymSpace> GetActionSpace() override
        {
            return MakeBoxSpace<int>(1, 0, 1);
        }
    };

To implement your own :code:`AgentApplication` it is necessary to inherit from :code:`AgentApplication` in order
to access all features provided by our framework. This can be seen in line 1.
The method :code:`InferenceAgentApp::GetTypeId` (lines 11-18) is mandatory, as it is part of the *ns-3* library. Since our
classes inherit from :code:`ns3::Object` one has to provide this method to allow the usage of *ns-3*-factories and *ns-3*-pointers.

:code:`InferenceAgentApp::Setup` is called at the beginning of the scenario and ensures that all required variables for inference
are initialized. It is adviced to call the parent method (line 22) since it informs the MARL interface about the action and observation-space
provided by :code:`InferenceAgentApp::GetObservationSpace` and :code:`InferenceAgentApp::GetActionSpace`.
Aditionally the :code:`InferenceAgentApp::Setup` method can be used to initialize :code:`m_observation` and :code:`m_reward` since
this method should always be called before the first occurence of inference and thereby guarantees that no uninitialized variables will be used for inference.

:code:`m_observation` and :code:`m_reward` are two inherited variables from the :code:`AgentApplication` class.
:code:`m_observation` is an :code:`OpenGymDataContainer` that stores the observations used for inference.
:code:`m_reward` is simply a float value representing the current reward. Both of this variables are passed to the MARL interface when :code:`AgentApplication::InferAction` is called (line 32).

:code:`InferenceAgentApp::OnRecvObs` and :code:`InferenceAgentApp::OnRecvReward` are called when the :code:`AgentApplication` receives
an observation or reward. The :code:`id` is the ID of the :code:`RlApplication` that sent the data. It can be used to retrieve the desired data from :code:`m_obsDataStruct` or :code:`m_rewardDataStruct` by calling :code:`HistoryContainer::GetNewestByID(id)`
(line 29). However, there is no restriction on how to update :code:`m_observation` or whether :code:`InferenceAgentApp::OnRecvObs` should be used at all.

Both of the data structures :code:`m_obsDataStruct` and :code:`m_rewardDataStruct` are instances of type :code:`HistoryContainer`. Once a reward
or observation is received, the :code:`AgentApplication` ensures both are updated accordingly before calling
:code:`InferenceAgentApp::OnRecvObs` or :code:`InferenceAgentApp::OnRecvReward`.

In line 32, the method :code:`AgentApplication::InferAction` is called. As mentioned earlier passes this method all required parameters for inference to the MARL interface. Aditionally, a callback
is passed on that sends the returned action from the Python side to an :code:`ActionApplication`. Pass the :code:`RlApplicationId::applicationId`
to :code:`AgentApplication::InferAction` as in :code:`InferAction(id)` if the received action should only be send to an specific :code:`ActionApplication`.
Otherwise the action will be sent to all registered instances. It is not required to call this method in
:code:`InferenceAgentApp::OnRecvObs`. For example :code:`AgentApplication::InferAction` could also be called in a method that is scheduled at equally spaced timesteps
or after an *ns-3*-event. If preferred, it is even possible to call inference outside of the :code:`ActionApplication`. Since :code:`AgentApplication::InferAction` is by design protected within :code:`AgentApplication`, this would require the
usage of :code:`OpenGymMultiAgentInterface::NotifyCurrentState` and thus thorough testing.

:code:`InferenceAgentApp::OnRecvReward` is similar to :code:`InferenceAgentApp::OnRecvObs` in terms of when it is called and its purpose. Both of these
methods allow to aggregate over the data received by multiple :code:`RlApplication` instances. One example could calculate the min of all rewards sent
by :code:`RewardApplication` in the method :code:`InferenceAgentApp::OnRecvReward`.

:code:`InferenceAgentApp::GetResetObservation` and :code:`InferenceAgentApp::GetResetReward` are vital after a reset of the environment. Therefore, they must be implemented in the inheriting class. When setting up a scenario without training a Ray agent and no resets, they are optional, yet it is essential to initialize :code:`m_observation` and :code:`m_reward` at the beginning of the scenario (e.g. ll.23-24).

The last two important methods are :code:`InferenceAgentApp::GetObservationSpace` and :code:`InferenceAgentApp::GetActionSpace`. These methods are mandatory
because they inform the MARL interface about the dimensions of the respective spaces. Information about the different spaces have to
be provided in instances of :code:`OpenGymSpace`. An exemplary creation of such spaces can be seen in line 62 to 69. These spaces as well as the
:code:`OpenGymDataContainer` are part of the *ns-3*-ai library. To reduce the overhead of creating an :code:`OpenGymSpace` or :code:`OpenGymDataContainer`,
some useful functions are provided in :code:`base-test.h`. An example usage of one of these functions can be seen in line 74.

Additional Features and Use-Cases
=================================

Configure History Containers
############################

The length of :code:`m_rewardDataStruct` and :code:`m_obsDataStruct` can be changed by setting the attribute :code:`MaxRewardHistoryLength`
or :code:`MaxObservationHistoryLength`.

It is also possible to save a timestamp, marking the time of arrival in :code:`m_rewardDataStruct` and :code:`m_obsDataStruct`.
If this feature is required, set :code:`ObservationTimestamping` or :code:`RewardTimestamping` to true.
More information is given in `Data History Container`_.

Provide Extra Info
##################

To pass extra info to the environment, override the method :code:`AgentApplication::GetExtraInfo`

..  code-block:: c++

    /* ... */
    private:
        std::string m_importantMessage;
        std::map<std::string, std::string> GetExtraInfo() override
        {
            std::map<std::string, std::string> info;
            info["agent"] = m_importantMessage;
            return info;
        }

Action Delay
############

To simulate the time required to calculate inference, a delay can be set between receiving an action and performing
the callback specified for action execution.

..  code-block:: c++

    /* ... */
    private:
        std::string m_importantMessage;
        Time GetActionDelay() override
        {
            return Seconds(1);
        }

Override initiateAction and initiateActionForApp
################################################

After inference took place, either of these methods is invoked with the returned action from the MARL interface.
This method then sends the received message to either all registered :code:`ActionApplication` or the one that matches :code:`remoteAppId`.
Overriding this method allows for example to only send over a specific :code:`ChannelInterface`.

..  code-block:: c++

    /* in your AgentApplication-class: */
    protected:
        uint32_t interfaceToUse;
        void InitiateActionForApp(uint remoteAppId, Ptr<OpenGymDataContainer> action)
        {
            SendAction(MakeDictContainer("default", action), remoteAppId, interfaceToUse);
        }


OnRecvFromAgent
###############

To specify how an :code:`AgentApplication` should handle messages from another :code:`AgentApplication`, override this method.

The method receives a :code:`remoteAppId` matching the :code:`RlApplicationId::applicationId` of the :code:`AgentApplication` that send the data and
the message itself as a :code:`Ptr<OpenGymDictContainer>`. Here is an example for this:

..  code-block:: c++

    /* in your AgentApplication-class: */
    protected:
        uint32_t agentOfInterest;
        void OnRecvFromAgent(uint remoteAppId, Ptr<OpenGymDictContainer> payload)
        {
            if(remoteAppId == agentOfInterest)
            {
                message = payload->Get("parameter")
                       ->GetObject<OpenGymBoxContainer<float>>()
                       ->GetValue(0);
            }
        }

.. _defiance-observation-application:

If desired, a new :code:`HistoryContainer` can be added to the class which can be used to store and retrieve the received agent messages in a similar fashion as the observations and rewards.

ObservationApplication
**********************

The main purpose of the :code:`ObservationApplication` is to send observations to the agent.
Therefore, the class is equipped with the methods :code:`ObservationApplication::RegisterCallbacks` and :code:`ObservationApplication::Send`.
To implement an :code:`ObservationApplication`, create a child class that inherits from
:code:`ObservationApplication`. This also requires overriding :code:`GetTypeId` in a similar fashion as seen earlier in the :code:`AgentApplication` example.

ObservationApplication::RegisterCallbacks
=========================================

This method allows registration of callbacks to trace sources. This ensures the :code:`ObservationApplication`
is always informed when a value that should be observed changes.

..  code-block:: c++

    class YourImplementation : public ObservationApplication{
      public:
        /* ... */
        void
        RegisterCallbacks() override
        {
            DynamicCast<YourNode>(GetNode())->m_reportYourTrace.ConnectWithoutContext(
                MakeCallback(&YourImplementation::Observe, this));
        }
        void Observe(/*values provided by the trace source*/)
        {
            /* send observation or wait for more observation */
        }
    }

.. note::
    It can be tricky to access the required trace source inside the :code:`ObservationApplication` class, especially if the trace source is not provided by *ns-3*. In this example,
    the costume trace source is accessed by inheriting the :code:`Node` class and adding the
    trace source as a class member. All :code:`ns3::Application` instances can access the node they are
    installed on with :code:`GetNode`. Alternatively, trace sources can be accessed by a
    *ns-3* path. Look into the *ns-3* documentation for more information.

ObservationApplication::Send
============================
Once the :code:`ObservationApplication` is satisfied with the observations, it can send these observations to registered instances of :code:`AgentApplication`.
This functionality is offered by the base class. The observations have to be wrapped into an :code:`OpenGymDictContainer`. If an observation
should only be sent to a specific agent, pass the :code:`RlApplicationId` to :code:`ObservationApplication::Send`. Furthermore, the ID of the :code:`ChannelInterface`
can be provided. If not provided, the observation is sent to all registered instances.

..  code-block:: c++

    class YourImplementation : public ObservationApplication{
      public:
        /* ... */
        void Observe(uint32_t value)
        {
            /*create OpenGymDataContainer */

            Send(/*OpenGymDictContainer*/);
            // or
            Send(/*OpenGymDictContainer*/, remoteId, interfaceId);
        }
    }

.. _defiance-reward-application:

RewardApplication
*****************
The :code:`RewardApplication` is in its functionality similar to :code:`ObservationApplication` since both classes inherit from the same base class.
A reward should be sent to an instance of :code:`AgentApplication` once a relevant event is triggered. To
accomplish that the :code:`RewardApplication::Send` is provided. It is required to wrap all reward information into an :code:`OpenGymDictContainer`.

..  code-block:: c++

    class YourImplementation : public RewardApplication{
      public:
        /* ... */
        void
        RegisterCallbacks() override
        {
            DynamicCast<YourNode>(GetNode())->m_reportYourTrace.ConnectWithoutContext(
                MakeCallback(&YourImplementation::ObserveReward, this));
        }
        void ObserveReward(/*values provided by the trace source*/)
        {
            /*create OpenGymDataContainer */

            Send(/*OpenGymDictContainer*/);
            // or
            Send(/*OpenGymDictContainer*/, remoteID, interfaceId);
        }
    }


.. _defiance-action-application:

ActionApplication
*****************
The :code:`ActionApplication` receives actions and executes them. Therefore, upon receiving an action from an :code:`AgentApplication`, the virtual method :code:`ActionApplication::ExecuteAction` is triggered.
To specify what action should be performed, override the :code:`ActionApplication::ExecuteAction` in a child class.

ActionApplication::ExecuteAction
================================

In this method, two parameters are accessible. :code:`remoteAppId` is the :code:`RlApplicationId` of the :code:`AgentApplication` that sent
the action. :code:`action` is an :code:`OpenGymDictContainer` that contains the sent action out of the action space.
An exemplary retrieval of the actual content of :code:`action` is provided in line 19. :code:`action->Get("default")` returns an
:code:`OpenGymDataContainer`. Therefore, it is necessary to dynamically cast this :code:`OpenGymDataContainer` to the type that was sent by the :code:`AgentApplication` (e.g. with :code:`GetObject<OpenGymBoxContainer<int>>()`). If the content of :code:`action` at key: :code:`"default"` doesn't
match the type passed to :code:`GetObject`, a null pointer will be returned even if its only a mismatch in the provided data type
for :code:`OpenGymBoxContainer`.

.. note::
    Make sure that the :code:`OpenGymDictContainer action` actually contains the key passed by :code:`action->Get("default")`.
    The :code:`AgentApplication::InitiateAction` will always wrap the received action from the MARL interface into an :code:`OpenGymDictContainer`
    with the key :code:`"default"`. However, if this method was overridden in a child class, a different key is possible.

..  code-block:: c++
    :linenos:

    class YourActionApp : public ActionApplication
    {
    public:
        YourActionApp(){};
        ~YourActionApp() override{};

        static TypeId GetTypeId()
        {
            static TypeId tid = TypeId("ns3::YourActionApp")
                                    .SetParent<ActionApplication>()
                                    .SetGroupName("defiance")
                                    .AddConstructor<YourActionApp>();
            return tid;
        }

        void ExecuteAction(uint32_t remoteAppId, Ptr<OpenGymDictContainer> action) override
        {
            // auto m_objectActionIsPerformedOn = DynamicCast<objectActionIsPerformedOn>(GetNode());
            auto act = action->Get("default")->GetObject<OpenGymBoxContainer<int>>()->GetValue(0);

            m_objectActionIsPerformedOn->SetValue(acc);
        }

        void SetObservationApp(Ptr<ActionObject> object)
        {
            m_objectActionIsPerformedOn = object;
        }

    private:
        Ptr<ActionObject> objectActionIsPerformedOn;
    };

To perform the action, the :code:`ActionApplication` needs a reference to the object it perfoms the action on.
One solution would be to pass it to the application as seen in line 24-27. Alternatively, the :code:`ActionApplication` could access the node it is installed on.


Communication between RL-Applications
*************************************

Add interfaces
==============

To properly use the RL applications, connect them to one another via the `ChannelInterface`_.
The :code:`RlApplication` interface provides the method :code:`RlApplication::AddInterface` to register a :code:`ChannelInterface`.
Two applications can be connected over multiple instances of :code:`ChannelInterface`, enabling potential multipath functionality. To index the different :code:`ChannelInterface` between
two applications, an :code:`interfaceId` has to be provided. :code:`RlApplicationId` in combination with the :code:`interfaceId` represents an
unique identifer for a connection between two instances of :code:`RlApplication`.

:code:`AddInterface` also sets up necessary callbacks for receiving messages.

..  code-block:: c++
    :linenos:
    :emphasize-lines: 13,17

    //code to create your agent
    RlApplicationHelper helper(TypeId::LookupByName("ns3::YourAgentClass"));
    helper.SetAttribute("StartTime", TimeValue(Seconds(0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(10)));
    RlApplicationContainer agentApps = helper.Install(agentNode);

    //code to create your observationApp
    helper.SetTypeId("ns3::YourObservationApp");
    RlApplicationContainer observationApps = helper.Install(obsNode);

    RlApplicationId remoteIdObservationApp = DynamicCast<YourObservationApp>(observationApps.Get(0))->GetId();
    Ptr<YourAgentClass> agent = DynamicCast<YourAgentClass>(agentApps.Get(0));
    uint interfaceAtAgentId = agent->AddInterface(remoteIdObservationApp, ptrToChannelInterface);

    RlApplicationId remoteAgentId = DynamicCast<YourAgentClass>(agentApps.Get(0))->GetId();
    Ptr<YourObservationApp> obsApp = DynamicCast<YourObservationApp>(observationApps.Get(0));
    uint interfaceAtObservationId = obsApp->AddInterface(remoteAgentId, ptrToChannelInterface);

Note that the functionality of this method is only provided for foreseen connections of the framework. For example it is necessary that
an :code:`AgentApplication` can exchange data with all other types of :code:`RlApplication`\ s. Therefore the call of :code:`AgentApplication::AddInterface`
will succeed as long as the provided :code:`RlApplicationId::ApplicationType` matches any of the following:

    * :code:`OBSERVATION`
    * :code:`REWARD`
    * :code:`AGENT`
    * :code:`ACTION`

However, if one tries to add a :code:`ChannelInterface` to an :code:`ObservationApplication` that is connected to another :code:`ObservationApplication`, the method
would result in an error because the exchange between two :code:`ObservationApplication` is deliberately excluded in the design of *ns3-defiance*.

When adding the :code:`ChannelInterface`, the application can derive the :code:`ApplicationType` from the :code:`RlApplicationId`.
This allows the application to properly handle the connection.

After registering the :code:`ChannelInterface`, the :code:`RlApplication` is ready to send.

Send
====

Call this method to send data over a registered :code:`ChannelInterface`. Note that the different :code:`RlApplication`\ s often wrap
the :code:`RlApplication::Send` for general use cases. Therefore, refrain from using :code:`RlApplication::Send` and use the respective appropiate method offered by each application instead. These methods
often ensure additional necessary prerequisites for proper communication (e.g. registering callbacks).

Even though these wrapped methods differ in their functionality they are all called in a similar manner.
There are always 3 arguments: :code:`Ptr<OpenGymDictContainer> data, uint32_t appId, uint32_t interfaceIndex`.
The first argument is required - the data that is supposed to be sent. The second argument is the
:code:`appId`. If provided, the data will only be sent to the :code:`RlApplication` that has a matching :code:`RlApplicationId::applicationId`.
The third argument the :code:`interfaceIndex` can be specified alongside the :code:`RlApplicationId::applicationId`. This ensures that only a specific
:code:`ChannelInterface` is used. The index of an interface is returned by the :code:`AddInterface` method.
If the :code:`interfaceIndex` is not set, all interfaces between the two applications are used.
Similarly, if the :code:`appId` is not set the data is sent to all registered applications of that type over all interfaces.

..  code-block:: c++

    // method to send actions from agent to action app
    uint interfaceIdActionApp = agentApp->AddInterface(remoteActionId, ptrToChannelInterface);

    Ptr<OpenGymDictContainer> action = /* create DictContainer */

    //send to all
    SendAction(action);
    //send to specific application
    SendAction(action, remoteActionId);
    //send to specific application over specific channelInterface
    SendAction(action, remoteActionId, interfaceIdActionApp);


AgentApplication Communication
==============================

The :code:`AgentApplication` can communicate with applications of any :code:`RlApplicationId::ApplicationType`:

    * :code:`OBSERVATION`
    * :code:`REWARD`
    * :code:`AGENT`
    * :code:`ACTION`

See `Add interfaces`_ for more information on how to set it up.

To fulfill its functionality, the :code:`AgentApplication` is equipped with two methods - :code:`SendAction` and :code:`SendToAgent`.
They are invoked as described in `Send`_. :code:`SendAction` only sends to applications of type :code:`ACTION`, while
:code:`SendToAgent` only sends to applications of type :code:`AGENT`.

RewardApplication Communication and ObservationApplication Communication
========================================================================

Both applications only allow communication to applications of type :code:`AGENT`. See `Add interfaces`_ on how to add interfaces.

The interface of :code:`RewardApplication` and :code:`ObservationApplication` offers a :code:`Send` method (through their parent class :code:`DataCollectorApplication`)
that works as described in `Send`_. The passed data should be used by the agent to determine the reward or update its observation.

ActionApplication Communication
===============================
The :code:`ActionApplication` only allows applications of type :code:`AGENT` to be added. See `Add interfaces`_ on how to add interfaces.

It doesn't wrap the :code:`Send` method because it is not supposed to send, but only receive.
