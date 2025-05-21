#include "ns3/aodv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h" // Include FlowMonitor
#include "ns3/internet-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/wifi-module.h"
#include <ns3/action-application.h>
#include <ns3/agent-application.h>
#include <ns3/base-test.h>
#include <ns3/defiance-module.h>
#include <ns3/observation-application.h>
#include <ns3/reward-application.h>

#include <ns3/uav-node.h>
#include <ns3/uav-env-creator.h>

#include <math.h>
#include <string>

using namespace ns3;

NodeContainer edgeNodes;
NodeContainer baseStation;
// Global map storing waypoints and velocity for each UAV
std::map<uint32_t, Vector> uavWaypoints;
std::map<uint32_t, Vector> uavVelocity; // UAV velocity vectors
std::map<uint32_t, std::vector<std::tuple<double, double, double>>>
    uavCostMap; // Stores (x, y, cost) for each UAV
std::vector<Vector> clusterPositions;
std::map<uint32_t, std::vector<uint32_t>> agentClusterCoverage; // Map to store clusters covered by each agent

std::map<uint32_t, std::vector<float>> agentRewardComponents; // Map to store reward components for each agent
int UAV_RANGE = 60;

uint32_t nEdge = 50;
uint32_t nUAV = 5;
uint32_t nClusters = 4;
double MIN_X = 0.0, MAX_X = 500.0;
double MIN_Y = 0.0, MAX_Y = 500.0;
double MIN_Z = 0.0, MAX_Z = 10.0;
double SIM_TIME = 300.0;
double L = 10.0;
double MAX_ANGLE = 15.0;
double MIN_ANGLE = -15.0;
double MAX_VEL = 16;
double MIN_VEL = 5;

uint32_t OBSERVATION_SIZE = 22;
double prevCoverageRatio = 0.0;

int packetLossCount = 0;
double prevPacketLoss = 0.0;

Vector baseStationPosition; // Global variable to store the base station's position
double COVERAGE_RADIUS = 500.0; // Scaling factor for positions

NS_LOG_COMPONENT_DEFINE("WirelessMeshUAV");

struct NetworkData
{
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor;
    uint32_t totalRxPackets; // Total number of received packets in all flows
    uint32_t totalTxPackets;
    uint64_t totalRxBytes;   // Total bytes received in all flows
    double totalDelaySum;    // Total delay sum in all flows
    double packetLossRate; 
    double throughput; 
    double delay; 

    // average delay (ms)
    double averageDelay()
    {
        return totalRxPackets ? totalDelaySum / totalRxPackets / 1000000 : 0;
    }
} uavPktData;

double statInterval = 1; 

void Throughput() {
    uavPktData.monitor->CheckForLostPackets();
    const FlowMonitor::FlowStatsContainer stats = uavPktData.monitor->GetFlowStats();

    uint64_t totalRxBytes = 0;
    uint32_t totalRxPackets = 0;
    uint32_t totalTxPackets = 0;
    double totalDelaySum = 0;

    // Iterating through every flow
    for (FlowMonitor::FlowStatsContainerCI iter = stats.begin(); iter != stats.end(); iter++)
    {
        totalRxBytes += iter->second.rxBytes;
        totalDelaySum += iter->second.delaySum.GetDouble();
        totalRxPackets += iter->second.rxPackets;
        totalTxPackets += iter->second.txPackets;
    }
    uint64_t rxBytesDiff = totalRxBytes - uavPktData.totalRxBytes;
    uint32_t rxPacketsDiff = totalRxPackets - uavPktData.totalRxPackets;
    double delayDiff = totalDelaySum - uavPktData.totalDelaySum;

    uavPktData.totalRxBytes = totalRxBytes;
    uavPktData.totalRxPackets = totalRxPackets;
    uavPktData.totalTxPackets = totalTxPackets;
    uavPktData.totalDelaySum = totalDelaySum;
    uavPktData.throughput = (rxBytesDiff * 8.0 / statInterval / (1024 * 1024)); // Mbps
    uavPktData.packetLossRate = (totalTxPackets > 0 ? 100.0 * (totalTxPackets - totalRxPackets) / totalTxPackets : 0.0); // Percentage
    uavPktData.delay = (rxPacketsDiff > 0 ? delayDiff / rxPacketsDiff / 1000000 : 0.0); // Average delay in ms
    // NS_LOG_UNCOND("Time (s): " << Simulator::Now().GetSeconds() << ", Throughput (Mbps): " << uavPktData.throughput << ", Average Delay (ms): " << uavPktData.delay << ", Packet Loss Rate (%): " << uavPktData.packetLossRate);
    prevPacketLoss = uavPktData.packetLossRate;
    if (prevPacketLoss==100.00){
        packetLossCount++;
    } else {
        packetLossCount = 0;
    }
    Simulator::Schedule(Seconds(statInterval), &Throughput);
}

void
SaveStats(Ptr<OutputStreamWrapper> stats_file,
          uint32_t nodeId,
          UAVState uavState,
          double networkPower,
          double propulsionPower,
          Vector resistanceVector,
          std::vector<UAVState> nearbyUAVStates)
{
    std::ostream* stream = stats_file->GetStream();

    // Write UAV ID, position, velocity, resistance vector, network power, propulsion power
    *stream << Simulator::Now().GetSeconds() << "," // Current simulation time
            << nodeId << "," 
            << uavState.currentPosition.x << "," << uavState.currentPosition.y << "," << uavState.currentPosition.z << ","
            << uavState.velocityVector.x << "," << uavState.velocityVector.y << "," << uavState.velocityVector.z << ","
            << resistanceVector.x << "," << resistanceVector.y << "," << resistanceVector.z << ","
            << networkPower << "," << propulsionPower << ",";

    // Write relative cluster distance vectors in a single string with commas replaced by spaces
    std::ostringstream clusterDistanceStream;
    clusterDistanceStream << "\"[";
    for (size_t i = 0; i < clusterPositions.size(); ++i)
    {
        Vector clusterPosition = clusterPositions[i];
        Vector relativeDistance = clusterPosition - uavState.currentPosition;

        // Formatting the relative distance vector for this cluster
        clusterDistanceStream << "(" << relativeDistance.x << " " << relativeDistance.y << " "
                              << relativeDistance.z << ")";

        // Add a space if it's not the last element
        if (i < clusterPositions.size() - 1)
        {
            clusterDistanceStream << " ";
        }
    }
    clusterDistanceStream << "]\"";
    *stream << clusterDistanceStream.str() << ",";

    // Write nearby UAVs' relative positions and velocities in a single string with commas replaced by spaces
    std::ostringstream nearbyUAVStream;
    nearbyUAVStream << "\"[";
    for (size_t i = 0; i < nearbyUAVStates.size(); ++i)
    {
        Vector relativePosition = nearbyUAVStates[i].currentPosition - uavState.currentPosition;
        Vector relativeVelocity = nearbyUAVStates[i].velocityVector;

        nearbyUAVStream << "(" << relativePosition.x << " " << relativePosition.y << " " << relativePosition.z
                        << " " << relativeVelocity.x << " " << relativeVelocity.y << " " << relativeVelocity.z << ")";

        if (i < nearbyUAVStates.size() - 1)
        {
            nearbyUAVStream << " ";
        }
    }
    nearbyUAVStream << "]\"";
    *stream << nearbyUAVStream.str() << ",";

    // Write packet data (throughput, delay, packet loss rate)
    *stream << uavPktData.throughput << "," 
            << uavPktData.delay << "," 
            << uavPktData.packetLossRate << ",";

    // Write reward components for the UAV
    if (agentRewardComponents.find(nodeId) != agentRewardComponents.end())
    {
        const auto& rewards = agentRewardComponents[nodeId];
        *stream << "\"[";
        for (size_t i = 0; i < rewards.size(); ++i)
        {
            *stream << rewards[i];
            if (i < rewards.size() - 1)
            {
                *stream << " "; // Replace comma with space
            }
        }
        *stream << "]\"";
    }
    else
    {
        *stream << "\"[]\""; // No reward components available
    }

    *stream << std::endl;
    stream->flush();
}


// OBSERVATION

class UavObservationApp : public ObservationApplication
{
  public:
    UavObservationApp(){};
    ~UavObservationApp() override{};
    static TypeId GetTypeId();
    void Observe(uint32_t nodeId,
        UAVState uavState,
        double networkPower,
        double propulsionPower,
        Vector resistanceVector,
        std::vector<UAVState> nearbyUAVStates);
    void RegisterCallbacks() override;
    void SendObservation(double delay);

  private:
    Ptr<OpenGymDataContainer> m_observation = MakeBoxContainer<float>(OBSERVATION_SIZE);
    
};

NS_OBJECT_ENSURE_REGISTERED(UavObservationApp);

TypeId
UavObservationApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavObservationApp")
                            .SetParent<ObservationApplication>()
                            .SetGroupName("defiance")
                            .AddConstructor<UavObservationApp>();
    return tid;
}

void
UavObservationApp::Observe(uint32_t nodeId,
                           UAVState uavState,
                           double networkPower,
                           double propulsionPower,
                           Vector resistanceVector,
                           std::vector<UAVState> nearbyUAVStates)
{
    auto box = MakeBoxContainer<float>(OBSERVATION_SIZE); // Adjust size as needed

    // Calculate relative position to the base station
    Vector relativeToBase = (uavState.currentPosition - baseStationPosition);
    
    // Check the coverage
    std::vector<uint32_t> coveredEdgeIds;
    for (uint32_t i = 0; i < edgeNodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mobility = edgeNodes.Get(i)->GetObject<MobilityModel>();
        Vector edgePosition = mobility->GetPosition();
        double distance = (edgePosition - uavState.currentPosition).GetLength();
        if (distance <= 80.0)
        {
            coveredEdgeIds.push_back(i); // Add the edge node ID instead of the position
        }
    }
    agentClusterCoverage[nodeId] = coveredEdgeIds; // Store edge IDs covered by the UAV

    // Fill UAV's own state
    box->AddValue(static_cast<float>(relativeToBase.x )); // Scaled relative X position
    box->AddValue(static_cast<float>(relativeToBase.y )); // Scaled relative Y position
    box->AddValue(static_cast<float>(uavState.velocityVector.x));
    box->AddValue(static_cast<float>(uavState.velocityVector.y));
    box->AddValue(static_cast<float>(resistanceVector.x));
    box->AddValue(static_cast<float>(resistanceVector.y));
    box->AddValue(static_cast<float>(networkPower));
    box->AddValue(static_cast<float>(propulsionPower));

    // Add the top 5 local centroids covered using clusterPositions
    std::vector<std::pair<double, Vector>> distanceWithVectors;
    for (const auto& clusterPosition : clusterPositions)
    {
        double distance = (clusterPosition - uavState.currentPosition).GetLength();
        distanceWithVectors.push_back({distance, clusterPosition});
    }

    // Sort the distances in ascending order
    std::sort(distanceWithVectors.begin(),
              distanceWithVectors.end(),
              [](const std::pair<double, Vector>& a, const std::pair<double, Vector>& b) {
                  return a.first < b.first;
              });

    // Add the closest 5 clusters to the observation
    size_t topN = std::min(distanceWithVectors.size(), size_t(3));
    for (size_t i = 0; i < topN; ++i)
    {
        Vector scaledCluster = distanceWithVectors[i].second; // Scale by COVERAGE_RADIUS
        box->AddValue(static_cast<float>(scaledCluster.x ));
        box->AddValue(static_cast<float>(scaledCluster.y ));
    }

    // Pad remaining entries if fewer than 5 clusters
    for (size_t i = topN; i < 3; ++i)
    {
        box->AddValue(static_cast<float>(COVERAGE_RADIUS/2)); // Padding value
        box->AddValue(static_cast<float>(COVERAGE_RADIUS/2)); // Padding value
    }

// Add the relative position and velocity of the top 2 closest UAVs
    std::vector<std::tuple<double, Vector, Vector>> nearbyDistances; // Store distance, relative position, and velocity
    for (const auto& nearbyState : nearbyUAVStates)
    {
        double distance = (nearbyState.currentPosition - uavState.currentPosition).GetLength();
        Vector relativePosition = nearbyState.currentPosition - uavState.currentPosition;
        Vector relativeVelocity = nearbyState.velocityVector;
        nearbyDistances.emplace_back(distance, relativePosition, relativeVelocity);
    }

    // Sort the nearby UAVs by distance
    std::sort(nearbyDistances.begin(),
            nearbyDistances.end(),
            [](const std::tuple<double, Vector, Vector>& a, const std::tuple<double, Vector, Vector>& b) {
                return std::get<0>(a) < std::get<0>(b); // Compare distances
            });

    // Add the closest 2 UAVs to the observation
    size_t topUAVs = std::min(nearbyDistances.size(), size_t(2));
    for (size_t i = 0; i < topUAVs; ++i)
    {
        Vector relativeUAV = std::get<1>(nearbyDistances[i]); // Relative position
        Vector relativeVelocity = std::get<2>(nearbyDistances[i]); // Relative velocity
        box->AddValue(static_cast<float>(relativeUAV.x ));
        box->AddValue(static_cast<float>(relativeUAV.y ));
        box->AddValue(static_cast<float>(relativeVelocity.x));
        box->AddValue(static_cast<float>(relativeVelocity.y));
    }
    

    // Pad remaining entries if fewer than 2 UAVs
    for (size_t i = topUAVs; i < 2; ++i)
    {
        box->AddValue(static_cast<float>(COVERAGE_RADIUS/2)); // Padding value
        box->AddValue(static_cast<float>(COVERAGE_RADIUS/2));
        box->AddValue(static_cast<float>(0)); // Padding value
        box->AddValue(static_cast<float>(0)); // Padding value
    }

    // Final observation
    m_observation = box;
    // Log all observation types and their values with timestamp
    // NS_LOG_UNCOND("Time (s): " << Simulator::Now().GetSeconds());
    // NS_LOG_UNCOND("Observation for UAV " << nodeId << ":");
    // NS_LOG_UNCOND("  Relative Position to Base Station: (" 
    //               << relativeToBase.x / COVERAGE_RADIUS << ", " 
    //               << relativeToBase.y / COVERAGE_RADIUS << ")");
    // NS_LOG_UNCOND("  Velocity Vector: (" 
    //               << uavState.velocityVector.x / MAX_VEL << ", " 
    //               << uavState.velocityVector.y / MAX_VEL << ")");
    // NS_LOG_UNCOND("  Resistance Vector: (" 
    //               << resistanceVector.x / MAX_VEL << ", " 
    //               << resistanceVector.y / MAX_VEL << ")");
    // NS_LOG_UNCOND("  Network Power: " << networkPower);
    // NS_LOG_UNCOND("  Propulsion Power: " << propulsionPower);

    // NS_LOG_UNCOND("  Top 5 Local Centroids Covered:");
    // for (size_t i = 0; i < topN; ++i)
    // {
    //     Vector scaledCentroid = distanceWithVectors[i].second;
    //     NS_LOG_UNCOND("    Centroid " << i + 1 << ": (" 
    //                   << scaledCentroid.x / COVERAGE_RADIUS << ", " 
    //                   << scaledCentroid.y / COVERAGE_RADIUS << ")");
    // }

    // NS_LOG_UNCOND("  Closest 2 UAVs:");
    // for (size_t i = 0; i < topUAVs; ++i)
    // {
    //     Vector relativeUAV = std::get<1>(nearbyDistances[i]);
    //     Vector relativeVelocity = std::get<2>(nearbyDistances[i]);
    //     NS_LOG_UNCOND("    UAV " << i + 1 << ": Relative Position (" 
    //                   << relativeUAV.x / COVERAGE_RADIUS << ", " 
    //                   << relativeUAV.y / COVERAGE_RADIUS << "), Relative Velocity (" 
    //                   << relativeVelocity.x / MAX_VEL << ", " 
    //                   << relativeVelocity.y / MAX_VEL << ")");
    // }
    // Termination condition (optional)
    for (const auto& nearbyState : nearbyUAVStates)
    {
        double distanceToNearbyUAV = (nearbyState.currentPosition - uavState.currentPosition).GetLength();
        if (distanceToNearbyUAV < 5.0) // Terminate if any nearby UAV is within 20m range
        {
            auto terminateTime = Simulator::Now().GetSeconds();
            std::map<std::string, std::string> terminateInfo = {
                {std::string{"terminateTime"}, std::to_string(terminateTime)}};
            for (const auto& [agentId, agentInterface] : m_interfaces)
            {
                std::string agentIdString = "agent_" + std::to_string(nodeId);
                std::string nearbyAgentIdString = "agent_" + std::to_string(nearbyState.nodeId);
                NS_LOG_WARN("Terminate agents " << agentIdString << " and " << nearbyAgentIdString << " due to proximity! Time: " << terminateTime);
                OpenGymMultiAgentInterface::Get()->NotifyCurrentState(agentIdString,
                                                                      m_observation,
                                                                      -1,
                                                                      true,
                                                                      terminateInfo,
                                                                      Seconds(0),
                                                                      noopCallback);
                OpenGymMultiAgentInterface::Get()->NotifyCurrentState(nearbyAgentIdString,
                                                                      m_observation,
                                                                      -1,
                                                                      true,
                                                                      terminateInfo,
                                                                      Seconds(0),
                                                                      noopCallback);
            }
            break; // Exit the loop once termination is triggered
        }
    }
    
    if (relativeToBase.GetLength() > (COVERAGE_RADIUS/2 + 200) || uavState.velocityVector.GetLength() > MAX_VEL )
    {
        auto terminateTime = Simulator::Now().GetSeconds();
        
        std::map<std::string, std::string> terminateInfo = {
            {std::string{"terminateTime"}, std::to_string(terminateTime)}};
        for (const auto& [agentId, agentInterface] : m_interfaces)
        {
            int negReward = -1;
            std::string agentIdString = "agent_" + std::to_string(agentId);
          
            NS_LOG_WARN("Terminate agent " << agentIdString << "! Time: " << terminateTime);
            
            OpenGymMultiAgentInterface::Get()->NotifyCurrentState(agentIdString,
                                                                  m_observation,
                                                                  negReward,
                                                                  true,
                                                                  terminateInfo,
                                                                  Seconds(0),
                                                                  noopCallback);
        }
    }
}

void
UavObservationApp::SendObservation(double delay)
{
    Send(MakeDictContainer("floatObs", m_observation));
    Simulator::Schedule(Seconds(delay), &UavObservationApp::SendObservation, this, delay);
}

void
UavObservationApp::RegisterCallbacks()
{
    DynamicCast<UAVNode>(GetNode())->m_reportUavState.ConnectWithoutContext(
        MakeCallback(&UavObservationApp::Observe, this));
}

// REWARD

class UavRewardApp : public RewardApplication
{
  public:
    UavRewardApp() : RewardApplication() {};
    ~UavRewardApp() override {};
    static TypeId GetTypeId();
    void Reward(uint32_t nodeId,
        UAVState uavState,
        double networkPower,
        double propulsionPower,
        Vector resistanceVector,
        std::vector<UAVState> nearbyUAVStates);
    void RegisterCallbacks() override;

};

NS_OBJECT_ENSURE_REGISTERED(UavRewardApp);

TypeId
UavRewardApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavRewardApp")
                            .SetParent<RewardApplication>()
                            .SetGroupName("defiance")
                            .AddConstructor<UavRewardApp>();
    return tid;
}

// Helper function to get the unit vector
Vector GetUnitVector(const Vector& vec)
{
    double length = vec.GetLength();
    if (length == 0)
    {
        return Vector(0.0, 0.0, 0.0); // Return zero vector if length is zero
    }
    return Vector(vec.x / length, vec.y / length, vec.z / length);
}

// Helper function to calculate the dot product of two vectors
double DotProduct(const Vector& vec1, const Vector& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}
void
UavRewardApp::Reward(uint32_t nodeId,
    UAVState uavState,
    double networkPower,
    double propulsionPower,
    Vector resistanceVector,
    std::vector<UAVState> nearbyUAVStates)
{
    const float REWARD_SCALE = 0.1f;

    // Global Coverage weight
    const float GLOBAL_COVERAGE_WEIGHT = 0.15f;

    // Exploration weight
    const float EXPLORATION_WEIGHT = 0.1f;

    // Proximity penalty weight
    const float PROXIMITY_PENALTY_WEIGHT = -0.5f;
    const float PROXIMITY_THRESHOLD = 60.0f; // Threshold for proximity penalty

    // Diversity reward weight
    const float DIVERSITY_REWARD_WEIGHT = -0.5f;
    const float CENTER_PENALTY_WEIGHT = -0.05f;
    float totalReward = 0.0f;

    const float NETWORK_WEIGHT = 0.00f;
    const float LOCAL_WEIGHT = 0.5f;

    float networkPerformanceReward = 0.0f;
    if (uavPktData.packetLossRate < 100.0 && uavPktData.throughput > 0)
    {
        float lossComponent = 1.0f - static_cast<float>(uavPktData.packetLossRate / 100.0);
        networkPerformanceReward = NETWORK_WEIGHT * lossComponent;
    }
    // Local Coverage Reward Component
    float localCoverageReward = 0.0f;
    for (const auto& coveredCentroid : uavState.localCentroidsCovered)
    {
        float dist = (coveredCentroid.first).GetLength();
        if (dist <= 60.0f)
        {
            localCoverageReward += 1 - dist / 60.0f; // Positive reward for centroids within 60 units
        }
    }
    localCoverageReward = LOCAL_WEIGHT * localCoverageReward;

    // 1. Global Coverage Component
    float globalCoverageReward = 0.0f;
    std::set<uint32_t> uniqueCoveredEdgeIds;
    for (const auto& [agentId, coveredEdges] : agentClusterCoverage)
    {
        uniqueCoveredEdgeIds.insert(coveredEdges.begin(), coveredEdges.end());
    }
    float edgeCoverageRatio = static_cast<float>(uniqueCoveredEdgeIds.size()) / static_cast<float>(edgeNodes.GetN());
    globalCoverageReward = GLOBAL_COVERAGE_WEIGHT * (edgeCoverageRatio);
    
    // Positive reward for being near any centroid
    float centroidProximityReward = 0.0f;
    const float CENTROID_PROXIMITY_THRESHOLD = 100.0f; // Threshold for proximity to centroid
    const float CENTROID_PROXIMITY_WEIGHT = 0.01f; // Weight for proximity reward

    for (const auto& clusterPosition : clusterPositions)
    {
        double distanceToCentroid = (clusterPosition - uavState.currentPosition).GetLength();
        if (distanceToCentroid < CENTROID_PROXIMITY_THRESHOLD)
        {
            centroidProximityReward += CENTROID_PROXIMITY_WEIGHT * (1 - distanceToCentroid / CENTROID_PROXIMITY_THRESHOLD);
        }
    }

    // totalReward += centroidProximityReward;

    // 2. Exploration Component
    
  
    float explorationReward = 0.0f;
    // Iterate through all cluster positions
    Vector uavDirectionToCluster = uavState.currentPosition + uavState.movingTowards.first;
    double uavAlignment = uavState.movingTowards.second;
    double nearbyAlignment = 0.0;
    bool isTargetUnique = true;
    for (const auto& nearbyState : nearbyUAVStates)
    {
        Vector nearbyDirectionToCluster = nearbyState.currentPosition + nearbyState.movingTowards.first;
        nearbyAlignment = nearbyState.movingTowards.second;

        if ((nearbyDirectionToCluster - uavDirectionToCluster).GetLength() < 5.0) // Threshold for alignment and proximity
        {
            isTargetUnique = false;
            break;
        }
    }
    if (isTargetUnique)
    {
        explorationReward += EXPLORATION_WEIGHT * uavAlignment; // Higher reward for better alignment
    } 
    
    // 5. Stay Within Coverage Radius and Move Towards Center Component

    float centerPenalty = 0.0f;

    // Calculate distance from the center
    Vector centerPosition(250.0f, 250.0f, 0.0);
    double distanceFromCenter = (uavState.currentPosition - centerPosition).GetLength();

    // Penalize moving away from the center
    if (distanceFromCenter < COVERAGE_RADIUS)
    {
        centerPenalty += CENTER_PENALTY_WEIGHT * (distanceFromCenter/250.0f); // Adjust the penalty based on distance from center
    }
    

    // 3. Proximity Penalty Component
    float proximityPenalty = 0.0f;
    for (const auto& nearbyState : nearbyUAVStates)
    {
        double distance = (nearbyState.currentPosition - uavState.currentPosition).GetLength();
        if (distance < PROXIMITY_THRESHOLD)
        {
            proximityPenalty += PROXIMITY_PENALTY_WEIGHT*(1 - distance/PROXIMITY_THRESHOLD);
        }
    }

    // 4. Diversity Reward Component
    float diversityReward = 0.0f;
    Vector headingDirection = GetUnitVector(uavState.velocityVector);
    for (const auto& nearbyState : nearbyUAVStates)
    {
        Vector nearbyHeading = GetUnitVector(nearbyState.velocityVector);
        double dotProduct = DotProduct(headingDirection, nearbyHeading);

        // Penalize similar headings
        if (dotProduct > 0.8) // Threshold for similar headings
        {
            diversityReward += DIVERSITY_REWARD_WEIGHT * dotProduct;
        }
    }



    // Combine all components
    totalReward = REWARD_SCALE * (explorationReward + proximityPenalty +  globalCoverageReward + centerPenalty + diversityReward + localCoverageReward);
        // Store reward components for this agent
    agentRewardComponents[nodeId] = {
        globalCoverageReward,
        explorationReward,
        proximityPenalty,
        diversityReward,
        centerPenalty,
        networkPerformanceReward,
        localCoverageReward,
        propulsionPower,
        networkPower,
        totalReward
    };
    // Send the reward
    Send(MakeDictBoxContainer<float>(1, "reward", totalReward));
}

void
UavRewardApp::RegisterCallbacks()
{
    DynamicCast<UAVNode>(GetNode())->m_reportUavState.ConnectWithoutContext(
        MakeCallback(&UavRewardApp::Reward, this));
}

// INFERENCE AGENT
class InferenceAgentApp : public AgentApplication
{
  public:
    InferenceAgentApp()
        : AgentApplication(){
              // m_observation=MakeBoxContainer<float>(32);
              // m_reward=0;
          };

    ~InferenceAgentApp() override{};

    Time m_stepTime;

    void PerformInferenceStep()
    {
        // std::cout << "Starting inference step..." << std::endl;  // Log statement before the loop

        for (const auto& [appId, interface] : m_observationInterfaces)
        {
            // use observation for current UAV
            // std::cout << "AppId:" << appId<< std::endl;
            if (m_obsDataStruct.HistoryExists(appId))
            {
                m_observation = m_obsDataStruct.GetNewestByID(appId)
                                    ->data->Get("floatObs")
                                    ->GetObject<OpenGymBoxContainer<float>>();
            }
            else
            {
                m_observation = GetResetObservation();
            }

            // use reward for current UAV
            if (m_rewardDataStruct.HistoryExists(appId))
            {
                m_reward = m_rewardDataStruct.GetNewestByID(appId)
                               ->data->Get("reward")
                               ->GetObject<OpenGymBoxContainer<float>>()
                               ->GetValue(0);
            }
            else
            {
                m_reward = 0;
            }
            // std::cout << "INFER:" << appId<< std::endl;
            // set id to sent action to corresponding ActionApp
            InferAction(appId);
        }

        Simulator::Schedule(m_stepTime, &InferenceAgentApp::PerformInferenceStep, this);
    }

    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::InferenceAgentApp")
                                .SetParent<AgentApplication>()
                                .SetGroupName("defiance")
                                .AddConstructor<InferenceAgentApp>()
                                .AddAttribute("StepTime",
                                              "the step time delay between inference steps",
                                              TimeValue(MilliSeconds(1000)),
                                              MakeTimeAccessor(&InferenceAgentApp::m_stepTime),
                                              MakeTimeChecker());
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
        auto observation = m_obsDataStruct.GetNewestByID(id)
                               ->data->Get("floatObs")
                               ->GetObject<OpenGymBoxContainer<float>>();
        m_observation = observation;
    }

    void OnRecvReward(uint id) override
    {
        // m_reward = m_rewardDataStruct.GetNewestByID(id)
        // ->data->Get("reward")
        // ->GetObject<OpenGymBoxContainer<float>>()
        // ->GetValue(0);
    }

    Ptr<OpenGymDataContainer> GetResetObservation()
    {
        auto obs = MakeBoxContainer<float>(OBSERVATION_SIZE);
        for (int i = 0; i < OBSERVATION_SIZE; i++)
        {
            obs->AddValue(0.0);
        }
        return obs;
    }

    float GetResetReward()
    {
        return 0.0;
    }

  private:
    Ptr<OpenGymSpace> GetObservationSpace() override
    {
        std::vector<float> low(OBSERVATION_SIZE, -COVERAGE_RADIUS);
        std::vector<float> high(OBSERVATION_SIZE, COVERAGE_RADIUS);
        low[2] = -MAX_VEL;
        high[2] = MAX_VEL;
        low[3] =  -MAX_VEL;
        high[3] = MAX_VEL;
        low[4] =  -MAX_VEL;
        high[4] = MAX_VEL;
        low[5] =  -MAX_VEL;
        high[5] = MAX_VEL;
        low[6] = -100.0f;
        high[6] = 100.0f;
        low[7] = -100.0f;
        high[7] = 100.0f;
        low[16] =  -MAX_VEL;
        high[16] = MAX_VEL;
        low[17] =  -MAX_VEL;
        high[17] = MAX_VEL;
        low[20] =  -MAX_VEL;
        high[20] = MAX_VEL;
        low[21] =  -MAX_VEL;
        high[21] = MAX_VEL;


        return MakeBoxSpace<float>(OBSERVATION_SIZE, low, high);
    }

    Ptr<OpenGymSpace> GetActionSpace() override
    {
        // Continuous action space for heading direction
        std::vector<float> low = {-15.0f, 5.0f}; // Example: Velocity range [-50, 50]
        std::vector<float> high = {15.0f, 15.0f};
        return MakeBoxSpace<float>(2, low, high);
    }
};

// ACTIONS

class UavActionApp : public ActionApplication
{
  public:
    UavActionApp(){};
    ~UavActionApp() override{};

    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::UavActionApp")
                                .SetParent<ActionApplication>()
                                .SetGroupName("defiance")
                                .AddConstructor<UavActionApp>();
        return tid;
    }

    void ExecuteAction(uint32_t remoteAppId, Ptr<OpenGymDictContainer> action) override
    {
        auto uavNode = DynamicCast<UAVNode>(GetNode()); // Assuming UavController controls the UAV
        auto act = action->Get("default")->GetObject<OpenGymBoxContainer<float>>();

        // Extract x and y velocities from the action
        float angle = act->GetValue(0);
        float velScalar = act->GetValue(1);
        // std::cout << "ID:"<<remoteAppId << " ANGLE " << angle << " VEL "<< velScalar <<  std::endl;
        if (std::isnan(angle))
        {
            angle = 0;
        }

        // Set the velocity vector on the UAV node
        uavNode->SetHeadingAngle(angle, velScalar);
    }

    void SetObservationApp(Ptr<UavObservationApp> observationApp)
    {
        m_observationApp = observationApp;
    }

  private:
    Ptr<UavObservationApp> m_observationApp;
};

NS_OBJECT_ENSURE_REGISTERED(UavActionApp);
NS_OBJECT_ENSURE_REGISTERED(InferenceAgentApp);

int
main(int argc, char* argv[])
{
    NS_LOG_APPEND_TIME_PREFIX;
    
    int offset = 1;
    uint32_t seed = 1;
    uint32_t runId = 1;
    uint32_t parallel = 0;
    std::string trial_name = "0"; // use simple channel interface per default

    // first of all we need to parse the command line arguments
    std::string interfaceType = "SIMPLE"; // use simple channel interface per default
    bool visualize = false;

    CommandLine cmd;
    cmd.AddValue("trial_name", "name of the trial", trial_name);
    cmd.AddValue("seed", "Seed to create comparable scenarios", seed);
    cmd.AddValue("runId", "Run ID. Is increased for every reset of the environment", runId);
    cmd.AddValue("parallel",
                 "Parallel ID. When running multiple environments in parallel, this is the index.",
                 parallel);
    cmd.AddValue("interfaceType", "The type of the channel interface to use", interfaceType);
    cmd.AddValue("numberOfAgents",
                 "The number of agents and base stations used in the simulation",
                 nUAV);
    cmd.AddValue("visualize", "Log visualization traces", visualize);
    cmd.AddValue("nEdge", "Number of Edge nodes", nEdge);
    cmd.AddValue("nUAV", "Number of UAV nodes", nUAV);
    cmd.AddValue("nClusters", "Number of clusters for edge nodes", nClusters);
    // cmd.AddValue("MIN_X", "Minimum X boundary", MIN_X);
    // cmd.AddValue("MAX_X", "Maximum X boundary", MAX_X);
    // cmd.AddValue("MIN_Y", "Minimum Y boundary", MIN_Y);
    // cmd.AddValue("MAX_Y", "Maximum Y boundary", MAX_Y);
    // cmd.AddValue("MIN_Z", "Minimum Z boundary", MIN_Z);
    // cmd.AddValue("MAX_Z", "Maximum Z boundary", MAX_Z);
    // cmd.AddValue("SIM_TIME", "Simulation time", SIM_TIME);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed+ parallel);
    RngSeedManager::SetRun(runId);
    Ns3AiMsgInterface::Get()->SetTrialName(trial_name);

    LogComponentEnable("WirelessMeshUAV", LOG_LEVEL_ALL);

    //**************************SETTING ENV************************************ */
    
    // Create the environment using UAVEnvCreator
    UAVEnvCreator environmentCreator;
    environmentCreator.SetupUAVScenario(nEdge, nUAV, nClusters, MIN_X, MAX_X, MIN_Y, MAX_Y, MIN_Z, MAX_Z, SIM_TIME);

    // Access the nodes created by the environment creator
    edgeNodes = environmentCreator.GetEdgeNodes();

    NodeContainer baseStation = environmentCreator.GetBaseStation();
    Ptr<MobilityModel> baseMobility = baseStation.Get(0)->GetObject<MobilityModel>();
    baseStationPosition = baseMobility->GetPosition();

    NodeContainer uavNodes = environmentCreator.GetUAVNodes();
    clusterPositions = environmentCreator.GetClusterPositions();

    // Now you can use the nodes and other components as needed
    // Example: Print the number of nodes
    // std::cout << "Number of Edge Nodes: " << edgeNodes.GetN() << std::endl;
    // std::cout << "Number of UAV Nodes: " << uavNodes.GetN() << std::endl;
    // std::cout << "Number of Base Station Nodes: " << baseStation.GetN() << std::endl;
    // std::cout << "Number of Clusters: " << clusterPositions.size() << std::endl;

    // Set up Animation (Optional, keep it as is)
    AnimationInterface anim("uav_sim.xml"); // Changed animation file name
    anim.SetMaxPktsPerTraceFile(500000);

    // Color the nodes (Red for UAVs, Green for Edge nodes, Blue for Base Station)
    for (uint32_t i = 0; i < edgeNodes.GetN(); ++i)
    {
        anim.UpdateNodeDescription(edgeNodes.Get(i), "Edge Node");
        anim.UpdateNodeColor(edgeNodes.Get(i), 0, 255, 0); // Green
    }

    for (uint32_t i = 0; i < uavNodes.GetN(); ++i)
    {
        anim.UpdateNodeDescription(uavNodes.Get(i), "UAV Node");
        anim.UpdateNodeColor(uavNodes.Get(i), 255, 0, 0); // Red
    }

    for (uint32_t i = 0; i < baseStation.GetN(); ++i)
    {
        anim.UpdateNodeDescription(baseStation.Get(i), "Base Station");
        anim.UpdateNodeColor(baseStation.Get(i), 0, 0, 255); // Blue
    }

    // phy1.EnablePcapAll ("wifi-sim");

    // phy.EnablePcapAll ("wifi-sim"); // Disable global pcap, if you only want UAV pcap

    // Enable IPv4 route tracking for routing visualization for UAVs
    anim.EnableIpv4RouteTracking("routingtable-uavs.xml",
                                 Seconds(0),
                                 Seconds(SIM_TIME),
                                 Seconds(0.5));

    uavPktData.monitor = uavPktData.flowmon.InstallAll();
    uavPktData.totalDelaySum = 0;
    uavPktData.totalRxBytes = 0;
    uavPktData.totalRxPackets = 0;
    Simulator::Schedule(Seconds(2.0 - 1.0), &Throughput);

    // Flow Monitor Configuration (Optional, for more detailed performance metrics)
    // FlowMonitorHelper flowmon;
    // Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    //*******************************FINISHED SETING UP ENV************************************* */

    RlApplicationHelper helper(TypeId::LookupByName("ns3::UavObservationApp"));
    helper.SetAttribute("StartTime", TimeValue(Seconds(0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(offset + SIM_TIME)));
    RlApplicationContainer observationApps = helper.Install(uavNodes);

    helper.SetTypeId("ns3::UavRewardApp");
    RlApplicationContainer rewardApps = helper.Install(uavNodes);

    helper.SetTypeId("ns3::UavActionApp");
    RlApplicationContainer actionApps = helper.Install(uavNodes);

    helper.SetTypeId("ns3::InferenceAgentApp");
    RlApplicationContainer agentApps = helper.Install(uavNodes);

    CommunicationHelper commHelper = CommunicationHelper();

    commHelper.SetObservationApps(observationApps);
    commHelper.SetAgentApps(agentApps);
    commHelper.SetRewardApps(rewardApps);
    commHelper.SetActionApps(actionApps);
    commHelper.SetIds();

    std::vector<CommunicationPair> adjacency = {};

    // add communications depending on what run type is chosen

    for (uint i = 0; i < uavNodes.GetN(); i++)
    {
        // uint uavId = i * nUAV + j;
        CommunicationPair observationCommPair = {observationApps.GetId(i), agentApps.GetId(i), {}};
        CommunicationPair rewardCommPair = {rewardApps.GetId(i), agentApps.GetId(i), {}};
        CommunicationPair actionCommPair = {actionApps.GetId(i), agentApps.GetId(i), {}};
        adjacency.emplace_back(observationCommPair);
        adjacency.emplace_back(rewardCommPair);
        adjacency.emplace_back(actionCommPair);
    }

    commHelper.AddCommunication(adjacency);
    commHelper.Configure();

    for (uint i = 0; i < uavNodes.GetN(); i++)
    {
        DynamicCast<UavActionApp>(actionApps.Get(i))
            ->SetObservationApp(DynamicCast<UavObservationApp>(observationApps.Get(i)));
    }

    for (uint i = 0; i < observationApps.GetN(); i++)
    {
        Simulator::Schedule(Seconds(offset),
                            &UavObservationApp::SendObservation,
                            DynamicCast<UavObservationApp>(observationApps.Get(i)),
                            0.1);
    }

    for (uint i = 0; i < agentApps.GetN(); i++)
    {
        Simulator::Schedule(Seconds(offset + 2),
                            &InferenceAgentApp::PerformInferenceStep,
                            DynamicCast<InferenceAgentApp>(agentApps.Get(i)));
    }

    // Ensure a new stats file is created every time the simulation runs
    std::string pathToNs3 = std::getenv("NS3_HOME");
    std::ofstream stats_file(pathToNs3 + "/uav_stats.csv", std::ios_base::trunc); // Use trunc to overwrite the old file
    stats_file << "Time,NodeId,PosX,PosY,PosZ,VelX,VelY,VelZ,ResX,ResY,ResZ,NetworkPower,PropulsionPower,"
               << "ClusterDistances,NearbyUAVs,Throughput,Delay,PacketLossRate,RewardComponents" << std::endl;
    //Saving edge positions
    std::ofstream edge_positions_file(pathToNs3 + "/edge_positions.csv", std::ios_base::trunc);
    edge_positions_file << "EdgeNodeId,PosX,PosY,PosZ" << std::endl;
    for (uint32_t i = 0; i < edgeNodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mobility = edgeNodes.Get(i)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        edge_positions_file << i << "," << position.x << "," << position.y << "," << position.z << std::endl;
    }
    edge_positions_file.close();


    // Add code to save stats if needed, or remove this section.

    if (1)
    {

        Ptr<OutputStreamWrapper> stats_file_ptr = Create<OutputStreamWrapper>(&stats_file);
        for (uint i = 0; i < nUAV; i++)
        {
            DynamicCast<UAVNode>(uavNodes.Get(i))
                ->m_reportUavState.ConnectWithoutContext(
                    MakeBoundCallback(&SaveStats, stats_file_ptr));
        }
    }

    Simulator::Stop(Seconds(SIM_TIME + offset));

    // Run the simulation
    Simulator::Run();

    //------ Flow Monitor Output -------
    uavPktData.monitor->SerializeToXmlFile("uav_flowmon.xml", false, true);

    Simulator::Destroy();
    OpenGymMultiAgentInterface::Get()->NotifySimulationEnd();
    return 0;
}