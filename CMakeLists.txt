# Check if stdint.h is supported
check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

# Core module construction
build_lib(
    LIBNAME p4sim
    SOURCE_FILES 
        model/p4sim.cc
        model/p4-channel.cc
        model/p4-controller.cc
        model/p4-topology-reader.cc
        model/p4-switch-core.cc
        model/p4-switch-interface.cc
        model/p4-logic-pri-rl-queue-disc.cc
        model/p4-rr-pri-queue-disc.cc
        helper/p4sim-helper.cc
        helper/priority-port-tag.cc
        helper/global.cc
        helper/p4-exception-handle.cc
        helper/p4-topology-reader-helper.cc
    HEADER_FILES 
        model/p4sim.h
        model/p4-channel.h
        model/p4-controller.h
        model/p4-topology-reader.h
        model/p4-switch-core.h
        model/p4-switch-interface.h
        model/p4-logic-pri-rl-queue-disc.h
        model/p4-rr-pri-queue-disc.h
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
