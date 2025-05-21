#ifndef UAV_NODE_H
#define UAV_NODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"
#include "ns3/wifi-module.h"

namespace ns3 {

/**
 * \brief A UAV state struct to hold the UAV's current state.
 */
 struct UAVState
 {
    uint32_t nodeId;
     Vector currentPosition;                // Current position of the UAV
     Vector velocityVector;                 // Current velocity vector of the UAV
     double powerLeft;                      // Remaining power percentage
     std::vector<std::pair<Vector, double>> localCentroidsCovered; // Local centroids covered (relative distance, cost)
     std::vector<std::pair<Vector, double>> centroidsDiscovered;   // Centroids discovered (relative distance, cost)
     std::vector<std::pair<Vector, double>> centroidsCovered;      // Centroids covered (relative distance, cost)
     int centroidsInRange;                  // Number of centroids in UAV_EDGE_RANGE
     std::pair<Vector, double> movingTowards;                  // Relative value of the centroid the UAV is moving towards
 };
/**
 * \brief A UAV node class.
 */
class UAVNode : public Node
{
public:
    static TypeId GetTypeId();

    UAVNode(double minX, double maxX, double minY, double maxY, double minZ, double maxZ);
    UAVNode(); // default constructor
    virtual ~UAVNode();

    void SetVelocityVector(Vector velocityVector);
    void SetHeadingAngle(double angle, double scalar);
    void SimulateTimeTic(double deltaT);
    void SetEdgeNodes(NodeContainer* edgeNodesPtr);
    void SetBaseStation(NodeContainer* baseStationPtr);
    void SetClusterPositions(std::vector<Vector>* clusterPositionsPtr);
    void SetOtherUAVNodes(const std::vector<UAVNode*>& otherUAVNodes);
    void SetResistance(Vector resistanceVector); // New method to set air resistance

    UAVState GetUAVState() const; // Method to retrieve the UAV's current state

    void SimulateSensor();       // New method to simulate sensor module
    void SimulateAggregation();  // New method to simulate aggregation of data from neighboring UAVs
    std::vector<UAVState> GetOtherUAVStates(); // Method to retrieve the states of other UAVs
    double CalculateNetworkPowerLoss();    // Calculate power consumed by network
    double CalculatePropulsionPowerLoss(); // Calculate power consumed by propulsion

    // Helper functions to find unique centroids
    std::vector<std::pair<Vector, double>> GetUniqueCentroids(const std::vector<std::pair<Vector, double>>& centroids, double threshold) const;

    TracedCallback<uint32_t, UAVState, double, double, Vector, std::vector<UAVState>> m_reportUavState;

protected:
    virtual void DoDispose();
    Vector GetUnitVec(Vector vec);

    static constexpr double UAV_UAV_RANGE = 150.0;  // Range between UAVs
    static constexpr double UAV_EDGE_RANGE = 80.0; // Range between UAV and edge nodes
    static constexpr double UAV_BASE_RANGE = 300.0; // Range between UAV and base station

    int m_uavRange = 60;
    double m_minAngle = -15.0;
    double m_maxAngle = 15.0;
    double m_minVel = 5.0;
    double m_maxVel = 15.0;
    double m_headingAngle = 0.0;
    Vector m_uavPosition;
    Vector m_velocityVector;
    Vector m_relativePosition;
    Vector m_resistanceVector; // Air resistance vector
    double m_costAtPosition;
    double m_power;
    double m_networkPower; 
    double m_prevNetworkPower;   // Power consumed by network
    double m_propulsionPower; // Power consumed by propulsion
    NodeContainer* m_edgeNodesPtr;
    NodeContainer* m_baseStationPtr;
    std::vector<Vector>* m_clusterPositionsPtr;
    std::vector<Vector> m_relativeDistanceVectors;
    UAVState m_uavState; // Instance of UAVState to store the UAV's current state
    std::vector<UAVNode*> m_otherUAVNodes; // List of references to other UAV nodes
};

} // namespace ns3

#endif // UAV_NODE_H