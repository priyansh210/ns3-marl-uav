#include "rl-application.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RlApplication");

std::string
RlApplicationId::ToString() const
{
    return ApplicationTypeToString(applicationType) + "_" + std::to_string(applicationId);
}

std::string
ApplicationTypeToString(ApplicationType type)
{
    switch (type)
    {
    case AGENT:
        return "agent";
    default:
        return "no_agent";
    }
}

TypeId
RlApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RlApplication").SetParent<Application>().SetGroupName("defiance");
    return tid;
}

RlApplication::RlApplication()
    : m_running(false)
{
}

RlApplication::~RlApplication()
{
}

void
RlApplication::StartApplication()
{
    m_running = true;
}

void
RlApplication::StopApplication()
{
    m_running = false;
}

void
RlApplication::SetId(RlApplicationId id)
{
    m_id = id;
}

RlApplicationId
RlApplication::GetId()
{
    return m_id;
}

void
RlApplication::SetDefaultAddress(Ipv4Address address)
{
    m_defaultAddress = address;
}

Ipv4Address
RlApplication::GetDefaultAddress()
{
    return m_defaultAddress;
}

void
RlApplication::Setup()
{
}

uint
RlApplication::AddObservationInterface(uint32_t remoteAppId, Ptr<ChannelInterface> interface)
{
    NS_ABORT_MSG("This RL app does not support observation communication.");
};

uint
RlApplication::AddRewardInterface(uint32_t remoteAppId, Ptr<ChannelInterface> interface)
{
    NS_ABORT_MSG("This RL app does not support reward communication.");
};

uint
RlApplication::AddActionInterface(uint32_t remoteAppId, Ptr<ChannelInterface> interface)
{
    NS_ABORT_MSG("This RL app does not support action communication.");
};

uint
RlApplication::AddAgentInterface(uint32_t remoteAppId, Ptr<ChannelInterface> interface)
{
    NS_ABORT_MSG("This RL app does not support agent communication.");
}

uint
RlApplication::AddInterface(RlApplicationId applicationId, Ptr<ChannelInterface> interface)
{
    auto appId = applicationId.applicationId;
    switch (applicationId.applicationType)
    {
    case OBSERVATION:
        return AddObservationInterface(appId, interface);
    case REWARD:
        return AddRewardInterface(appId, interface);
    case AGENT:
        return AddAgentInterface(appId, interface);
    case ACTION:
        return AddActionInterface(appId, interface);
    default:
        NS_ABORT_MSG("Unknown application type " << applicationId.applicationType);
    }
}

void
RlApplication::DeleteObservationInterface(uint32_t remoteAppId, uint interfaceId)
{
    NS_ABORT_MSG("This RL app does not support observation communication.");
};

void
RlApplication::DeleteRewardInterface(uint32_t remoteAppId, uint interfaceId)
{
    NS_ABORT_MSG("This RL app does not support reward communication.");
};

void
RlApplication::DeleteActionInterface(uint32_t remoteAppId, uint interfaceId)
{
    NS_ABORT_MSG("This RL app does not support action communication.");
};

void
RlApplication::DeleteAgentInterface(uint32_t remoteAppId, uint interfaceId)
{
    NS_ABORT_MSG("This RL app does not support agent communication.");
}

void
RlApplication::DeleteInterface(RlApplicationId applicationId, uint interfaceId)
{
    auto appId = applicationId.applicationId;
    switch (applicationId.applicationType)
    {
    case OBSERVATION:
        return DeleteObservationInterface(appId, interfaceId);
    case REWARD:
        return DeleteRewardInterface(appId, interfaceId);
    case AGENT:
        return DeleteAgentInterface(appId, interfaceId);
    case ACTION:
        return DeleteActionInterface(appId, interfaceId);
    default:
        NS_ABORT_MSG("Unknown application type " << applicationId.applicationType);
    }
}

void
RlApplication::Send(Ptr<OpenGymDictContainer> data,
                    const std::vector<Ptr<ChannelInterface>>& interfaces)
{
    NS_LOG_FUNCTION(this << data << interfaces);
    if (m_running)
    {
        for (const auto& interface : interfaces)
        {
            interface->Send(data);
        }
    }
}

void
RlApplication::Send(Ptr<OpenGymDictContainer> data,
                    const std::map<uint, Ptr<ChannelInterface>>& interfaces)
{
    NS_LOG_FUNCTION(this << data);
    if (m_running)
    {
        for (const auto& [_, interface] : interfaces)
        {
            interface->Send(data);
        }
    }
}

void
RlApplication::Send(Ptr<OpenGymDictContainer> data, const InterfaceMap& interfaces)
{
    for (const auto& [_, appInterfaces] : interfaces)
    {
        Send(data, appInterfaces);
    }
}

} // namespace ns3
