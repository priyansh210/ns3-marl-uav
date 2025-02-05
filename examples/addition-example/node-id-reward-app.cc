#include "node-id-reward-app.h"

#include <ns3/base-test.h>
#include <ns3/node-list.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NodeListRewardApp");

NodeListRewardApp::NodeListRewardApp()
{
    NS_LOG_FUNCTION(this);
}

TypeId
NodeListRewardApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NodeListRewardApp")
                            .SetParent<RewardApplication>()
                            .AddConstructor<NodeListRewardApp>();
    return tid;
}

void
NodeListRewardApp::Observe()
{
    auto node_id = (float)GetNode()->GetId() / NodeList::GetNNodes();
    NS_LOG_INFO("Weighted node id: " << node_id);
    auto observation = MakeBoxContainer<float>(1, node_id);
    Send(MakeDictContainer("reward_hint", observation));
    Simulator::Schedule(Seconds(1.0), &NodeListRewardApp::Observe, this);
}

void
NodeListRewardApp::RegisterCallbacks()
{
    NS_LOG_FUNCTION(this);
    Simulator::Schedule(Seconds(1.1), &NodeListRewardApp::Observe, this);
}

NS_OBJECT_ENSURE_REGISTERED(NodeListRewardApp);

} // namespace ns3
