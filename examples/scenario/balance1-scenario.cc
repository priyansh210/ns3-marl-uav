#include <ns3/environment-creator.h>
#include <ns3/pendulum-cart.h>

using namespace ns3;

int
main(int argc, char* argv[])
{
    EnvironmentCreator environmentCreator = EnvironmentCreator();
    environmentCreator.CreateCarts(1, 1000);
    auto cart = DynamicCast<PendulumCart>(environmentCreator.GetCartNodes().Get(0));

    // TODO: Implement RL Part here
    return 0;
}
