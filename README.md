# DEFIANCE project: How to Simulate MARL Deployments in Realistic Network Scenarios

Our project mainly builds upon the `ns3` network simulator, `ns3-ai` for ML integration.

## Setup the development environment

### Installation with `bake`

See <https://github.com/DEFIANCE-project/bake-defiance> for easy instructions.

### Installation without `bake`

The manual way: Clone ns-3.40 and put ns3-ai and this repo inside the `contrib` folder. Follow their instructions for complete setup. Finally, build it with `ns3 build defiance`.

### Development tools

This repo comes with additional developer tools.

- We format and lint our python code with `ruff`. Simply run `ruff check` to lint and `ruff format` to format.
- We enforce type-checking on our code with `mypy`. Simply run it!
- Optional jupyter notebook support can be installed with `poetry`. Simply run `poetry install --with ipynb`!

In order to test ns3, it needs to be configured correctly. Refer to <https://github.com/DEFIANCE-project/bake-defiance> for a complete command suggestion.

The ns3 testsuites in the `test` directory can be run with `./test.py -s <test-suite>`, e.g. `./test.py -s observation-app` for the `observation-app` testsuite in `/test/observation-application-test.cc`. For further information refer to <https://www.nsnam.org/docs/manual/html/how-to-write-tests.html>.

## About the ns-defiance repository

We are building a framework for ML and RL research using ns3. More information will be added soon.

When you run a ns3 simulation which uses ns3-ai, a segmentation fault occurs. A corresponding python agent is required to run the simulation. For this, you can use the `run-agent` cli program, i.e. `run-agent train` for training with ray and `run-agent debug` for debugging and `run-agent random` for a random agent. Check out `run-agent -h` for help.
