# Registers gtest binaries with CTest in one of two modes:
#   O2_TESTS_BATCH=ON  — one ctest entry per gtest suite; the whole suite runs in a single
#                        process (--gtest_filter=Suite.*), amortizing Application/render init.
#   O2_TESTS_BATCH=OFF — classic gtest_discover_tests: one process per test case.
#
# Usage: o2_gtest_discover_tests(<target> WORKING_DIRECTORY <dir>)
# Suite discovery always happens at ctest startup (PRE_TEST style), so binaries are never
# executed during the build.

include(GoogleTest)

function(o2_gtest_discover_tests TARGET)
    cmake_parse_arguments(ARG "" "WORKING_DIRECTORY" "" ${ARGN})

    if(NOT O2_TESTS_BATCH)
        gtest_discover_tests(${TARGET}
            WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
            DISCOVERY_MODE PRE_TEST
        )
        return()
    endif()

    set(include_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_suites_include.cmake")
    set(tests_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_suites_tests.cmake")

    string(CONCAT content
        "set(_o2_target [=[${TARGET}]=])\n"
        "set(_o2_exe [=[$<TARGET_FILE:${TARGET}>]=])\n"
        "set(_o2_workdir [=[${ARG_WORKING_DIRECTORY}]=])\n"
        "set(_o2_tests_file [=[${tests_file}]=])\n"
        "set(_o2_include_file [=[${include_file}]=])\n"
        "include([=[${CMAKE_CURRENT_FUNCTION_LIST_DIR}/O2TestSuitesDiscovery.cmake]=])\n"
    )
    file(GENERATE OUTPUT "${include_file}" CONTENT "${content}")

    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${include_file}")
endfunction()
