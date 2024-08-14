.. _defiance-application-helper:

RlApplicationHelper
*******************

As previously introduced, the DEFIANCE framework is mainly structured around user specific :code:`RlApplication`\ s. They are derived from their specific base classes (e.g. :code:`AgentApplication`) and communicate relevant information with one another during the simulation.

To simplify the creation of their instances, the :code:`RlApplicationHelper` class is provided. As with the typical helper classes already present in *ns-3*, it makes the creation of the applications more intuitive.

The following example demonstrates how the :code:`RlApplicationHelper` can be used.

.. code-block:: c++

    RlApplicationHelper helper(TypeId::LookupByName("ns3::MyObservationApp"));

    // the helper allows to set attributes for the applications
    // this is persistent for all the applications that will be created afterwards
    helper.SetAttribute("StartTime", TimeValue(Seconds(0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(10)));

    RlApplicationContainer observationApps = helper.Install(myNodes1);

    helper.SetTypeId("ns3::MyRewardApp");
    RlApplicationContainer rewardApps = helper.Install(myNodes2);

    helper.SetTypeId("ns3::MyActionApp");
    RlApplicationContainer actionApps = helper.Install(myNodes3);

    helper.SetTypeId("ns3::MyAgentApp");
    RlApplicationContainer agentApps = helper.Install(myNodes4);

This example shows the main features of the :code:`RlApplicationHelper`. First of all, it wraps the created application instances in an :code:`RlApplicationContainer`. This container can be used like the standard *ns-3* :code:`ApplicationContainer` to access or iterate over the applications but does not require to cast the applications each time that DEFIANCE-specific functionality is required. Secondly, the helper allows to set attributes for the applications. This enables work with the :code:`TypeId` system, which makes it easy to set default arguments and to work with command line arguments. In the example above, the helper is used to create different types of applications but sets the same start and stop time for all of them.

.. note::

    The :code:`RlApplicationHelper` is not limited to the applications that are provided by the DEFIANCE framework. It can be used with any application that is derived from the :code:`RlApplication` class.
