include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
set_target_properties(gtest PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(gtest_main PROPERTIES FOLDER o2/3rdPartyLibs)

# Codegen for Tests (reflection, TestScriptObject etc.)
set(o2Tests_CODEGEN_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Sources/o2Tests.cpp)
add_custom_command(OUTPUT ${o2Tests_CODEGEN_OUTPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../CodeTool/Bin/o2CodeTool
        -project o2Tests
        -sources "${CMAKE_CURRENT_SOURCE_DIR}/Sources"
        -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources/o2/CodeToolCache.xml"
    COMMENT "Run CodeTool for o2Tests"
    DEPENDS o2CodeTool
)

set(o2Tests_SOURCES_DIRS "Sources/tests/*.cpp")
file(GLOB_RECURSE o2Tests_SOURCES ${o2Tests_SOURCES_DIRS})
list(INSERT o2Tests_SOURCES 0 Sources/TestsMain.cpp)
list(APPEND o2Tests_SOURCES ${o2Tests_CODEGEN_OUTPUT})

add_executable(o2Tests ${o2Tests_SOURCES})

target_include_directories(o2Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources
)

target_link_libraries(o2Tests PRIVATE
    GTest::gtest
    o2Framework
)

add_dependencies(o2Tests o2Framework o2FrameworkCodegen)

include(GoogleTest)
gtest_discover_tests(o2Tests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2Tests>
)
