#include "uav-node.h"

#include <cmath>
#include <cstdlib>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UAVNode");

NS_OBJECT_ENSURE_REGISTERED(UAVNode);

TypeId
UAVNode::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UAVNode")
            .SetParent<Node>()
            .AddConstructor<UAVNode>()
            .AddAttribute("UAVRange",
                          "The range of the UAV",
                          IntegerValue(60),
                          MakeIntegerAccessor(&UAVNode::m_uavRange),
                          MakeIntegerChecker<int>())
            .AddAttribute("MinAngle",
                          "The minimum angle of the UAV",
                          DoubleValue(-15.0),
                          MakeDoubleAccessor(&UAVNode::m_minAngle),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxAngle",
                          "The maximum angle of the UAV",
                          DoubleValue(15.0),
                          MakeDoubleAccessor(&UAVNode::m_maxAngle),
                          MakeDoubleChecker<double>())
            .AddAttribute("MinVel",
                          "The minimum velocity of the UAV",
                          DoubleValue(5.0),
                          MakeDoubleAccessor(&UAVNode::m_minVel),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxVel",
                          "The maximum velocity of the UAV",
                          DoubleValue(15.0),
                          MakeDoubleAccessor(&UAVNode::m_maxVel),
                          MakeDoubleChecker<double>())
            .AddAttribute("HeadingAngle",
                          "The heading angle of the UAV",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&UAVNode::m_headingAngle),
                          MakeDoubleChecker<double>())
            .AddAttribute("VelocityVector",
                          "The velocity vector of the UAV",
                          VectorValue(Vector(0.0, 0.0, 0.0)),
                          MakeVectorAccessor(&UAVNode::m_velocityVector),
                          MakeVectorChecker())
            .AddAttribute("RelativePosition",
                          "The relative position of the UAV",
                          VectorValue(Vector(0.0, 0.0, 0.0)),
                          MakeVectorAccessor(&UAVNode::m_relativePosition),
                          MakeVectorChecker())
            .AddAttribute("CostAtPosition",
                          "The cost at the position of the UAV",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&UAVNode::m_costAtPosition),
                          MakeDoubleChecker<double>())
            .AddAttribute("Power",
                          "The power of the UAV",
                          DoubleValue(100.0),
                          MakeDoubleAccessor(&UAVNode::m_power),
                          MakeDoubleChecker<double>())
            .AddTraceSource("ReportUAVStats",
                             "Reports all the physical properties of the UAV after one timestep update",
                             MakeTraceSourceAccessor(&UAVNode::m_reportUavState),
                             "ns3::UAVNode::ReportUAVStatsCallback");

    return tid;
}

UAVNode::UAVNode() : UAVNode(0.0, 500.0, 0.0, 500.0, 0.0, 50.0) {}

UAVNode::UAVNode(double minX, double maxX, double minY, double maxY, double minZ, double maxZ)
    : Node(),
      m_velocityVector(0.0, 0.0, 0.0),
      m_relativePosition(0.0, 0.0, 0.0),
      m_costAtPosition(0.0),
      m_power(100.0),
      m_edgeNodesPtr(nullptr),
      m_baseStationPtr(nullptr),
      m_clusterPositionsPtr(nullptr)
    //   m_uavState(UAVState()) // Initialize UAVState using its default constructor
{
    NS_LOG_FUNCTION(this);

    MobilityHelper mobility;

    auto positionAlloc = CreateObject<ListPositionAllocator>();

    // Generate random positions based on UAV ID and seed
    Ptr<UniformRandomVariable> randX = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> randY = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> randZ = CreateObject<UniformRandomVariable>();

    // Set seed and stream based on UAV ID for deterministic behavior

    // uint32_t seed = 12345; // Example seed value
    // uint32_t stream = GetId(); // Use UAV ID as stream
    // randX->SetStream(stream);
    // randY->SetStream(stream + 1); // Offset stream for Y
    // randZ->SetStream(stream + 2); // Offset stream for Z

    // double x = randX->GetValue(minX, maxX);
    // double y = randY->GetValue(minY, maxY);
    // double z = 50.0 ;

    double x = minX + (maxX - minX) * (double)rand() / RAND_MAX;
    double y = minY + (maxY - minY) * (double)rand() / RAND_MAX;
    double z = 50.0;

    positionAlloc->Add(Vector(x, y, z));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(this);

    Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
    Ptr<WifiRadioEnergyModel> wifiEnergyModel = CreateObject<WifiRadioEnergyModel>();

    energySource->SetInitialEnergy(10000);
    wifiEnergyModel->SetEnergySource(energySource);
    energySource->AppendDeviceEnergyModel(wifiEnergyModel);
    this->AggregateObject(energySource);

    m_prevNetworkPower = energySource->GetRemainingEnergy();

    m_resistanceVector = Vector(0.0, 0.0, 0.0);
    m_uavState = UAVState();
    m_uavState.nodeId = GetId();
    // Fill UAV state with initial values
    m_uavState.currentPosition = Vector(x, y, z);
    // m_uavState.velocityVector = m_velocityVector;
    m_uavState.powerLeft = m_power;
    // m_uavState.relativeDistances.clear();
    // m_uavState.centroidsInRange = 0;
}

UAVNode::~UAVNode()
{
    NS_LOG_FUNCTION(this);
}

void
UAVNode::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Node::DoDispose();
}

void
UAVNode::SetVelocityVector(Vector velocityVector)
{
    NS_LOG_FUNCTION(this << velocityVector);
    m_velocityVector = velocityVector;
    Ptr<ConstantVelocityMobilityModel> mobility = this->GetObject<ConstantVelocityMobilityModel>();
    mobility->SetVelocity(m_velocityVector);
}

void
UAVNode::SetHeadingAngle(double angle, double scalar)
{
    NS_LOG_FUNCTION(this << angle << scalar);
    angle = std::max(m_minAngle, angle);
    angle = std::min(m_maxAngle, angle);

    scalar = std::max(m_minVel, scalar);
    scalar = std::min(m_maxVel, scalar);

    m_headingAngle += angle;

    Vector velVec =
        Vector(scalar * cos(m_headingAngle * M_PI / 180.0), scalar * sin(m_headingAngle * M_PI / 180.0), 0);
    SetVelocityVector(velVec);
}

Vector UAVNode::GetUnitVec( Vector vec)
{
    double length = vec.GetLength();
    if (length == 0)
    {
        return Vector(0.0, 0.0, 0.0); // Return zero vector if length is zero
    }
    return Vector(vec.x / length, vec.y / length, vec.z / length);
}

double DotProduct(const Vector& vec1, const Vector& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

void
UAVNode::SimulateTimeTic(double deltaT)
{
    NS_LOG_FUNCTION(this << deltaT);

    Ptr<ConstantVelocityMobilityModel> mobility = this->GetObject<ConstantVelocityMobilityModel>();
    Vector m_uavPosition = mobility->GetPosition();

    Ptr<BasicEnergySource> energySource = this->GetObject<BasicEnergySource>();
    double remainingEnergy = energySource->GetRemainingEnergy();
    double initialEnergy = energySource->GetInitialEnergy();
    m_power = (remainingEnergy / initialEnergy) * 100.0;

    SimulateSensor();
    m_networkPower = CalculateNetworkPowerLoss();
    m_propulsionPower = CalculatePropulsionPowerLoss();

    // Exclude the current UAV from nearby UAV states
    std::vector<UAVState> nearbyUAVStates;
    for (size_t i = 0; i < m_otherUAVNodes.size(); ++i)
    {
        if (m_otherUAVNodes[i] == this)
        {
            continue;
        }

        Ptr<ConstantVelocityMobilityModel> otherMobility = m_otherUAVNodes[i]->GetObject<ConstantVelocityMobilityModel>();
        Vector otherPosition = otherMobility->GetPosition();
        if ((m_uavPosition - otherPosition).GetLength() <= UAV_UAV_RANGE)
        {
            UAVState otherState = m_otherUAVNodes[i]->GetUAVState();
            nearbyUAVStates.push_back(otherState);
        }
    }

    m_reportUavState(GetId(),
                     m_uavState,
                     m_networkPower,
                     m_propulsionPower,
                     m_resistanceVector,
                     nearbyUAVStates);

    Simulator::Schedule(Seconds(deltaT), &UAVNode::SimulateTimeTic, this, deltaT);
}

void
UAVNode::SetEdgeNodes(NodeContainer* edgeNodesPtr)
{
    NS_LOG_FUNCTION(this << edgeNodesPtr);
    m_edgeNodesPtr = edgeNodesPtr;
}

void
UAVNode::SetBaseStation(NodeContainer* baseStationPtr)
{
    NS_LOG_FUNCTION(this << baseStationPtr);
    m_baseStationPtr = baseStationPtr;
}

void
UAVNode::SetClusterPositions(std::vector<Vector>* clusterPositionsPtr)
{
    NS_LOG_FUNCTION(this << clusterPositionsPtr);
    m_clusterPositionsPtr = clusterPositionsPtr;
}

UAVState
UAVNode::GetUAVState() const
{
    return m_uavState;
}

void
UAVNode::SetResistance(Vector resistanceVector)
{
    NS_LOG_FUNCTION(this << resistanceVector);
    m_resistanceVector = resistanceVector;
}

void
UAVNode::SimulateSensor()
{
    NS_LOG_FUNCTION(this);

    // Get the current UAV position
    Ptr<ConstantVelocityMobilityModel> mobility = this->GetObject<ConstantVelocityMobilityModel>();
    m_uavPosition = mobility->GetPosition();

    // Initialize variables for centroids
    int centroidsInRange = 0;
    std::vector<std::pair<Vector, double>> centroidsDiscovered; // Relative positions of discovered centroids
    std::vector<std::pair<Vector, double>> centroidsCovered;    // Relative positions of covered centroids
    std::vector<std::pair<Vector, double>> localCentroidsCovered; // Centroids covered locally

    // Discover centroids in local range
    if (m_clusterPositionsPtr)
    {
        for (const auto& clusterPosition : *m_clusterPositionsPtr)
        {
            Vector distanceVector = clusterPosition - m_uavPosition;
            double distance = distanceVector.GetLength();
            double cost = 100.0 / (distance + 1); // Cost inversely proportional to distance

            if (distance <= UAV_EDGE_RANGE)
            {
                centroidsCovered.emplace_back(distanceVector, cost);
                centroidsDiscovered.emplace_back(distanceVector, cost);
                localCentroidsCovered.emplace_back(distanceVector, cost);
                centroidsInRange++;
            }
        }
    }

    // Aggregate centroids from other UAVs
    for (const auto& otherUAV : m_otherUAVNodes)
    {
        // Skip the current UAV itself
        if (otherUAV == this)
        {
            continue;
        }

        Ptr<ConstantVelocityMobilityModel> otherMobility = otherUAV->GetObject<ConstantVelocityMobilityModel>();
        Vector otherPosition = otherMobility->GetPosition();

        // Only consider UAVs within UAV_UAV_RANGE
        if ((m_uavPosition - otherPosition).GetLength() <= UAV_UAV_RANGE)
        {
            UAVState otherState = otherUAV->GetUAVState();

            // Add centroids covered by other UAVs
            for (const auto& otherCentroid : otherState.centroidsCovered)
            {
                Vector distanceVector = otherCentroid.first + (otherState.currentPosition - m_uavPosition);
                double cost = otherCentroid.second;
                centroidsCovered.emplace_back(distanceVector, cost);
            }

            // Add centroids discovered by other UAVs
            for (const auto& otherCentroid : otherState.centroidsDiscovered)
            {
                Vector distanceVector = otherCentroid.first + (otherState.currentPosition - m_uavPosition);
                double cost = otherCentroid.second;
                centroidsDiscovered.emplace_back(distanceVector, cost);
            }
        }
    }

    // Filter unique centroids
    centroidsDiscovered = GetUniqueCentroids(centroidsDiscovered, 2.0); // Threshold: 2.0
    centroidsCovered = GetUniqueCentroids(centroidsCovered, 2.0);
    localCentroidsCovered = GetUniqueCentroids(localCentroidsCovered, 2.0);

    // Determine the closest centroid in range based on direction and distance
    Vector uavDirection = GetUnitVec(m_velocityVector);
    double maxAlignment = -1.0;
    std::pair<Vector, double> targetCluster;

    for (const auto& clusterPosition : *m_clusterPositionsPtr)
    {
        Vector directionToCluster = clusterPosition - m_uavPosition;
        Vector unitClusterDirection = GetUnitVec(directionToCluster);
        double alignment = DotProduct(uavDirection, unitClusterDirection);

        if (alignment > maxAlignment && directionToCluster.GetLength() <= UAV_UAV_RANGE)
        {
            // Update the maximum alignment and target cluster
            maxAlignment = alignment;
            targetCluster = {directionToCluster, alignment};
        }
    }
   
    // Update UAV state
    m_uavState.nodeId = GetId();
    m_uavState.currentPosition = m_uavPosition;
    m_uavState.velocityVector = m_velocityVector;
    m_uavState.powerLeft = m_power;
    m_uavState.centroidsDiscovered = centroidsDiscovered;
    m_uavState.centroidsCovered = centroidsCovered;
    m_uavState.localCentroidsCovered = localCentroidsCovered;
    m_uavState.movingTowards = targetCluster;
    m_uavState.centroidsInRange = centroidsInRange;
}

std::vector<UAVState>
UAVNode::GetOtherUAVStates()
{
    NS_LOG_FUNCTION(this);

    if (m_otherUAVNodes.empty())
    {
        return std::vector<UAVState>();
    }

    std::vector<UAVState> nearbyUAVStates;

    for (const auto& otherUAV : m_otherUAVNodes)
    {
        Ptr<ConstantVelocityMobilityModel> otherMobility = otherUAV->GetObject<ConstantVelocityMobilityModel>();
        Vector otherPosition = otherMobility->GetPosition();
        if ((m_uavPosition - otherPosition).GetLength() <= UAV_UAV_RANGE)
        {
            // std::cout << "UAVNode::GetOtherUAVStates: " << otherUAV->GetUAVState().currentPosition << std::endl;
            UAVState otherState = otherUAV->GetUAVState();
            nearbyUAVStates.push_back(otherState);
        }
    }

    return nearbyUAVStates;
}

void UAVNode::SetOtherUAVNodes(const std::vector<UAVNode*>& otherUAVNodes)
{
    NS_LOG_FUNCTION(this << &otherUAVNodes);
    m_otherUAVNodes = otherUAVNodes;
}

double
UAVNode::CalculateNetworkPowerLoss()
{
    NS_LOG_FUNCTION(this);

    Ptr<BasicEnergySource> energySource = this->GetObject<BasicEnergySource>();
    double remainingEnergy = energySource->GetRemainingEnergy();
    m_networkPower = m_prevNetworkPower - remainingEnergy;
    m_prevNetworkPower = remainingEnergy;
    return m_networkPower;
}

double
UAVNode::CalculatePropulsionPowerLoss()
{
    NS_LOG_FUNCTION(this);

    // Simplified propulsion power loss calculation
    Vector netVelVec = m_velocityVector - m_resistanceVector;
    double propulsionPowerLoss = std::pow(netVelVec.GetLength(), 3);
    // std::cout << "Agent ID: " << GetId() << ", Propulsion Power Loss: " << propulsionPowerLoss << std::endl;
    m_propulsionPower = propulsionPowerLoss*0.001;
    return m_propulsionPower;
}

std::vector<std::pair<Vector, double>>
UAVNode::GetUniqueCentroids(const std::vector<std::pair<Vector, double>>& centroids, double threshold) const
{
    NS_LOG_FUNCTION(this << &centroids << threshold);

    std::vector<std::pair<Vector, double>> uniqueCentroids;

    for (const auto& centroid : centroids)
    {
        bool isUnique = true;

        for (const auto& uniqueCentroid : uniqueCentroids)
        {
            double distance = (centroid.first - uniqueCentroid.first).GetLength();
            if (distance < threshold)
            {
                isUnique = false;
                break;
            }
        }

        if (isUnique)
        {
            uniqueCentroids.push_back(centroid);
        }
    }

    return uniqueCentroids;
}

} // namespace ns3