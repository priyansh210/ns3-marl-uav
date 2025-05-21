#ifndef UAV_ENV_CREATOR_H
#define UAV_ENV_CREATOR_H

#include "ns3/aodv-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <ns3/netanim-module.h>
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

#include <iostream>
#include <cstdlib> // For rand() and srand()
#include <ctime> 

#include <vector>

namespace ns3 {

class UAVEnvCreator : public Object
{
public:
    UAVEnvCreator();

    NodeContainer& GetEdgeNodes();
    NodeContainer& GetBaseStation();
    NodeContainer& GetUAVNodes();
    std::vector<Vector>& GetClusterPositions();

    void CreateEdgeNodes(uint32_t nEdge, uint32_t nClusters, double minX, double maxX, double minY, double maxY);
    void CreateBaseStation(double minX, double maxX, double minY, double maxY);
    void CreateUAVNodes(uint32_t nUAV, double minX, double maxX, double minY, double maxY, double minZ, double maxZ);
    void SetupUAVScenario(uint32_t nEdge, uint32_t nUAV, uint32_t nClusters, double minX, double maxX, double minY, double maxY, double minZ, double maxZ, uint32_t SIM_TIME);

    void SetEdgeNodes(NodeContainer& edgeNodes);
    void SetBaseStation(NodeContainer& baseStation);
    void SetClusterPositions(std::vector<Vector>& clusterPositions);
    static void Throughput();
    

private:
    NodeContainer m_edgeNodes;
    NodeContainer m_baseStation;
    NodeContainer m_uavNodes;
    std::vector<Vector> m_clusterPositions;
};

} // namespace ns3

#endif // UAV_ENV_CREATOR_H