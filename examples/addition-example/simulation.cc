#include "addition-agent-app.h"
#include "no-op-action-app.h"
#include "node-id-reward-app.h"
#include "random-observation-app.h"

#include <ns3/communication-helper.h>
#include <ns3/data-rate.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/ipv4-static-routing-helper.h>
#include <ns3/node-container.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/rl-application-helper.h>

using namespace ns3;

int
main(int argc, char* argv[])
{
    auto rewardNodes = NodeContainer(5);
    auto agentNodes = NodeContainer(5);
    auto actionNodes = NodeContainer(5);
    auto observationNodes = NodeContainer(5);

    RlApplicationHelper rlAppHelper(AdditionAgentApp::GetTypeId());
    auto agentApps = rlAppHelper.Install(agentNodes);
    rlAppHelper = RlApplicationHelper(NoOpActionApp::GetTypeId());
    auto actionApps = rlAppHelper.Install(actionNodes);
    rlAppHelper = RlApplicationHelper(NodeListRewardApp::GetTypeId());
    auto rewardApps = rlAppHelper.Install(rewardNodes);
    rlAppHelper = RlApplicationHelper(RandomObservationApp::GetTypeId());
    auto observationApps = rlAppHelper.Install(observationNodes);

    CommunicationHelper commHelper = CommunicationHelper();

    commHelper.SetObservationApps(observationApps);
    commHelper.SetAgentApps(agentApps);
    commHelper.SetRewardApps(rewardApps);
    commHelper.SetActionApps(actionApps);
    commHelper.SetIds();

    std::vector<CommunicationPair> adjacency = {};
    for (uint i = 0; i < agentNodes.GetN(); i++)
    {
        CommunicationPair observationCommPair = {observationApps.GetId(i), agentApps.GetId(i), {}};
        CommunicationPair rewardCommPair = {rewardApps.GetId(i), agentApps.GetId(i), {}};
        CommunicationPair actionCommPair = {actionApps.GetId(i), agentApps.GetId(i), {}};
        adjacency.emplace_back(observationCommPair);
        adjacency.emplace_back(rewardCommPair);
        adjacency.emplace_back(actionCommPair);
    }
    commHelper.AddCommunication(adjacency);
    commHelper.Configure();

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    OpenGymMultiAgentInterface::Get()->NotifySimulationEnd();

    return 0;
}
