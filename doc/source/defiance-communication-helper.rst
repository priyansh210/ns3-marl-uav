.. _defiance-communication-helper:

CommunicationHelper
*******************

The natural extension to the :code:`RlApplicationHelper` is the :code:`CommunicationHelper`. It can work with :code:`RlApplicationContainer`\ s to create communication channels between the applications and configure them accordingly. The CommunicationHelper simplifies this procedure and reduces the risks of bugs.

First, create an instance of :code:`CommunicationHelper` and set the different applications:

..  code-block:: c++
    :linenos:

    CommunicationHelper commHelper = CommunicationHelper();

    commHelper.SetObservationApps(observationApps);
    commHelper.SetAgentApps(agentApps);
    commHelper.SetRewardApps(rewardApps);
    commHelper.SetActionApps(actionApps);
    commHelper.SetIds();

The different :code:`Set` methods expect an object of type :code:`RlApplicationContainer`. See chapter :doc:`ApplicationHelper <defiance-application-helper>` for more information on how to create one. After the helper received all :code:`RlApplicationContainer`\ s, the IDs of these applications
need to be assigned (line 7). The IDs are used to identify the instances of :code:`RlApplication` and are required for the next step.

Once the IDs are assigned, the actual connection can be configured.
This can be done by passing a vector of type :code:`CommunicationPair` to the :code:`CommunicationHelper`.
To create an instance of :code:`CommunicationPair`, the IDs of the two :code:`RlApplication`\ s and a :code:`CommunicationAttributes` object have to be provided.
The :code:`CommunicationAttributes` object describes the type of connection. If no argument is passed, a :code:`SimpleChannelInterface` is created.
To create a socket connection via TCP or UDP, a :code:`SocketCommunicationAttributes` object with :code:`TypeId protocol` set accordingly can be passed as :code:`CommunicationAttributes`.
The following code is a simple example that creates :code:`CommunicationPair`\ s of different types.

..  code-block:: c++
    :linenos:

    // UDP
    CommunicationPair actionCommPair = {
        actionApps.GetId(0),
        agentApps.GetId(0),
        SocketCommunicationAttributes{"7.0.0.2", "1.0.0.2", UdpSocketFactory::GetTypeId()}};

    //TCP
    CommunicationPair observationCommPair = {
            observationApps.GetId(0),
            agentApps.GetId(0),
            SocketCommunicationAttributes{"7.0.0.2", "1.0.0.2", TcpSocketFactory::GetTypeId()}};

    //SIMPLE
    CommunicationPair actionCommPair = {actionApps.GetId(0),
                                                    agentApps.GetId(0),
                                                    {}};


The method :code:`GetId(i)` allows to retrieve the :code:`RlApplicationId` by passing the index :code:`i` to the :code:`RlApplicationContainer` (as used in e.g. line 3–4).
When creating :code:`SocketCommunicationAttributes`, the passed IP addresses have to match the addresses of the node the application is installed on.

Once these :code:`CommunicationPair`\ s are created, collect them in a vector and pass it to :code:`CommunicationHelper::AddCommunication` as a parameter.
Finally, the configuration can be finished by calling :code:`Configure` on the :code:`CommunicationHelper`. Now all channel interfaces are created accordingly, ready for sending and receiving data.
An explanation of the :code:`Configure` method can be found in section `Helper` of the design documentation.
