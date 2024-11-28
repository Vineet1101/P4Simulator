# Check if stdint.h is supported
check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

# Enable examples and test files
set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        examples/p4sim-example.cc
        # test/p4sim-examples-test-suite.cc
    )
endif()

# Module core code construction
build_lib(
    LIBNAME p4sim
    SOURCE_FILES 
        model/p4sim.cc
        model/p4-channel.cc
        model/p4-controller.cc
        model/p4-net-device.cc
        model/p4-net-device-core.cc
        model/p4-switch-net-device.cc
        model/p4-switch-net-device-core.cc
        model/p4-switch-interface.cc
        helper/p4sim-helper.cc
        helper/priority-port-tag.cc
        helper/format-utils.cc
        helper/p4-exception-handle.cc
    HEADER_FILES 
        model/p4sim.h
        model/p4-channel.h
        model/p4-controller.h
        model/p4-net-device.h
        model/p4-net-device-core.h
        model/p4-switch-net-device.h
        model/p4-switch-net-device-core.h
        model/p4-switch-interface.h
        helper/p4sim-helper.h
        helper/priority-port-tag.h
        helper/format-utils.h
        helper/p4-exception-handle.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES 
        test/p4sim-test-suite.cc
        ${examples_as_tests_sources}
)

# Build the QueueDisc submodule under Traffic Control
build_lib(
    LIBNAME traffic-control-p4sim
    SOURCE_FILES 
        patches/traffic-control/model/p4-logic-pri-rl-queue-disc.cc
        patches/traffic-control/model/p4-rr-pri-queue-disc.cc
    HEADER_FILES 
        patches/traffic-control/model/p4-logic-pri-rl-queue-disc.h
        patches/traffic-control/model/p4-rr-pri-queue-disc.h
    LIBRARIES_TO_LINK ${libtraffic_control} ${libcore}
    TEST_SOURCES 
        # patches/traffic-control/test/p4-logic-pri-rl-queue-disc-test-suite.cc
)

# Add examples and test targets
if(ENABLE_EXAMPLES)
    add_executable(p4sim-example examples/p4sim-example.cc)
    target_link_libraries(p4sim-example ${libcore} p4sim traffic-control-p4sim)
endif()
