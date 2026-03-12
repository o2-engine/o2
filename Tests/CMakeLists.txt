include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(o2Tests_SOURCES_DIRS "Sources/tests/*.cpp")
file(GLOB_RECURSE o2Tests_SOURCES ${o2Tests_SOURCES_DIRS})
list(INSERT o2Tests_SOURCES 0 Sources/TestsMain.cpp)

add_executable(o2Tests ${o2Tests_SOURCES})

target_include_directories(o2Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources
)

target_link_libraries(o2Tests PRIVATE
    GTest::gtest
    o2Framework
)

add_dependencies(o2Tests o2Framework)

include(GoogleTest)
gtest_discover_tests(o2Tests)
