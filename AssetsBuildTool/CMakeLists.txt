add_library(o2AssetsBuilder STATIC)

add_compile_definitions(${O2_COMPILE_DEFINITIONS_EXPORT})

file(GLOB_RECURSE o2AssetsBuilder_SOURCES 
    "Sources/*.cpp" "Sources/*.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2AssetsBuilder_SOURCES})

target_include_directories(o2AssetsBuilder PUBLIC "Sources")
target_sources(o2AssetsBuilder PRIVATE ${o2AssetsBuilder_SOURCES})

target_link_libraries(o2AssetsBuilder 
    PUBLIC
    o2Framework
)

if(WIN32)
    add_custom_command(TARGET o2AssetsBuilder PRE_BUILD
                       COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../CodeTool/Bin/o2CodeTool 
                            -project AssetsBuildTool 
                            -sources "${CMAKE_CURRENT_SOURCE_DIR}/Sources" 
                            -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources/o2/CodeToolCache.xml"
                       COMMENT "Run CodeTool:"
                            "-project AssetsBuildTool "
                            "-sources \"${CMAKE_CURRENT_SOURCE_DIR}/Sources\" "
                            "-parent_projects \"${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources/o2/CodeToolCache.xml\"")
endif()

add_dependencies(o2AssetsBuilder o2Framework)

if(MSVC)
    target_compile_options(o2AssetsBuilder PUBLIC ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus")
elseif (UNIX)
    target_compile_options(o2AssetsBuilder PUBLIC ${DEMO_WARNING_OPTION} -Wno-pedantic)
endif()