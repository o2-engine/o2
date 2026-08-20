file(GLOB o2CodeToolTests_SOURCES "*.cpp" "*.h")

add_executable(o2CodeToolTests ${o2CodeToolTests_SOURCES})

target_compile_definitions(o2CodeToolTests PRIVATE CODETOOL_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(o2CodeToolTests PRIVATE
    GTest::gtest
    o2CodeToolLib
)

if(MSVC)
    target_compile_options(o2CodeToolTests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus")
endif()

set_target_properties(o2CodeToolTests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2CodeToolTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2CodeToolTests>
)
