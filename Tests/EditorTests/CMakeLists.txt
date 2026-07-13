set(o2EditorTests_SOURCES_DIRS "Sources/tests/*.cpp")
file(GLOB_RECURSE o2EditorTests_SOURCES ${o2EditorTests_SOURCES_DIRS})
list(INSERT o2EditorTests_SOURCES 0 Sources/TestsMain.cpp)

add_executable(o2EditorTests ${o2EditorTests_SOURCES})

target_include_directories(o2EditorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Editor/Sources
)

target_link_libraries(o2EditorTests PRIVATE
    GTest::gtest
    o2Editor
)

add_dependencies(o2EditorTests o2Editor o2EditorCodegen o2FrameworkCodegen)

if(MSVC)
    target_compile_options(o2EditorTests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2EditorTests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2EditorTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2EditorTests>
)

# Non-headless editor UI test runner (Render + editor styles). Lets tests build real property widgets.
file(GLOB_RECURSE o2EditorUITests_SOURCES "Sources/uitests/*.cpp")

add_executable(o2EditorUITests ${o2EditorUITests_SOURCES})

target_include_directories(o2EditorUITests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Editor/Sources
)

target_link_libraries(o2EditorUITests PRIVATE
    GTest::gtest
    o2Editor
)

add_dependencies(o2EditorUITests o2Editor o2EditorCodegen o2FrameworkCodegen)

if(MSVC)
    target_compile_options(o2EditorUITests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2EditorUITests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2EditorUITests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2EditorUITests>
)
