#include "observation-application.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ObservationApplication");

ObservationApplication::ObservationApplication()
    : DataCollectorApplication()
{
}

ObservationApplication::~ObservationApplication()
{
}

TypeId
ObservationApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ObservationApplication")
                            .SetParent<DataCollectorApplication>()
                            .SetGroupName("defiance");
    return tid;
}

} // namespace ns3
