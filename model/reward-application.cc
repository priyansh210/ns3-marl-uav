#include "reward-application.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RewardApplication");

RewardApplication::RewardApplication()
    : DataCollectorApplication()
{
}

RewardApplication::~RewardApplication()
{
}

TypeId
RewardApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RewardApplication")
                            .SetParent<DataCollectorApplication>()
                            .SetGroupName("defiance");
    return tid;
}

} // namespace ns3
