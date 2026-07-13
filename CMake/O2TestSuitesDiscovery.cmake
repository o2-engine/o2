# Executed by ctest at startup (via TEST_INCLUDE_FILES) for each test target when
# O2_TESTS_BATCH=ON. Lists gtest suites and registers one ctest test per suite.
# Expects: _o2_target, _o2_exe, _o2_workdir, _o2_tests_file, _o2_include_file.

if(NOT EXISTS "${_o2_exe}")
    add_test(${_o2_target}_NOT_BUILT ${_o2_target}_NOT_BUILT)
    return()
endif()

if(NOT EXISTS "${_o2_tests_file}"
   OR "${_o2_exe}" IS_NEWER_THAN "${_o2_tests_file}"
   OR "${_o2_include_file}" IS_NEWER_THAN "${_o2_tests_file}")

    execute_process(
        COMMAND "${_o2_exe}" --gtest_list_tests
        WORKING_DIRECTORY "${_o2_workdir}"
        TIMEOUT 30
        OUTPUT_VARIABLE _o2_list_output
        ERROR_VARIABLE _o2_list_error
        RESULT_VARIABLE _o2_list_result
    )

    if(NOT _o2_list_result EQUAL 0)
        message(WARNING "Suite discovery for ${_o2_target} failed (${_o2_list_result}): ${_o2_list_error}")
        add_test(${_o2_target}_DISCOVERY_FAILED ${_o2_target}_DISCOVERY_FAILED)
        return()
    endif()

    set(_o2_script "")
    string(REPLACE "\n" ";" _o2_lines "${_o2_list_output}")
    foreach(_o2_line ${_o2_lines})
        # Suite lines start at column 0 and end with '.' (typed/parameterized ones may
        # carry a '# TypeParam = ...' tail); individual tests are indented.
        if(_o2_line MATCHES "^([A-Za-z_][A-Za-z0-9_/]*)\\.")
            set(_o2_suite "${CMAKE_MATCH_1}")
            # Target prefix keeps names unique: several binaries share suite names (Actor, ...)
            set(_o2_name "${_o2_target}/${_o2_suite}")
            string(APPEND _o2_script
                "add_test([=[${_o2_name}]=] [=[${_o2_exe}]=] [==[--gtest_filter=${_o2_suite}.*]==])\n"
                "set_tests_properties([=[${_o2_name}]=] PROPERTIES WORKING_DIRECTORY [=[${_o2_workdir}]=])\n")
            if(_o2_suite MATCHES "^DISABLED_")
                string(APPEND _o2_script
                    "set_tests_properties([=[${_o2_name}]=] PROPERTIES DISABLED TRUE)\n")
            endif()
        endif()
    endforeach()

    file(WRITE "${_o2_tests_file}" "${_o2_script}")
endif()

include("${_o2_tests_file}")
