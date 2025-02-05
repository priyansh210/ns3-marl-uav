build_lib_example(
        NAME defiance-addition-example
        SOURCE_FILES
                addition-example/addition-agent-app.cc
                addition-example/no-op-action-app.cc
                addition-example/node-id-reward-app.cc
                addition-example/random-observation-app.cc
                addition-example/simulation.cc
        HEADER_FILES
                addition-example/addition-agent-app.h
                addition-example/no-op-action-app.h
                addition-example/node-id-reward-app.h
                addition-example/random-observation-app.h
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-agent-agent-communication
        SOURCE_FILES agent-agent-communication/agent-agent-communication.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-observation-sharing
        SOURCE_FILES agent-agent-communication/observation-sharing.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-ai-demo
        SOURCE_FILES ai-demo/apb.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-send-to-agent
        SOURCE_FILES application-communication-example/send-to-agent.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
                ${libapplications}
)

build_lib_example(
        NAME defiance-channel-interface
        SOURCE_FILES channel-interface-example/channel-interface-example.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-lte-learning
        SOURCE_FILES
                lte-learning/pc-simulation.cc
                lte-learning/pc-environment.cc
        HEADER_FILES lte-learning/pc-environment.h
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-lte-test
        SOURCE_FILES lte-learning/test.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-pendulum
        SOURCE_FILES pendulum-cart/test-cart.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-balance1
        SOURCE_FILES scenario/balance1-scenario.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
                ${libnetanim}
)

build_lib_example(
        NAME defiance-balance2
        SOURCE_FILES scenario/balance2-scenario.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
                ${libnetanim}
)

build_lib_example(
        NAME defiance-lte-animation
        SOURCE_FILES scenario/basic-lte-animation.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
                ${libnetanim}
)

build_lib_example(
        NAME defiance-environment-creator-example
        SOURCE_FILES scenario/environment-creator-example.cc
        LIBRARIES_TO_LINK
                ${libdefiance}
)

build_lib_example(
        NAME defiance-sumo-test
        SOURCE_FILES sumo-test/sumo-example.cc
        LIBRARIES_TO_LINK
                ${libapplications}
                ${libdefiance}
                ${libinternet}
                ${libmobility}
                ${libnetanim}
)

build_lib_example(
        NAME defiance-sumo-topology-creator
        SOURCE_FILES topology-creation/sumo-topology-creator.cc
        LIBRARIES_TO_LINK
                ${libapplications}
                ${libdefiance}
                ${libinternet}
                ${libmobility}
                ${libnetanim}
)
