#include "handover-reward-application.h"

#include <ns3/base-test.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/lte-net-device.h>
#include <ns3/lte-rlc-sap.h>
#include <ns3/lte-rlc.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-ue-rrc.h>

#include <string>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(ThroughputRewardApp);

TypeId
ThroughputRewardApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ThroughputRewardApp")
            .SetParent<RewardApplication>()
            .AddConstructor<ThroughputRewardApp>()
            .AddAttribute("SimulationTime",
                          "The total simulation time",
                          TimeValue(Seconds(100)),
                          MakeTimeAccessor(&ThroughputRewardApp::m_simTime),
                          MakeTimeChecker())
            .AddAttribute("StepTime",
                          "Interval for calculation reward",
                          TimeValue(MilliSeconds(1000)),
                          MakeTimeAccessor(&ThroughputRewardApp::m_calculationInterval),
                          MakeTimeChecker());
    return tid;
}

void
ThroughputRewardApp::IncreaseReceivedBytes(Ptr<const Packet> packet,
                                           Ptr<Ipv4> ipLayer,
                                           uint32_t interface)
{
    auto bytes = packet->GetSize();
    m_receivedBytes += bytes;
}

double
ThroughputRewardApp::ComputeThroughput()
{
    auto throughput = m_receivedBytes * 8 / m_calculationInterval.GetSeconds(); // in Bit/s
    m_receivedBytes = 0;
    return throughput;
}

void
ThroughputRewardApp::SendReward()
{
    auto throughput = ComputeThroughput();
    // make total reward independent of simulation time and agents step time
    // this ensures that the reward are comparable between simulations with different parameters
    throughput /= m_simTime.GetSeconds();
    throughput *= m_calculationInterval.GetSeconds();
    auto reward = MakeDictBoxContainer<double>(1, "reward", throughput);
    Send(reward);
    Simulator::Schedule(m_calculationInterval, &ThroughputRewardApp::SendReward, this);
}

void
ThroughputRewardApp::RegisterCallbacks()
{
    auto ueNode = GetNode();
    ueNode->GetObject<Ipv4L3Protocol>()->TraceConnectWithoutContext(
        "Rx",
        MakeCallback(&ThroughputRewardApp::IncreaseReceivedBytes, this));
    Simulator::Schedule(Seconds(0), &ThroughputRewardApp::SendReward, this);
}
