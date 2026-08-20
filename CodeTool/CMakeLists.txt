file(GLOB_RECURSE o2CodeToolLib_SOURCES "Src/*.cpp" "Src/*.h")
list(REMOVE_ITEM o2CodeToolLib_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Src/CodeTool.cpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2CodeToolLib_SOURCES})

# Library with all tool logic, shared between the executable and o2CodeToolTests
add_library(o2CodeToolLib STATIC ${o2CodeToolLib_SOURCES})
target_include_directories(o2CodeToolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Src)

add_executable(o2CodeTool Src/CodeTool.cpp)
target_link_libraries(o2CodeTool PRIVATE o2CodeToolLib)

foreach(target o2CodeToolLib o2CodeTool)
    if(MSVC)
        target_compile_options(${target} PUBLIC ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus")
    elseif (UNIX)
        target_compile_options(${target} PUBLIC ${DEMO_WARNING_OPTION} -Wno-pedantic)
    endif()
endforeach()

set(O2CODE_TOOL_BIN_OUTPUTDIR "${CMAKE_CURRENT_SOURCE_DIR}/Bin")

set_target_properties(o2CodeTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${O2CODE_TOOL_BIN_OUTPUTDIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${O2CODE_TOOL_BIN_OUTPUTDIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${O2CODE_TOOL_BIN_OUTPUTDIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${O2CODE_TOOL_BIN_OUTPUTDIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASEWITHDEBUGINFO ${O2CODE_TOOL_BIN_OUTPUTDIR}
)

set_target_properties(o2CodeToolLib PROPERTIES FOLDER o2/Codegen)

if(MSVC)
  set_property(TARGET o2CodeTool PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:o2CodeTool>")
endif()
