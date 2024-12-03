# Check if stdint.h is supported
check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

include_directories(/home/mm/appinstall/behavioral-model/include)
include_directories(/home/mm/appinstall/spdlog/include)

# Core module construction
build_lib(
    LIBNAME p4sim
    SOURCE_FILES 
        model/p4sim.cc
        model/p4-bridge-channel.cc
        model/p4-controller.cc
        model/p4-topology-reader.cc
        model/p4-switch-core.cc
        model/p4-switch-interface.cc
        model/bridge-p4-net-device.cc
        model/standard-metadata-tag.cc
        model/p4-queue-item.cc
        model/p4-rr-pri-queue-disc.cc
        model/format-utils.cc
        model/switch-api.cc
        helper/p4sim-helper.cc
        helper/priority-port-tag.cc
        helper/global.cc
        helper/p4-exception-handle.cc
        helper/p4-topology-reader-helper.cc
    HEADER_FILES 
        model/p4sim.h
        model/p4-bridge-channel.h
        model/p4-controller.h
        model/p4-topology-reader.h
        model/p4-switch-core.h
        model/p4-switch-interface.h
        model/bridge-p4-net-device.h
        model/standard-metadata-tag.h
        model/p4-queue-item.h
        model/p4-rr-pri-queue-disc.h
        model/format-utils.h
        model/switch-api.h
        model/register_access.h
        helper/p4sim-helper.h
        helper/priority-port-tag.h
        helper/global.h
        helper/p4-exception-handle.h
        helper/p4-topology-reader-helper.h
    LIBRARIES_TO_LINK ${libcore} ${libnetwork}  ${libtraffic-control}
    TEST_SOURCES 
        test/p4sim-test-suite.cc
        test/p4-logic-pri-rl-queue-disc-test-suite.cc
        ${examples_as_tests_sources}
)
