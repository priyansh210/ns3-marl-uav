#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("LTE_LEARNING");

int
main(int argc, char* argv[])
{
    uint16_t numUEs = 2;
    uint16_t numBSs = 1;
    uint32_t maxDistance = 10;
    Time simTime = MilliSeconds(1100);
    Time interPacketInterval = MilliSeconds(100);

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numUEs", "Number of UEs", numUEs);
    cmd.AddValue("numBSs", "Number of BS", numBSs);
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("interPacketInterval", "Inter packet interval", interPacketInterval);
    cmd.AddValue("maxDistance", "The maximum distance from center during setup.", maxDistance);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numUEs);
    ueNodes.Create(numBSs);

    // Now follows everything mobility and movement related
    MobilityHelper mobility;
    // set position of the BSs
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> BsPositionAlloc = CreateObject<ListPositionAllocator>();
    // TODO: currently only suports 1 BS - change if needed
    BsPositionAlloc->Add(Vector(0, 0, 0));
    mobility.SetPositionAllocator(BsPositionAlloc);
    mobility.Install(enbNodes);

    // set position of the UEs randomly in max distance
    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel"); // TODO: ues are not moving currently
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    x->SetAttribute("Min", DoubleValue(0));
    x->SetAttribute("Max", DoubleValue(maxDistance));
    Ptr<ListPositionAllocator> UePositionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < numBSs; i++)
    {
        BsPositionAlloc->Add(Vector(x->GetValue(), x->GetValue(), 0));
    }
    mobility.SetPositionAllocator(BsPositionAlloc);
    mobility.Install(ueNodes);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Attach the UEs to the closest base station
    lteHelper->AttachToClosestEnb(ueLteDevs, enbLteDevs);
    // side effect: the default EPS bearer will be activated
    auto device = ueLteDevs.Get(0)->GetObject<LteUeNetDevice>();
    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        // downlink traffic
        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(interPacketInterval));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(dlClient.Install(remoteHost));

        // uplink traffic
        ++ulPort;
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(interPacketInterval));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));
    }
    // TODO: find out effect of starting later / earlier
    serverApps.Start(MilliSeconds(500));
    clientApps.Start(MilliSeconds(500));
    lteHelper->EnableTraces();
    // Uncomment to enable PCAP tracing
    // p2ph.EnablePcapAll("lena-simple-epc");

    Simulator::Stop(simTime);
    Simulator::Run();

    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
    return 0;
}
