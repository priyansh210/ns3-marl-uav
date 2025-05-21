#include "uav-env-creator.h"
#include "uav-node.h"

#include <cstdlib>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UAVEnvCreator");

UAVEnvCreator::UAVEnvCreator()
{
}

NodeContainer& UAVEnvCreator::GetEdgeNodes()
{
    return m_edgeNodes;
}

NodeContainer& UAVEnvCreator::GetBaseStation()
{
    return m_baseStation;
}

NodeContainer& UAVEnvCreator::GetUAVNodes()
{
    return m_uavNodes;
}

std::vector<Vector>& UAVEnvCreator::GetClusterPositions()
{
    return m_clusterPositions;
}



void UAVEnvCreator::CreateEdgeNodes(uint32_t nEdge, uint32_t nClusters, double minX, double maxX, double minY, double maxY)
{
    m_edgeNodes.Create(nEdge);

    MobilityHelper mobility;
    Ptr<UniformRandomVariable> xRand = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> yRand = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < nClusters; ++i)
    {
        double x = xRand->GetValue(minX, maxX);
        double y = yRand->GetValue(minY, maxY);
        m_clusterPositions.push_back(Vector(x, y, 0));
    }

    NodeContainer::Iterator nodeIterator = m_edgeNodes.Begin();
    for (uint32_t i = 0; i < nEdge; ++i)
    {
        uint32_t clusterId = i % nClusters;
        double clusterX = m_clusterPositions[clusterId].x;
        double clusterY = m_clusterPositions[clusterId].y;
        double edgeNodeX = clusterX + xRand->GetValue(-20.0, 20.0);
        double edgeNodeY = clusterY + yRand->GetValue(-20.0, 20.0);

        ListPositionAllocator* positionAlloc = new ListPositionAllocator();
        positionAlloc->Add(Vector(edgeNodeX, edgeNodeY, 0.0));
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(*nodeIterator);
        ++nodeIterator;
    }
}

void UAVEnvCreator::CreateBaseStation(double minX, double maxX, double minY, double maxY)
{
    m_baseStation.Create(1);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue((minX + maxX) / 2),
                                  "MinY", DoubleValue((minY + maxY) / 2),
                                  "GridWidth", UintegerValue(1),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_baseStation);
}

void UAVEnvCreator::CreateUAVNodes(uint32_t nUAV, double minX, double maxX, double minY, double maxY, double minZ, double maxZ)
{
    std::vector<UAVNode*> uavNodePtrs; // Vector to store pointers to all UAV nodes

    Ptr<UniformRandomVariable> windDirection = CreateObject<UniformRandomVariable>();
    double windX = windDirection->GetValue(-5.0, 5.0); // Random wind direction in X
    double windY = windDirection->GetValue(-5.0, 5.0); // Random wind direction in Y
    double windZ = windDirection->GetValue(-1.0, 1.0); // Random wind direction in Z (optional)
    Vector windResistance(windX, windY, windZ);

    // Create UAV nodes and add them to the container and vector
    for (uint32_t i = 0; i < nUAV; ++i)
    {
        Ptr<UAVNode> uavNode = CreateObject<UAVNode>(minX, maxX, minY, maxY, minZ, maxZ);
        m_uavNodes.Add(uavNode);
        uavNodePtrs.push_back(&(*uavNode)); // Add raw pointer to the vector

        uavNode->SetResistance(windResistance);

    
        Simulator::Schedule(Seconds(1.0), &UAVNode::SimulateTimeTic, uavNode, 0.1);
    }

    // Set the list of other UAV nodes for each UAV
    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        Ptr<UAVNode> uavNode = DynamicCast<UAVNode>(m_uavNodes.Get(i));
        uavNode->SetOtherUAVNodes(uavNodePtrs);
    }
}

void UAVEnvCreator::SetupUAVScenario(uint32_t nEdge, uint32_t nUAV, uint32_t nClusters, double minX, double maxX, double minY, double maxY, double minZ, double maxZ, uint32_t SIM_TIME)
{
    CreateEdgeNodes(nEdge, nClusters, minX, maxX, minY, maxY);
    CreateBaseStation(minX, maxX, minY, maxY);
    CreateUAVNodes(nUAV, minX, maxX, minY, maxY, minZ, maxZ);

    // Setup Internet Stack and Routing
    InternetStackHelper internet;
    AodvHelper aodv;
    internet.SetRoutingHelper(aodv);
    internet.Install(m_edgeNodes);
    internet.Install(m_uavNodes);
    internet.Install(m_baseStation);

    // Create a single WiFi Adhoc network for all nodes
    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211b);
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode", StringValue("DsssRate11Mbps"),
        "ControlMode", StringValue("DsssRate11Mbps"));
    Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(200.0));
    channel.AddPropagationLoss("ns3::RangePropagationLossModel");

    phy.Set("TxPowerStart", DoubleValue(15.0));
    phy.Set("TxPowerEnd", DoubleValue(15.0));
    phy.Set("RxGain", DoubleValue(13));

    WifiMacHelper macAdhoc;
    macAdhoc.SetType("ns3::AdhocWifiMac", "Ssid", SsidValue(Ssid("single-adhoc-network")));

    NetDeviceContainer adhocDevices = wifi.Install(phy, macAdhoc, m_edgeNodes);
    adhocDevices.Add(wifi.Install(phy, macAdhoc, m_uavNodes));
    adhocDevices.Add(wifi.Install(phy, macAdhoc, m_baseStation));
    // Assign IP addresses and MAC addresses for the single adhoc network
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces = address.Assign(adhocDevices);

    // Set up UDP echo server on Base Station
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(m_baseStation.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(SIM_TIME));

    // Set up UDP echo clients on Edge Nodes to send packets to Base Station
    Ipv4Address baseStationAddress = adhocInterfaces.GetAddress(m_baseStation.Get(0)->GetId());
    // NS_ASSERT_MSG(!baseStationAddress.IsInvalid(), "Base station address is invalid");
    UdpEchoClientHelper echoClient(baseStationAddress, 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(m_edgeNodes);
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(SIM_TIME));
   
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Set EdgeNodes, BaseStation, Cluster positions to UAV nodes.
    for (uint32_t i = 0; i < m_uavNodes.GetN(); ++i)
    {
        Ptr<UAVNode> uavNode = DynamicCast<UAVNode>(m_uavNodes.Get(i));
        uavNode->SetEdgeNodes(&m_edgeNodes);
        uavNode->SetBaseStation(&m_baseStation);
        uavNode->SetClusterPositions(&m_clusterPositions);
    }

}

void UAVEnvCreator::SetEdgeNodes(NodeContainer& edgeNodes) {
    m_edgeNodes = edgeNodes;
}

void UAVEnvCreator::SetBaseStation(NodeContainer& baseStation) {
    m_baseStation = baseStation;
}

void UAVEnvCreator::SetClusterPositions(std::vector<Vector>& clusterPositions) {
    m_clusterPositions = clusterPositions;
}
