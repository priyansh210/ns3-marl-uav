#include "history-container.h"

#include <iterator>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HistoryContainer");

TimestampedData::TimestampedData(Ptr<OpenGymDictContainer> data, bool trackNs3Time)
    : data(data),
      timestamp(std::chrono::system_clock::now())
{
    if (trackNs3Time)
    {
        ns3timestamp = Simulator::Now().ToInteger(Time::Unit::MS);
    }
    else
    {
        ns3timestamp = -1;
    }
}

void
TimestampedDataDeque::Push(TimestampedData value)
{
    m_deque.push_back(value);
}

void
TimestampedDataDeque::PopNewest(uint count)
{
    while (count > 0 && !m_deque.empty())
    {
        m_deque.pop_back();
        count--;
    }
}

void
TimestampedDataDeque::PopOldest(uint count)
{
    while (count > 0 && !m_deque.empty())
    {
        m_deque.pop_front();
        count--;
    }
}

std::vector<TimestampedData*>
TimestampedDataDeque::GetOldest(uint count)
{
    std::vector<TimestampedData*> firstElements;
    uint i = 0;
    for (auto it = m_deque.begin(); it != m_deque.end() && i < count; ++it, ++i)
    {
        firstElements.push_back(&*it);
    }
    return firstElements;
}

std::vector<TimestampedData*>
TimestampedDataDeque::GetNewest(uint count)
{
    std::vector<TimestampedData*> lastElements;
    uint i = 0;
    for (auto it = m_deque.rbegin(); it != m_deque.rend() && i < count; ++it, ++i)
    {
        lastElements.push_back(&*it);
    }
    return lastElements;
}

std::vector<TimestampedData*>
TimestampedDataDeque::GetAll()
{
    return GetNewest(Size());
}

void
TimestampedDataDeque::Clear()
{
    m_deque.clear();
}

uint
TimestampedDataDeque::Size() const
{
    return m_deque.size();
}

HistoryContainer::HistoryContainer(uint historyLength, bool trackNs3Time)
    : m_historyCount{0},
      m_historyLength{historyLength},
      m_trackNs3Time{trackNs3Time}
{
}

HistoryContainer::~HistoryContainer()
{
}

uint
HistoryContainer::GetHistoryCount() const
{
    return m_historyCount;
}

void
HistoryContainer::Push(Ptr<OpenGymDictContainer> obs, uint id)
{
    if (m_histories.find(id) == m_histories.end())
    {
        NS_LOG_INFO("No history with id " << id << " found, creating new history.");
        this->m_histories.insert(std::make_pair(id, TimestampedDataDeque()));
        this->m_historyCount++;
    }

    TimestampedData timestampedData = TimestampedData(obs, m_trackNs3Time);
    m_histories[id].Push(timestampedData);
    m_combinedHistory.Push(timestampedData);
    if (m_histories[id].Size() > m_historyLength)
    {
        m_histories[id].PopOldest(1);
    }
    if (m_combinedHistory.Size() > (m_historyLength * m_historyCount))
    {
        m_combinedHistory.PopOldest(1);
    }
}

bool
HistoryContainer::AssertHistoryExists(uint id)
{
    NS_ASSERT_MSG(m_histories.find(id) != m_histories.end(),
                  "No history with id " << id << " found");
    return HistoryExists(id);
}

bool
HistoryContainer::HistoryExists(uint id)
{
    return m_histories.find(id) != m_histories.end();
}

std::map<std::string, AggregatedInfo>
HistoryContainer::AggregateNewest(uint id, uint n)
{
    std::map<std::string, AggregatedInfo> returned_agg;
    auto datavec = GetNewestByID(id, n);
    for (const auto& d : datavec)
    {
        auto dict = d->data;
        std::map<std::string, AggregatedInfo> tmp = GetInfoFromDict(dict);

        for (const auto& key : tmp)
        {
            returned_agg[key.first].UpdateStatistics(tmp[key.first]);
        }
    }
    return returned_agg;
}

std::map<std::string, AggregatedInfo>
HistoryContainer::GetInfoFromDict(Ptr<OpenGymDictContainer> dict)
{
    std::map<std::string, AggregatedInfo> aggregator;

    for (const auto& key : dict->GetKeys())
    {
        // turn data into box and go through object
        Ptr<OpenGymDataContainer> tmp = dict->Get(key);
        auto intbox = tmp->GetObject<OpenGymBoxContainer<uint>>();
        auto floatbox = tmp->GetObject<OpenGymBoxContainer<float>>();
        auto uintbox = tmp->GetObject<OpenGymBoxContainer<uint32_t>>();
        auto doublebox = tmp->GetObject<OpenGymBoxContainer<double>>();
        AggregatedInfo tmpagg;
        // ugly casting because its the fastest way
        if (PeekPointer(intbox))
        {
            for (auto it : intbox->GetData())
            {
                tmpagg.UpdateStatistics((float)it);
            }
        }
        else if (PeekPointer(floatbox))
        {
            for (auto it : floatbox->GetData())
            {
                tmpagg.UpdateStatistics(it);
            }
        }
        else if (PeekPointer(uintbox))
        {
            for (auto it : uintbox->GetData())
            {
                tmpagg.UpdateStatistics((float)it);
            }
        }
        else if (PeekPointer(doublebox))
        {
            for (auto it : doublebox->GetData())
            {
                tmpagg.UpdateStatistics((float)it);
            }
        }
        else
        {
            NS_ABORT_MSG("not implemented!");
        }
        aggregator[key] = tmpagg;
    }
    return aggregator;
}

std::vector<TimestampedData*>
HistoryContainer::GetNewestOfCombinedHistory(uint n)
{
    return m_combinedHistory.GetNewest(n);
}

std::vector<TimestampedData*>
HistoryContainer::GetNewestByID(uint id, uint n)
{
    AssertHistoryExists(id);
    return m_histories[id].GetNewest(n);
}

TimestampedData*
HistoryContainer::GetNewestByID(uint id)
{
    return GetNewestByID(id, 1)[0];
}

TimestampedData*
HistoryContainer::GetNewestOfCombinedHistory()
{
    return GetNewestOfCombinedHistory(1)[0];
}

uint
HistoryContainer::GetSize(uint id)
{
    AssertHistoryExists(id);
    return m_histories[id].Size();
};

uint
HistoryContainer::GetSizeOfHistory()
{
    return m_combinedHistory.Size();
};

void
HistoryContainer::Print(std::ostream& where)
{
    uint i = 0;
    for (auto& [id, history] : m_histories)
    {
        if (history.Size() > 0)
        {
            where << "History at: " << std::to_string(i)
                  << " of type: " << history.GetNewest(1)[0]->data->GetTypeId() << " with size "
                  << history.Size() << "\n";
        }
        i++;
    }
}

void
HistoryContainer::PrintHistory(std::ostream& where, uint id, ns3_ai_gym::SpaceType type)
{
    if (type == ns3_ai_gym::Box)
    {
        for (auto obs : m_histories[id].GetAll())
        {
            std::time_t currentTime = std::chrono::system_clock::to_time_t(obs->timestamp);

            auto dict = obs->data->GetObject<OpenGymDictContainer>();
            for (auto key : dict->GetKeys())
            {
                Ptr<OpenGymDataContainer> tmp = dict->Get(key);
                auto intbox = tmp->GetObject<OpenGymBoxContainer<uint>>();
                auto floatbox = tmp->GetObject<OpenGymBoxContainer<float>>();
                auto uintbox = tmp->GetObject<OpenGymBoxContainer<uint32_t>>();
                auto doublebox = tmp->GetObject<OpenGymBoxContainer<double>>();
                where << "Data collected, id=" << id << ": ";

                // ugly casting because its the fastest way
                if (PeekPointer(intbox))
                {
                    intbox->Print(where);
                }
                else if (PeekPointer(floatbox))
                {
                    floatbox->Print(where);
                }
                else if (uintbox)
                {
                    uintbox->Print(where);
                }
                else if (PeekPointer(doublebox))
                {
                    doublebox->Print(where);
                }
                else
                {
                    NS_ABORT_MSG("not implemented!");
                }
                where << " at time: " << std::ctime(&currentTime);
            }
        }
        where << "\n";
    }
    else
    {
        where << "NOT IMPLEMENTED";
    }
}

void
HistoryContainer::DeleteHistory(uint id)
{
    AssertHistoryExists(id);
    auto history = m_histories.find(id);
    this->m_histories.erase(history);

    this->m_historyCount--;
}
