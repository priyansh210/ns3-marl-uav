#include <ns3/defiance-module.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/rl-application-helper.h>
#include <ns3/test.h>

/**
 * \ingroup defiance-tests
 * Test to check if communication connections can be deleted and added after setup
 */

class CommunicationTestCase : public TestCase
{
  public:
    CommunicationTestCase();
    virtual ~CommunicationTestCase();

  private:
    void DoRun() override;
};

CommunicationTestCase::CommunicationTestCase()
    : TestCase("Check if communication connections can be deleted and added after setup")
{
}

CommunicationTestCase::~CommunicationTestCase()
{
}

void
CommunicationTestCase::DoRun()
{
    NodeContainer nodes;
    nodes.Create(3);

    RlApplicationHelper helper(TypeId::LookupByName("ns3::TestObservationApp"));
    helper.SetAttribute("StartTime", TimeValue(Seconds(0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(20)));
    RlApplicationContainer observationApps = helper.Install(nodes.Get(0));
    helper.SetTypeId(TypeId::LookupByName("ns3::TestAgentApp"));
    RlApplicationContainer agentApps = helper.Install({nodes.Get(1), nodes.Get(2)});

    CommunicationHelper commHelper = CommunicationHelper();

    commHelper.SetObservationApps(observationApps);
    commHelper.SetAgentApps(agentApps);
    commHelper.SetIds();

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    auto devices = p2p.Install({nodes.Get(0), nodes.Get(1)});
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    commHelper.AddCommunication(
        {{observationApps.GetId(0),
          agentApps.GetId(0),
          SocketCommunicationAttributes{interfaces.GetAddress(0), interfaces.GetAddress(1)}},
         {observationApps.GetId(0), agentApps.GetId(1), {}}});
    commHelper.Configure();

    Simulator::Schedule(Seconds(3),
                        &TestObservationApp::ExecuteCallback,
                        DynamicCast<TestObservationApp>(observationApps.Get(0)),
                        47.0,
                        -1,
                        -1);
    Simulator::Schedule(Seconds(4), [&] {
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(0))->GetLatest()[0],
                              47.0,
                              "received observation equals sent observation");
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(1))->GetLatest()[0],
                              47.0,
                              "received observation equals sent observation");
    });

    Simulator::Schedule(Seconds(5), [&] {
        commHelper.AddCommunication({{observationApps.GetId(0), agentApps.GetId(0), {}}});
    });

    Simulator::Schedule(Seconds(6),
                        &TestObservationApp::ExecuteCallback,
                        DynamicCast<TestObservationApp>(observationApps.Get(0)),
                        99.0,
                        agentApps.GetId(0).applicationId,
                        1);
    Simulator::Schedule(Seconds(7), [&] {
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(0))->GetLatest()[0],
                              99.0,
                              "received observation equals sent observation");
    });

    Simulator::Schedule(Seconds(10), [&] {
        commHelper.DeleteCommunication(observationApps.GetId(0), agentApps.GetId(1), 0, 0);
    });

    Simulator::Schedule(Seconds(11),
                        &TestObservationApp::ExecuteCallback,
                        DynamicCast<TestObservationApp>(observationApps.Get(0)),
                        65.0,
                        1,
                        -1);
    Simulator::Schedule(Seconds(12), [&] {
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(1))->GetLatest()[0],
                              47.0,
                              "received observation equals sent observation");
    });

    Simulator::Schedule(Seconds(14), [&] {
        commHelper.AddCommunication({{observationApps.GetId(0), agentApps.GetId(1), {}}});
    });

    Simulator::Schedule(Seconds(15),
                        &TestObservationApp::ExecuteCallback,
                        DynamicCast<TestObservationApp>(observationApps.Get(0)),
                        23.0,
                        agentApps.GetId(1).applicationId,
                        0);
    Simulator::Schedule(Seconds(16), [&] {
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(1))->GetLatest()[0],
                              23.0,
                              "received observation equals sent observation");
    });

    Simulator::Schedule(Seconds(17), [&] {
        commHelper.DeleteCommunication(observationApps.GetId(0), agentApps.GetId(0));
        commHelper.DeleteCommunication(observationApps.GetId(0), agentApps.GetId(0), 1, 1);
    });

    Simulator::Schedule(Seconds(18),
                        &TestObservationApp::ExecuteCallback,
                        DynamicCast<TestObservationApp>(observationApps.Get(0)),
                        77.0,
                        -1,
                        -1);
    Simulator::Schedule(Seconds(19), [&] {
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(0))->GetLatest()[0],
                              99.0,
                              "received observation equals sent observation");
        NS_TEST_ASSERT_MSG_EQ(DynamicCast<TestAgentApp>(agentApps.Get(1))->GetLatest()[0],
                              77.0,
                              "received observation equals sent observation");
    });

    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * \ingroup defiance-tests
 *
 * \brief TestSuite for Communication Helper
 */
class CommunicationTestSuite : public TestSuite
{
  public:
    CommunicationTestSuite();
};

CommunicationTestSuite::CommunicationTestSuite()
    : TestSuite("defiance-communication", UNIT)
{
    AddTestCase(new CommunicationTestCase, TestCase::QUICK);
}

static CommunicationTestSuite sCommunicationTestSuite; //!< Static variable for test initialization
