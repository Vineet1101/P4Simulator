check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/p4sim-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME p4sim
    SOURCE_FILES model/p4sim.cc
                 helper/p4sim-helper.cc
    HEADER_FILES model/p4sim.h
                 helper/p4sim-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/p4sim-test-suite.cc
                 ${examples_as_tests_sources}
)
