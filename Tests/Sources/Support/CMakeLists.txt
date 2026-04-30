# Shared test support: TestComponent / TestComponent2 (used by both tiers via
# SceneTestHelpers), TestScriptObject (used by Systems), UITestHelpers (used by Rendered).

set(o2TestsSupport_CODEGEN_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/o2TestsSupport.cpp)
add_custom_command(OUTPUT ${o2TestsSupport_CODEGEN_OUTPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../../CodeTool/Bin/o2CodeTool
        -project o2TestsSupport
        -sources "${CMAKE_CURRENT_SOURCE_DIR}"
        -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources/o2/CodeToolCache.xml"
    COMMENT "Run CodeTool for o2TestsSupport"
    DEPENDS o2CodeTool
)

file(GLOB_RECURSE o2TestsSupport_SOURCES "*.cpp" "*.h")
list(APPEND o2TestsSupport_SOURCES ${o2TestsSupport_CODEGEN_OUTPUT})

add_library(o2TestsSupport STATIC ${o2TestsSupport_SOURCES})

target_include_directories(o2TestsSupport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources
)

target_link_libraries(o2TestsSupport PUBLIC o2Framework)

add_dependencies(o2TestsSupport o2Framework o2FrameworkCodegen)

if(MSVC)
    target_compile_options(o2TestsSupport PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2TestsSupport PROPERTIES FOLDER o2/Tests)
