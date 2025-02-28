project(o2 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)

# Options
option(O2_SELFPROFILE "Enable profiling o2 benchmark." OFF)
option(O2_EDITOR "Enables o2 editor." ON)
option(O2_ASAN "Enables ASAN (address sanitizer)." OFF)
option(O2_TRACY "Enables Tracy profiling" ON)
option(O2_MEMORY_ANALYZE "Enables memory analyzing (slows down)" OFF)

# Common definitions
set(O2_COMPILE_DEFINITIONS SCRIPTING_BACKEND_JERRYSCRIPT _CRT_SECURE_NO_WARNINGS)

if (WIN32)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_WINDOWS)
elseif (APPLE)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_MAC)
    list(APPEND O2_COMPILE_DEFINITIONS BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
elseif (UNIX)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_LINUX)
endif()

if (O2_IS_FILESYSTEM_EXPERIMENTAL)
    list(APPEND O2_COMPILE_DEFINITIONS O2_FILESYSTEM_EXPERIMENTAL)
endif()

if (O2_RENDER_API STREQUAL "gles2")
    list(APPEND O2_COMPILE_DEFINITIONS O2_RENDER_GLES2)
    list(APPEND O2_COMPILE_DEFINITIONS O2_DISABLE_PLATFORM)
endif()

if (O2_SELFPROFILE)
    list(APPEND O2_COMPILE_DEFINITIONS O2_PROFILE_STATS)
endif()

if (O2_EDITOR)
    list(APPEND O2_COMPILE_DEFINITIONS O2_EDITOR_ENABLED)
endif()

if (O2_TRACY)
    list(APPEND O2_COMPILE_DEFINITIONS TRACY_ENABLE)
endif()

if (O2_MEMORY_ANALYZE)
    list(APPEND O2_COMPILE_DEFINITIONS MEMORY_ANALYZE_ENABLE)
endif()

if (APPLE)
    set(O2_PLATFORM "Mac" PARENT_SCOPE)
elseif (UNIX)
    set(O2_PLATFORM "Linux" PARENT_SCOPE)
elseif (WIN32)
    set(O2_PLATFORM "Windows" PARENT_SCOPE)
endif()

set(O2_COMPILE_DEFINITIONS_EXPORT ${O2_COMPILE_DEFINITIONS} PARENT_SCOPE)

# warings and errors
if (MSVC)
    add_compile_options(/W2)   

    if(O2_ASAN)
        add_compile_options(/fsanitize=address)
        add_compile_definitions(_DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION _DISABLE_ITERATOR_ANNOTATION)
    endif()
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-error=format -Wno-error=format-extra-args -Wno-error=conversion \
                         -Wno-error=incompatible-pointer-types -Wno-error=sign-conversion -Wno-error=pointer-sign \
                         -Wno-error=deprecated -Wno-reorder -Wno-unused-variable -Wno-unused-local-typedef \
                         -Wno-undefined-var-template -Wno-overloaded-virtual -Wno-deprecated-declarations \
                         -Wno-delete-non-abstract-non-virtual-dtor -Wno-unused-but-set-variable \
                         -Wno-inconsistent-missing-override -Wno-shorten-64-to-32 -Wno-switch -Wno-delete-incomplete \
                         -Wno-potentially-evaluated-expression")
endif()

# dependencies
add_subdirectory(Framework/3rdPartyLibs)

# code tool
add_subdirectory(CodeTool)

# o2 copy builtin framework assets
add_custom_target(o2CopyBuiltinAssets
                  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Assets" 
                                                             "${CMAKE_CURRENT_SOURCE_DIR}/../BuiltAssets/${O2_PLATFORM}/FrameworkData"
                  COMMENT "Copy o2 builtin assets"
)

# o2 Framework library
add_library(o2Framework STATIC)

file(GLOB_RECURSE o2Framework_SOURCES "Framework/Sources/*.cpp" "Framework/Sources/*.h"
                                      "Framework/Sources/*.m" "Framework/Sources/*.mm")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2Framework_SOURCES})

set(o2Framework_INCLUDES 
        "Framework/Sources"
        "Framework/3rdPartyLibs"
        "Framework/3rdPartyLibs/rapidjson/include"
        "Framework/3rdPartyLibs/FreeType/include"
        "Framework/3rdPartyLibs/jerryscript/jerry-core/include"
        "Framework/3rdPartyLibs/jerryscript"
        "Framework/3rdPartyLibs/boost/stacktrace/include"
        "Framework/3rdPartyLibs/boost/config/include"
        "Framework/3rdPartyLibs/boost/predef/include"
        "Framework/3rdPartyLibs/boost/core/include"
        "Framework/3rdPartyLibs/boost/container_hash/include"
        "Framework/3rdPartyLibs/boost/winapi/include"
        "Framework/"
)

target_sources(o2Framework PRIVATE ${o2Framework_SOURCES})
target_include_directories(o2Framework PUBLIC ${o2Framework_INCLUDES})
target_precompile_headers(o2Framework PUBLIC "Framework/Sources/o2/stdafx.h")
target_compile_definitions(o2Framework PUBLIC ${O2_COMPILE_DEFINITIONS})

target_link_libraries(o2Framework 
    PUBLIC 
        Box2D 
        libpng 
        zlib 
        FreeType 
        pugixml 
        jerry-core 
        jerry-ext 
        jerry-port-default
        spine-cpp
)

if (O2_TRACY)
    list(APPEND o2Framework_INCLUDES "Framework/3rdPartyLibs/tracy/public")
    target_link_libraries(o2Framework PUBLIC Tracy::TracyClient)
endif()

if (O2_MEMORY_ANALYZE)
    target_link_libraries(o2Framework PUBLIC boost_stacktrace_basic)

    if(WIN32)
        target_link_libraries(o2Framework PUBLIC boost_stacktrace_windbg boost_stacktrace_noop)
    endif()
endif()

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
        target_link_libraries(o2Framework PUBLIC ${X11_LIBRARIES})
    endif()

    if (O2_RENDER_API STREQUAL "gles2")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
        find_package(OpenGLES2 REQUIRED)
        target_link_libraries(o2Framework PRIVATE OpenGLES2::OpenGLES2)
    elseif(NOT APPLE)
        find_package(OpenGL REQUIRED COMPONENTS GLX)
        target_link_libraries(o2Framework PUBLIC OpenGL::GL OpenGL::GLX)
    else()
        find_library(CORE_GRAPHICS CoreGraphics)
        find_library(METAL Metal)
        find_library(APP_KIT AppKit)
        find_library(METALKIT MetalKit)
        find_library(FOUNDATION Foundation)
        find_library(OBJC objc)

        target_link_libraries(o2Framework
                PRIVATE
                    ${CORE_GRAPHICS}
                    ${METAL}
                    ${APP_KIT}
                    ${FOUNDATION}
                    ${OBJC}
                    ${METALKIT}
        )
    endif()
endif()

if(WIN32)
    target_link_libraries(o2Framework PRIVATE Shlwapi.lib)
    target_link_libraries(o2Framework PRIVATE opengl32.lib)  
endif()

if (UNIX)
    target_compile_options(o2Framework PRIVATE -Wno-pedantic)
elseif(MSVC)
    target_compile_options(o2Framework PUBLIC "/MP" "/Zc:__cplusplus" "/bigobj")
    target_sources(o2Framework PRIVATE "Framework/Platforms/Windows/Framework.natvis")
endif()

# Codegen
add_custom_target(o2FrameworkCodegen
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Framework -sources "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2"
                  COMMENT "Run CodeTool: ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Framework -sources \"${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2\""
)
add_dependencies(o2FrameworkCodegen o2CodeTool)

# dependencies
add_dependencies(o2Framework Box2D jerry-core jerry-ext jerry-port-default libpng zlib FreeType)
add_dependencies(o2Framework o2CodeTool)
add_dependencies(o2Framework o2CopyBuiltinAssets)
add_dependencies(o2Framework o2FrameworkCodegen)

# o2 Editor Library
if (O2_EDITOR)
    add_library(o2Editor STATIC)

    file(GLOB_RECURSE o2Framework_SOURCES "Editor/Sources/*.cpp" "Editor/Sources/*.h"
                                          "Editor/Sources/*.m" "Editor/Sources/*.mm")

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2Framework_SOURCES})

    target_sources(o2Editor PRIVATE ${o2Framework_SOURCES})
    target_include_directories(o2Editor 
        PUBLIC 
            "Editor/Sources"
            ${o2Framework_INCLUDES}
    )

    target_precompile_headers(o2Editor PUBLIC "Editor/Sources/o2Editor/stdafx.h")
    target_compile_definitions(o2Editor PUBLIC ${O2_COMPILE_DEFINITIONS})
    target_link_libraries(o2Editor PUBLIC o2Framework)

    if(MSVC)
        target_compile_options(o2Editor PRIVATE "/MP" "/Zc:__cplusplus" "/bigobj")
    endif()

    # Codegen
    add_custom_target(o2EditorCodegen
                      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Editor -sources "${CMAKE_CURRENT_SOURCE_DIR}/Editor/Sources/o2Editor" -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/CodeToolCache.xml"
                      COMMENT "Run CodeTool: ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Editor -sources \"${CMAKE_CURRENT_SOURCE_DIR}/Editor/Sources/o2Editor\" -parent_projects \"${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/CodeToolCache.xml\""
    )
    add_dependencies(o2EditorCodegen o2CodeTool)

    add_dependencies(o2Editor o2Framework)
    add_dependencies(o2Editor o2EditorCodegen)
    set_target_properties(o2Editor PROPERTIES FOLDER o2)
    set_target_properties(o2EditorCodegen PROPERTIES FOLDER o2/Codegen)
endif()

# assets build tool
add_subdirectory(AssetsBuildTool)

# group in IDE
set_target_properties(o2Framework PROPERTIES FOLDER o2)
set_target_properties(o2AssetsBuilder PROPERTIES FOLDER o2)
set_target_properties(o2CopyBuiltinAssets PROPERTIES FOLDER o2)
set_target_properties(o2FrameworkCodegen PROPERTIES FOLDER o2/Codegen)
set_target_properties(o2CodeTool PROPERTIES FOLDER o2/Codegen)

