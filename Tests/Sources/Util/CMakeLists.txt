set(o2UtilTests_CODEGEN_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/o2UtilTests.cpp)
add_custom_command(OUTPUT ${o2UtilTests_CODEGEN_OUTPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeTool/Bin/o2CodeTool
        -project o2UtilTests
        -sources "${CMAKE_CURRENT_SOURCE_DIR}"
        -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources/o2/CodeToolCache.xml"
    COMMENT "Run CodeTool for o2UtilTests"
    DEPENDS o2CodeTool
)

file(GLOB_RECURSE o2UtilTests_SOURCES "*.cpp")
list(REMOVE_ITEM o2UtilTests_SOURCES ${o2UtilTests_CODEGEN_OUTPUT})
list(INSERT o2UtilTests_SOURCES 0 TestsMain.cpp)
list(REMOVE_DUPLICATES o2UtilTests_SOURCES)
list(APPEND o2UtilTests_SOURCES ${o2UtilTests_CODEGEN_OUTPUT})

add_executable(o2UtilTests ${o2UtilTests_SOURCES})

target_include_directories(o2UtilTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources
)

target_link_libraries(o2UtilTests PRIVATE
    GTest::gtest
    o2Framework
)

add_dependencies(o2UtilTests o2Framework o2FrameworkCodegen)

if(MSVC)
    target_compile_options(o2UtilTests PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2UtilTests PROPERTIES FOLDER o2/Tests)

o2_gtest_discover_tests(o2UtilTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:o2UtilTests>
)
