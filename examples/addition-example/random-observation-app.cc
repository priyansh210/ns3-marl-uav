#include "random-observation-app.h"

#include <ns3/base-test.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RandomObservationApp");

RandomObservationApp::RandomObservationApp()
{
    NS_LOG_FUNCTION(this);

    m_uv = CreateObject<UniformRandomVariable>();
}

TypeId
RandomObservationApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RandomObservationApp")
                            .SetParent<ObservationApplication>()
                            .AddConstructor<RandomObservationApp>();
    return tid;
}

void
RandomObservationApp::Observe()
{
    auto random_number = m_uv->GetInteger(0, 9);
    NS_LOG_INFO("Random number: " << random_number);
    auto observation = CreateObject<OpenGymDiscreteContainer>(random_number);
    observation->SetValue(random_number);
    Send(MakeDictContainer("random_number", observation));
    Simulator::Schedule(Seconds(1.0), &RandomObservationApp::Observe, this);
}

void
RandomObservationApp::RegisterCallbacks()
{
    NS_LOG_FUNCTION(this);
    Simulator::Schedule(Seconds(1.0), &RandomObservationApp::Observe, this);
}

void
RandomObservationApp::AssignStreams(int64_t streamNumber)
{
    m_uv->SetStream(streamNumber);
}

NS_OBJECT_ENSURE_REGISTERED(RandomObservationApp);

} // namespace ns3
