#include "rl-application-container.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RlApplicationContainer");

Ptr<RlApplication>
RlApplicationContainer::Get(uint32_t index) const
{
    return DynamicCast<RlApplication>(ApplicationContainer::Get(index));
}

RlApplicationId
RlApplicationContainer::GetId(uint32_t index) const
{
    return Get(index)->GetId();
}

} // namespace ns3
