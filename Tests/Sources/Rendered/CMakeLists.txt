set(o2RenderTests_CODEGEN_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/o2RenderTests.cpp)
add_custom_command(OUTPUT ${o2RenderTests_CODEGEN_OUTPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeTool/Bin/o2CodeTool
        -project o2RenderTests
        -sources "${CMAKE_CURRENT_SOURCE_DIR}"
        -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources/o2/CodeToolCache.xml ${CMAKE_CURRENT_SOURCE_DIR}/../Support/CodeToolCache.xml"
    COMMENT "Run CodeTool for o2RenderTests"
    DEPENDS o2CodeTool o2TestsSupport
)

file(GLOB_RECURSE o2RenderTests_SOURCES "*.cpp")
list(REMOVE_ITEM o2RenderTests_SOURCES ${o2RenderTests_CODEGEN_OUTPUT})
list(INSERT o2RenderTests_SOURCES 0 TestsMain.cpp)
list(REMOVE_DUPLICATES o2RenderTests_SOURCES)
list(APPEND o2RenderTests_SOURCES ${o2RenderTests_CODEGEN_OUTPUT})

add_executable(o2RenderTests ${o2RenderTests_SOURCES})

target_include_directories(o2RenderTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources
)

target_link_libraries(o2RenderTests PRIVATE
    GTest::gtest
    o2TestsSupport
    o2Framework
    o2AssetsBuilder # built-in texture encoders (ASTC) for the compressed-texture tests
)

add_dependencies(o2RenderTests o2Framework o2FrameworkCodegen o2TestsSupport)

if(MSVC)
    target_compile_options(o2RenderTests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2RenderTests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2RenderTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2RenderTests>
)
