#include <ns3/command-line.h>
#include <ns3/environment-creator.h>

using namespace ns3;

int
main(int argc, char* argv[])
{
    int num_carts = 1;
    int num_enbs = 1;
    CommandLine cmd;
    cmd.AddValue("num_carts", "Number of carts", num_carts);
    cmd.AddValue("num_enbs", "Number of eNBs", num_enbs);
    cmd.Parse(argc, argv);

    auto env = EnvironmentCreator();
    env.SetupPendulumScenario(num_carts, num_enbs, true);

    return 0;
}
