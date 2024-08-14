Data History Container
----------------------
The data history container is used for storage of data in the :code:`AgentApplication`, specifically the latest observations received from :code:`ObservationApplication`\ s and latest rewards received from :code:`RewardApplication`\ s. When creating the history container, specify how much data it should store before deleting old data. It is possile to also specify whether the *ns-3* simulation time should be tracked with every data entry. If the usage of another history container is desired somewhere else, create a new instance of :code:`HistoryContainer`. This can be useful for e.g. inter-agent communication.

The data container generally accepts every form of :code:`OpenGymDictContainer`\ s, but when the included aggregation functions like average, minimum or maximum over the last :code:`n` entries are used, the aggregation functions will assume :code:`OpenGymDictContainer`\ s with :code:`OpenGymBoxContainer`\ s inside for them to work.

By way of the observation history container: It has an individual queue for each :code:`ObservationApplication` that is connected to the :code:`AgentApplication`. It also consists of a queue that contains all observations across :code:`ObservationApplication`\ s. The same applies to the reward history container, but with :code:`RewardApplication`\ s.

In order to add data to the history container, call the method :code:`ns3::HistoryContainer::Push(ns3::Ptr<ns3::OpenGymDictContainer> data, uint id)`, which will add the data to the queue specified through :code:`id`. This doesn't need to be done manually though, as the :code:`AgentApplication` will automatically add the data to the history container when received from the :code:`ObservationApplication`\ s and :code:`RewardApplication`\ s. In order to do the same with agent messages, define a new history container and fill it accordingly in a method derived from :code:`AgentApplication::OnRecvFromAgent`.

In order to get data from the history container, call the method :code:`HistoryContainer::GetNewestByID(uint id, uint n)`, which will return the data from the queue specified through :code:`id`. If necessary, use :code:`n` to specify the number of entries to retrieve. If the newest data across all queues is needed, call the method :code:`HistoryContainer::GetNewestOfCombinedHistory(uint n)`, which will return the latest :code:`n` entries across all queues. Note that this might not retrieve evenly distributed numbers of entries from the queues, but rather the overall newest entries because different queues might be filled at different rates.

To get the average, minimum or maximum over the last :code:`n` entries, call the method :code:`HistoryContainer::AggregateNewest(uint id, uint n)`, which will return the average of the last :code:`n` entries from the queue specified through :code:`id`. This way, we can access the average, minimum or maximum of the last :code:`n` entries for each key of the :code:`OpenGymDictContainer`.

It makes sense to retreive the data from the history container in the :code:`AgentApplication` after the :code:`AgentApplication` has received data from the :code:`ObservationApplication`\ s and :code:`RewardApplication`\ s. Thus, the methods :code:`void OnRecvObs(uint id) override` and :code:`void OnRecvRew(uint id) override` are the right place to retrieve the latest observations and rewards, respectively, or to do other calculations.

For example, retrieve the newest observation from the history container with ID 0 like this:

..  code-block:: c++

    void OnRecvObs(uint id) override
    {
        auto obs = m_obsDataStruct.GetNewestByID(0)
                        ->data->Get("floatObs")
                        ->GetObject<OpenGymBoxContainer<float>>();
        m_observation = obs;
    }

The following code creates a mapping of :code:`AggregatedInfo` of the last 10 entries for each key of the :code:`OpenGymDictContainer`, providing access to average, minimum and maximum values:

..  code-block:: c++

    auto agg = m_obsDataStruct.AggregateNewest(0, 10);
    auto min = agg["floatObs"].GetMin();
    auto max = agg["floatObs"].GetMax();
    auto avg = agg["floatObs"].GetAvg();
