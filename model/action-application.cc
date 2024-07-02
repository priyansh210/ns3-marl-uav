#include "action-application.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ActionApplication");

TypeId
ActionApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ActionApplication").SetParent<RlApplication>().SetGroupName("defiance");
    return tid;
}

ActionApplication::ActionApplication()
    : m_interfaces({})
{
}

ActionApplication::~ActionApplication()
{
}

uint
ActionApplication::AddAgentInterface(uint32_t remoteAppId, Ptr<ChannelInterface> interface)
{
    NS_LOG_FUNCTION(this << interface << remoteAppId);
    uint id;
    if (m_interfaces[remoteAppId].empty())
    {
        id = 0;
    }
    else
    {
        auto it = m_interfaces[remoteAppId].rbegin();
        id = (*it).first + 1;
    }
    m_interfaces[remoteAppId][id] = interface;
    interface->AddRecvCallback(MakeCallback(&ActionApplication::ExecuteAction, this, remoteAppId));
    return id;
}

void
ActionApplication::DeleteAgentInterface(uint32_t remoteAppId, uint interfaceId)
{
    NS_LOG_FUNCTION(this << interfaceId << remoteAppId);
    m_interfaces[remoteAppId][interfaceId]->Disconnect();
    auto it = m_interfaces[remoteAppId].find(interfaceId);
    m_interfaces[remoteAppId].erase(it);
}

} // namespace ns3
