#include <ns3/ipv4.h>
#include <ns3/reward-application.h>

#include <cstdint>

using namespace ns3;

/**
 * \ingroup defiance
 * \brief Child class of RewardApplication that sends rewards based on the throughput of the UE.
 */
class ThroughputRewardApp : public RewardApplication
{
  public:
    ThroughputRewardApp(){};
    ~ThroughputRewardApp() override{};
    static TypeId GetTypeId();
    // Measure arriving packet sizes
    void IncreaseReceivedBytes(Ptr<const Packet> packet, Ptr<Ipv4> ipLayer, uint32_t interface);
    // Compute throughput of the UE in the last calculation interval (bit/s)
    double ComputeThroughput();
    // Send the reward to the agent
    void SendReward();
    void RegisterCallbacks() override;

  private:
    uint32_t m_receivedBytes{0}; // bytes received since the last reward computation
    Time m_calculationInterval{MilliSeconds(1000)};
    Time m_simTime{Seconds(100)};
};
