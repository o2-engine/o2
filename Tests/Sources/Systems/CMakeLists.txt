set(o2SystemTests_CODEGEN_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/o2SystemTests.cpp)
add_custom_command(OUTPUT ${o2SystemTests_CODEGEN_OUTPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeTool/Bin/o2CodeTool
        -project o2SystemTests
        -sources "${CMAKE_CURRENT_SOURCE_DIR}"
        -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources/o2/CodeToolCache.xml ${CMAKE_CURRENT_SOURCE_DIR}/../Support/CodeToolCache.xml"
    COMMENT "Run CodeTool for o2SystemTests"
    DEPENDS o2CodeTool o2TestsSupport
)

file(GLOB_RECURSE o2SystemTests_SOURCES "*.cpp")
list(REMOVE_ITEM o2SystemTests_SOURCES ${o2SystemTests_CODEGEN_OUTPUT})
list(INSERT o2SystemTests_SOURCES 0 TestsMain.cpp)
list(REMOVE_DUPLICATES o2SystemTests_SOURCES)
list(APPEND o2SystemTests_SOURCES ${o2SystemTests_CODEGEN_OUTPUT})

add_executable(o2SystemTests ${o2SystemTests_SOURCES})

target_include_directories(o2SystemTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources
)

target_link_libraries(o2SystemTests PRIVATE
    GTest::gtest
    o2TestsSupport
    o2Framework
)

add_dependencies(o2SystemTests o2Framework o2FrameworkCodegen o2TestsSupport)

if(MSVC)
    target_compile_options(o2SystemTests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2SystemTests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2SystemTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2SystemTests>
)
