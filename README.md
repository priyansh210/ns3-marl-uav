# DEFIANCE project: How to Simulate MARL Deployments in Realistic Network Scenarios

Our project mainly builds upon the `ns3` network simulator, `ns3-ai` for ML integration.

You can find the design documentation here: <https://DEFIANCE-project.github.io>

## About the ns3-defiance repository

We provide a framework for ML and RL research using ns3.
You can find our [design documentation and user documentation here](https://DEFIANCE-project.github.io).

For a practical example of how our framework is used, see [this Medium article](https://medium.com/@oliver.zimmermann/reinforcement-learning-in-ns3-part-1-698b9c30c0cd). The blog is divided into two parts and demonstrates building a balancing inverted pendulum in a network scenario using our framework.

## Setup the development environment

### Installation with `bake`

See <https://github.com/DEFIANCE-project/bake-defiance> for easy instructions.

### Docker

If you just want a docker container with a built ns3-defiance, head to <https://github.com/DEFIANCE-project/bake-defiance> and follow the steps to build the full image `defiance-full`.

For users with a hpi login, you can add `--build-arg BASE=registry.gitlab.hpi.de/bp_defiance/bake-defiance:latest-full`
after you logged in with `docker login registry.gitlab.hpi.de` instead of building `defiance-full` manually.

For development, we supply a `Dockerfile` here, in which you can build your custom changes to ns3-defiance. It builds upon `defiance-full`, so make sure you have built or downloaded it beforehand.
Then, you can build a docker image containing your local changes with a simple `docker build .`.
The ns3 root directory is at `$NS3_HOME`; the default working directory.
By default, ns3 and ns3-defiance are built directly. To skip the build, add `--build-arg BUILD_NS3=False`.

### Installation without `bake`

The manual way:

Requirements: Depending on your use-case, different dependencies are needed. For a complete list of all possible
development dependencies, refer
to [our devcontainer Dockerfile](https://github.com/DEFIANCE-project/bake-defiance/blob/main/.devcontainer/Dockerfile#L9)

1. Clone ns3 `git clone https://gitlab.com/nsnam/ns-3-dev.git -b ns-3.40`
2. Some of our code needs that the environment variable `NS3_HOME` is set. Set it with `export NS3_HOME=$(pwd)/ns-3-dev`
3. Clone ns3-ai and ns3-defiance into `ns3/contrib`:
    ```shell-c
   cd ns-3-dev/
   git clone https://github.com/DEFIANCE-project/ns3-ai contrib/ai
   git clone https://github.com/DEFIANCE-project/ns3-defiance contrib/defiance
    ```
4. Make sure, you have all dependencies. Running `./ns3 configure --enable-python --enable-examples --enable-tests`
   should succeed.
5. Then, compile ns3-ai to generate the message types with protobuf: `./ns3 build ai`
6. Install the python requirements with `poetry install -C contrib/defiance`
7. Manually install the ns3-ai message
   types: `pip install -e ./contrib/ai/python_utils -e ./contrib/ai/model/gym-interface/py`
8. Compile everything with `./ns3 build`
9. You are now able to start the training of our example scenario, such as `defiance-balance2.5`
   with `run-agent train -n defiance-balance2.5`. See `run-agent --help` for more info.

### Development tools

This repo comes with additional developer tools, which may be installed with `poetry install --with dev`.

- We format and lint our python code with `ruff`. Simply run `ruff check` to lint and `ruff format` to format.
- We enforce type-checking on our code with `mypy`. Simply run it!
- Optional jupyter notebook support can be installed with `poetry`. Simply run `poetry install --with ipynb`!

In order to test ns3, it needs to be configured correctly. Refer to <https://github.com/DEFIANCE-project/bake-defiance>
for a complete command suggestion.

The ns3 testsuites in the `test` directory can be run with `./test.py -s <test-suite>`,
e.g. `./test.py -s defiance-agent-application` for the `defiance-agent-application` testsuite
in `/test/agent-application-test.cc`. For
further information refer to <https://www.nsnam.org/docs/manual/html/how-to-write-tests.html>.

The special ns3-ai tests need to be executed with `./contrib/defiance/tests/run-python-tests.sh`

## Frequent problems

When you run a ns3 simulation which uses ns3-ai, a segmentation fault occurs. A corresponding python agent is required
to run the simulation. For this, you can use the `run-agent` cli program, i.e. `run-agent train` for training with ray
and `run-agent debug` for debugging and `run-agent random` for a random agent. Check out `run-agent -h` for help.
