#include "handover-agent-application.h"

#include <ns3/base-test.h>
#include <ns3/lte-helper.h>

#include <cstdint>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HandoverAgentApplication");

HandoverAgentApplication::HandoverAgentApplication()
    : AgentApplication()
{
    m_reward = 0;
}

HandoverAgentApplication::~HandoverAgentApplication()
{
}

TypeId
HandoverAgentApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::HandoverAgentApplication")
            .SetParent<AgentApplication>()
            .SetGroupName("defiance")
            .AddConstructor<HandoverAgentApplication>()
            .AddAttribute("NumUes",
                          "Number of user equipments in the simulation.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&HandoverAgentApplication::m_numUes),
                          MakeUintegerChecker<uint>())
            .AddAttribute("NumBs",
                          "Number of base stations in the simulation.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&HandoverAgentApplication::m_numBs),
                          MakeUintegerChecker<uint>())
            .AddAttribute("StepTime",
                          "Time between each step in the simulation.",
                          UintegerValue(420),
                          MakeUintegerAccessor(&HandoverAgentApplication::m_stepTime),
                          MakeUintegerChecker<uint>());
    return tid;
}

void
HandoverAgentApplication::Setup()
{
    AgentApplication::Setup();

    m_observation = GetResetObservation();
    m_reward = GetResetReward();
}

void
HandoverAgentApplication::OnRecvObs(uint id)
{
    NS_LOG_FUNCTION(this << id);
    m_observation = m_obsDataStruct.GetNewestByID(id)->data;
    if (Simulator::Now() - m_lastInferredActionTime >= MilliSeconds(m_stepTime))
    {
        InferAction();
        m_lastInferredActionTime = Simulator::Now();
    }
}

void
HandoverAgentApplication::OnRecvReward(uint id)
{
    NS_LOG_FUNCTION(this << id);
    auto reward = m_rewardDataStruct.GetNewestByID(id)->data;
    auto rewardValue = DynamicCast<OpenGymBoxContainer<double>>(reward->Get("reward"))->GetValue(0);
    m_reward = rewardValue;
}

void
HandoverAgentApplication::InitiateAction(Ptr<OpenGymDataContainer> action)
{
    NS_LOG_FUNCTION(this);
    auto dictAction = CreateObject<OpenGymDictContainer>();
    dictAction->Add("newCellId", action);
    // wont fix: action should only get to node that the UE is connected to
    SendAction(dictAction);
}

Ptr<OpenGymSpace>
HandoverAgentApplication::GetObservationSpace()
{
    auto dictSpace = CreateObject<OpenGymDictSpace>();
    auto rsrpsSpace = CreateObject<OpenGymBoxSpace>(-1,
                                                    97,
                                                    std::vector<uint32_t>{m_numBs},
                                                    TypeNameGet<int32_t>());
    auto rsrqsSpace = CreateObject<OpenGymBoxSpace>(-1,
                                                    34,
                                                    std::vector<uint32_t>{m_numBs},
                                                    TypeNameGet<int32_t>());
    auto cellIdSpace =
        CreateObject<OpenGymBoxSpace>(0, m_numBs, std::vector<uint32_t>{1}, TypeNameGet<int32_t>());
    auto imsiSpace = CreateObject<OpenGymBoxSpace>(0,
                                                   m_numUes,
                                                   std::vector<uint32_t>{1},
                                                   TypeNameGet<int32_t>());
    dictSpace->Add("rsrps", rsrpsSpace);
    dictSpace->Add("rsrqs", rsrqsSpace);
    dictSpace->Add("cellId", cellIdSpace);
    dictSpace->Add("imsi", imsiSpace);
    return dictSpace;
}

Ptr<OpenGymSpace>
HandoverAgentApplication::GetActionSpace()
{
    std::vector<uint32_t> shape = {1};
    std::string dtype = TypeNameGet<int32_t>();
    Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace>(
        1,
        m_numBs,
        shape,
        dtype); // 0 for no handover, otherwise switch to cell with cellid
    return box;
}

Ptr<OpenGymDictContainer>
HandoverAgentApplication::GetResetObservation() const
{
    auto newObservation = CreateObject<OpenGymDictContainer>();
    auto rsrps = MakeBoxContainer<int32_t>(m_numBs);
    auto rsrqs = MakeBoxContainer<int32_t>(m_numBs);
    for (uint32_t i = 0; i < m_numBs; i++)
    {
        rsrps->AddValue(-1);
        rsrqs->AddValue(-1);
    }

    auto cellId = MakeBoxContainer<int32_t>(1, int32_t(0));
    cellId->AddValue(int32_t(0));
    auto imsi = MakeBoxContainer<int32_t>(1, int32_t(0));
    newObservation->Add("rsrps", rsrps);
    newObservation->Add("rsrqs", rsrqs);
    newObservation->Add("cellId", cellId);
    newObservation->Add("imsi", imsi);
    return newObservation;
}

float
HandoverAgentApplication::GetResetReward()
{
    return 0.0;
}
