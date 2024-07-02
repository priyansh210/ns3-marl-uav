#include <ns3/callback.h>
#include <ns3/defiance-module.h>
#include <ns3/rl-application.h>
#include <ns3/test.h>

using namespace ns3;

/**
 * \ingroup defiance-tests
 * Test to check if observation can be sent to a single interface
 */
class ObsAppOneInterfaceTestCase : public RlAppBaseTestCase
{
  public:
    ObsAppOneInterfaceTestCase();
    virtual ~ObsAppOneInterfaceTestCase();

  private:
    void Simulate() override;
};

ObsAppOneInterfaceTestCase::ObsAppOneInterfaceTestCase()
    : RlAppBaseTestCase("Check if observation can be sent to a single interface")
{
}

ObsAppOneInterfaceTestCase::~ObsAppOneInterfaceTestCase()
{
}

void
ObsAppOneInterfaceTestCase::Simulate()
{
    // Create ObservationApp without helper
    auto observationApp = CreateObject<TestObservationApp>();
    for (uint i = 0; i < m_sendingInterfaces.size(); ++i)
    {
        uint appId = (i == 1) ? 2 : 5;
        observationApp->AddInterface({AGENT, appId}, m_sendingInterfaces[i]);
    }
    observationApp->Setup();
    // Install ObservationApp on Node
    m_node->AddApplication(observationApp);

    observationApp->SetStartTime(Seconds(0.0));
    observationApp->SetStopTime(Seconds(10.0));

    // Send observation
    Simulator::Schedule(Seconds(5),
                        &TestObservationApp::ExecuteCallback,
                        observationApp,
                        42.0,
                        5,
                        1);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(0.0, m_receivedData[0], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(0.0, m_receivedData[1], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[2], "received observation equals sent observation");
}

/**
 * \ingroup defiance-tests
 * Test to check if observation can be sent to all interfaces
 */
class ObsAppAllInterfacesTestCase : public RlAppBaseTestCase
{
  public:
    ObsAppAllInterfacesTestCase();
    virtual ~ObsAppAllInterfacesTestCase();

  private:
    void Simulate() override;
};

ObsAppAllInterfacesTestCase::ObsAppAllInterfacesTestCase()
    : RlAppBaseTestCase("Check if observation can be sent to all interfaces")
{
}

ObsAppAllInterfacesTestCase::~ObsAppAllInterfacesTestCase()
{
}

void
ObsAppAllInterfacesTestCase::Simulate()
{
    // Create ObservationApp without helper
    auto observationApp = CreateObject<TestObservationApp>();
    for (uint i = 0; i < m_sendingInterfaces.size(); ++i)
    {
        uint appId = (i == 1) ? 2 : 5;
        observationApp->AddInterface({AGENT, appId}, m_sendingInterfaces[i]);
    }
    observationApp->Setup();
    // Install ObservationApp on Node
    m_node->AddApplication(observationApp);

    observationApp->SetStartTime(Seconds(0.0));
    observationApp->SetStopTime(Seconds(10.0));

    // Send observation
    Simulator::Schedule(Seconds(5),
                        &TestObservationApp::ExecuteCallback,
                        observationApp,
                        42.0,
                        -1,
                        -1);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[0], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[1], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[2], "received observation equals sent observation");
}

/**
 * \ingroup defiance-tests
 * Test to check if observation can be sent to a custom number of interfaces
 */

class ObsAppOneAppTestCase : public RlAppBaseTestCase
{
  public:
    ObsAppOneAppTestCase();
    virtual ~ObsAppOneAppTestCase();

  private:
    void Simulate() override;
};

ObsAppOneAppTestCase::ObsAppOneAppTestCase()
    : RlAppBaseTestCase("Check if observation can be sent to a custom number of interfaces")
{
}

ObsAppOneAppTestCase::~ObsAppOneAppTestCase()
{
}

void
ObsAppOneAppTestCase::Simulate()
{
    // Create ObservationApp without helper
    auto observationApp = CreateObject<TestObservationApp>();
    for (uint i = 0; i < m_sendingInterfaces.size(); ++i)
    {
        uint appId = (i == 1) ? 2 : 5;
        observationApp->AddInterface({AGENT, appId}, m_sendingInterfaces[i]);
    }
    observationApp->Setup();
    // Install ObservationApp on Node
    m_node->AddApplication(observationApp);

    observationApp->SetStartTime(Seconds(0.0));
    observationApp->SetStopTime(Seconds(10.0));

    // Send observation
    Simulator::Schedule(Seconds(5),
                        &TestObservationApp::ExecuteCallback,
                        observationApp,
                        42.0,
                        5,
                        -1);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[0], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(0.0, m_receivedData[1], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[2], "received observation equals sent observation");
}

/**
 * \ingroup defiance-tests
 * Test to check if interfaces can be deleted and added after setup
 */

class ObsAppDynamicTestCase : public RlAppBaseTestCase
{
  public:
    ObsAppDynamicTestCase();
    virtual ~ObsAppDynamicTestCase();

  private:
    void Simulate() override;
};

ObsAppDynamicTestCase::ObsAppDynamicTestCase()
    : RlAppBaseTestCase("Check if interfaces can be deleted and added after setup")
{
}

ObsAppDynamicTestCase::~ObsAppDynamicTestCase()
{
}

void
ObsAppDynamicTestCase::Simulate()
{
    // Create ObservationApp without helper
    auto observationApp = CreateObject<TestObservationApp>();
    uint interfaceId1 = observationApp->AddInterface({AGENT, 72}, m_sendingInterfaces[0]);
    NS_TEST_ASSERT_MSG_EQ(0, interfaceId1, "InterfaceID 0 is set correctly.");

    observationApp->Setup();

    // Install ObservationApp on Node
    m_node->AddApplication(observationApp);

    observationApp->SetStartTime(Seconds(0.0));
    observationApp->SetStopTime(Seconds(20.0));

    // Add interface after Setup
    uint interfaceId2 = observationApp->AddInterface({AGENT, 72}, m_sendingInterfaces[1]);
    NS_TEST_ASSERT_MSG_EQ(1, interfaceId2, "InterfaceID 1 is set correctly.");

    observationApp->DeleteAgentInterface(72, interfaceId1);

    // Send observation
    Simulator::Schedule(Seconds(5),
                        &TestObservationApp::ExecuteCallback,
                        observationApp,
                        42.0,
                        72,
                        -1);

    Simulator::Schedule(Seconds(6),
                        [&] { observationApp->DeleteAgentInterface(72, interfaceId2); });
    Simulator::Schedule(Seconds(7),
                        &TestObservationApp::ExecuteCallback,
                        observationApp,
                        4.0,
                        72,
                        -1);
    Simulator::Schedule(Seconds(10), [&] {
        uint interfaceId3 = observationApp->AddInterface({AGENT, 3}, m_sendingInterfaces[2]);
        NS_TEST_ASSERT_MSG_EQ(0, interfaceId3, "InterfaceID 3 is set correctly.");
        Simulator::Schedule(Seconds(5),
                            &TestObservationApp::ExecuteCallback,
                            observationApp,
                            48.0,
                            3,
                            interfaceId3);
    });

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(0.0, m_receivedData[0], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(42.0, m_receivedData[1], "received observation equals sent observation");
    NS_TEST_ASSERT_MSG_EQ(48.0, m_receivedData[2], "received observation equals sent observation");
}

/**
 * \ingroup defiance-tests
 *
 * \brief TestSuite for ObservationApp
 */
class ObservationAppTestSuite : public TestSuite
{
  public:
    ObservationAppTestSuite();
};

ObservationAppTestSuite::ObservationAppTestSuite()
    : TestSuite("defiance-observation-application-test", UNIT)
{
    AddTestCase(new ObsAppOneInterfaceTestCase, TestCase::QUICK);
    AddTestCase(new ObsAppAllInterfacesTestCase, TestCase::QUICK);
    AddTestCase(new ObsAppOneAppTestCase, TestCase::QUICK);
    AddTestCase(new ObsAppDynamicTestCase, TestCase::QUICK);
}

static ObservationAppTestSuite
    sObservationAppTestSuite; //!< Static variable for test initialization
