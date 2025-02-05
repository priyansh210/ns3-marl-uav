#include "addition-agent-app.h"

#include <ns3/base-test.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("AdditionAgentApp");

AdditionAgentApp::AdditionAgentApp()
    : m_last_action(0)
{
    NS_LOG_FUNCTION(this);
}

TypeId
AdditionAgentApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AdditionAgentApp")
                            .SetParent<AgentApplication>()
                            .AddConstructor<AdditionAgentApp>();
    return tid;
}

void
AdditionAgentApp::OnRecvObs(uint id)
{
    auto obs = m_obsDataStruct.GetNewestByID(id)
                   ->data->Get("random_number")
                   ->GetObject<OpenGymBoxContainer<float>>();
    NS_LOG_INFO("Received observation: " << obs->GetValue(0));
    m_observation = obs;
}

void
AdditionAgentApp::OnRecvReward(uint remoteAppId)
{
    auto reward_hint = m_rewardDataStruct.GetNewestByID(remoteAppId)
                           ->data->Get("reward_hint")
                           ->GetObject<OpenGymBoxContainer<float>>()
                           ->GetValue(0);
    // the agent should have given (observation + reward_hint)
    auto difference =
        std::abs(m_last_action -
                 m_observation->GetObject<OpenGymBoxContainer<float>>()->GetValue(0) - reward_hint);
    m_reward = std::exp(-difference);

    NS_LOG_INFO("Received normalized reward: " << m_reward);

    // as rewards will arrive later than observations infer action now
    InferAction();
}

Ptr<OpenGymSpace>
AdditionAgentApp::GetObservationSpace()
{
    return MakeBoxSpace<float>(1, 0, 1);
}

Ptr<OpenGymSpace>
AdditionAgentApp::GetActionSpace()
{
    return MakeBoxSpace<float>(1, 0, 2);
}

void
AdditionAgentApp::InitiateAction(Ptr<OpenGymDataContainer> action)
{
    m_last_action = action->GetObject<OpenGymBoxContainer<float>>()->GetValue(0);
    SendAction(MakeDictContainer("default", action));
}

NS_OBJECT_ENSURE_REGISTERED(AdditionAgentApp);
} // namespace ns3
