#include "base-test.h"

#include "simple-channel-interface.h"

namespace ns3
{

Ptr<OpenGymDictContainer>
MakeDictContainer(std::string key, Ptr<OpenGymDataContainer> data)
{
    auto dictContainer = CreateObject<OpenGymDictContainer>();
    dictContainer->Add(key, data);
    return dictContainer;
}

Callback<void, Ptr<OpenGymDataContainer>> noopCallback =
    MakeCallback(*[](Ptr<OpenGymDataContainer>) {});

RlAppBaseTestCase::RlAppBaseTestCase(std::string name)
    : TestCase(name)
{
    m_receivedData = {0.0, 0.0, 0.0};
}

RlAppBaseTestCase::~RlAppBaseTestCase()
{
}

void
RlAppBaseTestCase::ReceiveData(int id, Ptr<OpenGymDictContainer> data)
{
    auto boxContainer = data->Get("floatObs")->GetObject<OpenGymBoxContainer<float>>();
    m_receivedData[id] = boxContainer->GetValue(0);
}

void
RlAppBaseTestCase::CreateInterfaces()
{
    auto sendingChannelInterface = CreateObject<SimpleChannelInterface>();
    auto receivingChannelInterface = CreateObject<SimpleChannelInterface>();
    m_sendingInterfaces.emplace_back(sendingChannelInterface);
    m_receivingInterfaces.emplace_back(receivingChannelInterface);
}

void
RlAppBaseTestCase::ScenarioSetup()
{
    // Create Nodes
    m_node = CreateObject<Node>();
    auto receivingNode = CreateObject<Node>();
    NodeContainer nodes;
    nodes.Add(m_node);
    nodes.Add(receivingNode);

    for (int i = 0; i < 3; i++)
    {
        CreateInterfaces();

        // Register Callback which receives data
        m_receivingInterfaces[i]->AddRecvCallback(
            MakeCallback(&RlAppBaseTestCase::ReceiveData, this, i));

        // Connect ChannelInterfaces
        (DynamicCast<SimpleChannelInterface>(m_sendingInterfaces[i]))
            ->Connect(DynamicCast<SimpleChannelInterface>(m_receivingInterfaces[i]));
    }
}

void
RlAppBaseTestCase::DoRun()
{
    ScenarioSetup();
    Simulate();
}

NS_OBJECT_ENSURE_REGISTERED(TestObservationApp);

TypeId
TestObservationApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TestObservationApp")
                            .SetParent<ObservationApplication>()
                            .SetGroupName("defiance")
                            .AddConstructor<TestObservationApp>();
    return tid;
}

TestObservationApp::TestObservationApp()
{
}

TestObservationApp::~TestObservationApp()
{
}

void
TestObservationApp::ExecuteCallback(float arg, int arg2, int arg3)
{
    m_callback(arg, arg2, arg3);
}

void
TestObservationApp::Observe(float observation, int appId, int interfaceIndex)
{
    if (appId < 0)
    {
        Send(MakeDictBoxContainer<float>(1, "floatObs", observation));
    }
    else if (interfaceIndex < 0)
    {
        Send(MakeDictBoxContainer<float>(1, "floatObs", observation), appId);
    }
    else
    {
        Send(MakeDictBoxContainer<float>(1, "floatObs", observation), appId, interfaceIndex);
    }
}

void
TestObservationApp::RegisterCallbacks()
{
    m_callback = MakeCallback(&TestObservationApp::Observe, this);
}

NS_OBJECT_ENSURE_REGISTERED(TestAgentApp);

TypeId
TestAgentApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TestAgentApp")
                            .SetParent<AgentApplication>()
                            .SetGroupName("defiance")
                            .AddConstructor<TestAgentApp>();
    return tid;
}

std::map<uint, double>
TestAgentApp::GetLatest() const
{
    return m_latest;
}

void
TestAgentApp::OnRecvObs(uint remoteAppId)
{
    m_latest[remoteAppId] = m_obsDataStruct.AggregateNewest(remoteAppId, 1)["floatObs"].GetAvg();
}

void
TestAgentApp::OnRecvReward(uint remoteAppId)
{
}

Ptr<OpenGymSpace>
TestAgentApp::GetObservationSpace()
{
    return {};
};

Ptr<OpenGymSpace>
TestAgentApp::GetActionSpace()
{
    return {};
};

} // namespace ns3
