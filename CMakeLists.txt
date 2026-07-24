if(APPLE)
    project(o2 LANGUAGES C CXX OBJC OBJCXX)
else()
    project(o2 LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

if(APPLE)
    include(CheckLanguage)
    include(MetalShaderSupport)
    check_language(Metal)
    
    if(CMAKE_Metal_COMPILER)
       enable_language(Metal)
    else ()
       message(FATAL_ERROR "Could not find compiler for metal")
    endif()
endif()

# Force off editor and tests for iOS
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(O2_EDITOR OFF CACHE BOOL "" FORCE)
    set(O2_TESTS OFF CACHE BOOL "" FORCE)
endif()

# Force off editor and tests for WebAssembly / Emscripten
if(EMSCRIPTEN)
    set(O2_EDITOR OFF CACHE BOOL "" FORCE)
    set(O2_TESTS OFF CACHE BOOL "" FORCE)
endif()

# Force off editor and tests for Android
if(ANDROID)
    set(O2_EDITOR OFF CACHE BOOL "" FORCE)
    set(O2_TESTS OFF CACHE BOOL "" FORCE)
endif()

# Options
option(O2_SELFPROFILE "Enable profiling o2 benchmark." OFF)
option(O2_TESTS_BATCH "Register one ctest entry per gtest suite (one process per suite) instead of one per test case" ON)
option(O2_EDITOR "Enables o2 editor." ON)
option(O2_ASAN "Enables ASAN (address sanitizer)." OFF)

# Tracy profiling is on by default for development builds (runs in on-demand mode, so it costs almost
# nothing until a profiler connects), and off by default for shipping builds, where its listener thread
# and open port are unwanted. Override explicitly with -DO2_TRACY=ON/OFF
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(O2_TRACY_DEFAULT OFF)
else()
    set(O2_TRACY_DEFAULT ON)
endif()
option(O2_TRACY "Enables Tracy profiling" ${O2_TRACY_DEFAULT})
option(O2_MEMORY_ANALYZE "Enables memory analyzing (slows down)" OFF)
option(O2_PLATFORM_INITIALIZATION_ENABLED "Enables platform initialization" ON)
option(O2_ENABLE_LINK_ZLIB "Enables linking zlib library" ON)
option(O2_SCRIPTING "Enables scripting engine (OFF does not compile yet: reflection core still depends on ScriptValue)" ON)
set(O2_SCRIPTING_BACKEND "" CACHE STRING "Scripting engine backend: quickjs / jerryscript / browserjs; empty picks the platform default (browserjs on WebAssembly, quickjs elsewhere)")
option(O2_ENABLE_LINK_LIBPNG "Enables linking libpng library" ON)
option(O2_DISABLE_O2FRAMEWORK_PCH "Disables precompiled headers for o2Framework" OFF)

if(MSVC AND NOT DEFINED O2_MSVC_MP_FLAG)
    set(O2_MSVC_MP_COUNT "" CACHE STRING "MSVC /MP process count; empty uses compiler default, 0 disables /MP")

    if(O2_MSVC_MP_COUNT STREQUAL "0")
        set(O2_MSVC_MP_FLAG "")
    elseif(O2_MSVC_MP_COUNT STREQUAL "")
        set(O2_MSVC_MP_FLAG "/MP")
    else()
        set(O2_MSVC_MP_FLAG "/MP${O2_MSVC_MP_COUNT}")
    endif()
endif()

# Common definitions
set(O2_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
if (O2_SCRIPTING)
    if (O2_SCRIPTING_BACKEND STREQUAL "")
        if (EMSCRIPTEN)
            set(O2_SCRIPTING_BACKEND "browserjs")
        else()
            set(O2_SCRIPTING_BACKEND "quickjs")
        endif()
    endif()

    if (O2_SCRIPTING_BACKEND STREQUAL "browserjs")
        if (NOT EMSCRIPTEN)
            message(FATAL_ERROR "browserjs scripting backend requires an Emscripten build")
        endif()
        list(APPEND O2_COMPILE_DEFINITIONS SCRIPTING_BACKEND_BROWSERJS)
    elseif (O2_SCRIPTING_BACKEND STREQUAL "quickjs")
        list(APPEND O2_COMPILE_DEFINITIONS SCRIPTING_BACKEND_QUICKJS)
    elseif (O2_SCRIPTING_BACKEND STREQUAL "jerryscript")
        list(APPEND O2_COMPILE_DEFINITIONS SCRIPTING_BACKEND_JERRYSCRIPT)
    else()
        message(FATAL_ERROR "Unknown O2_SCRIPTING_BACKEND: ${O2_SCRIPTING_BACKEND}")
    endif()

    message(STATUS "o2 scripting backend: ${O2_SCRIPTING_BACKEND}")
endif()

if (EMSCRIPTEN)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_WASM)
    list(APPEND O2_COMPILE_DEFINITIONS BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
elseif (ANDROID)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_ANDROID)
    list(APPEND O2_COMPILE_DEFINITIONS BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
elseif (WIN32)
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND O2_COMPILE_DEFINITIONS PLATFORM_IOS)
    list(APPEND O2_COMPILE_DEFINITIONS BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
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
    # On-demand: the client collects nothing until a profiler connects, so it doesn't grow memory
    # while running unconnected
    list(APPEND O2_COMPILE_DEFINITIONS TRACY_ENABLE TRACY_ON_DEMAND)
endif()

if (O2_MEMORY_ANALYZE)
    list(APPEND O2_COMPILE_DEFINITIONS MEMORY_ANALYZE_ENABLE)
endif()

if (O2_PLATFORM_INITIALIZATION_ENABLED)
    list(APPEND O2_COMPILE_DEFINITIONS O2_PLATFORM_INITIALIZATION_ENABLED)
endif()

if (EMSCRIPTEN)
    set(O2_PLATFORM "WebAssembly")
    set(O2_PLATFORM "WebAssembly" PARENT_SCOPE)
elseif (ANDROID)
    set(O2_PLATFORM "Android")
    set(O2_PLATFORM "Android" PARENT_SCOPE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(O2_PLATFORM "iOS")
    set(O2_PLATFORM "iOS" PARENT_SCOPE)
elseif (APPLE)
    set(O2_PLATFORM "Mac")
    set(O2_PLATFORM "Mac" PARENT_SCOPE)
elseif (UNIX)
    set(O2_PLATFORM "Linux")
    set(O2_PLATFORM "Linux" PARENT_SCOPE)
elseif (WIN32)
    set(O2_PLATFORM "Windows")
    set(O2_PLATFORM "Windows" PARENT_SCOPE)
else()
    message(FATAL_ERROR "Unknown platform, please set O2_PLATFORM manually")
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
    set(O2_C_FLAGS "-Wall -Werror")

    if (EMSCRIPTEN)
        # Emscripten's Clang vintage varies between local dev and CI; tolerate unknown
        # warning flags rather than erroring out, and silence diagnostics that older
        # 3rd-party code in this repo trips on.
        set(O2_C_FLAGS "${O2_C_FLAGS} -Wno-unknown-warning-option -Wno-error=misleading-indentation -Wno-error=enum-enum-conversion -Wno-error=unterminated-string-initialization")
    endif()

    if (ANDROID)
        # NDK 28 Clang rejects unknown -Werror= options entirely, so we keep the
        # list minimal and tolerate unknown warning flags rather than erroring out.
        set(O2_C_FLAGS "${O2_C_FLAGS} -Wno-error=misleading-indentation -Wno-error=enum-enum-conversion -Wno-unknown-warning-option")
    endif()
    
    set(O2_CXX_FLAGS "-Wall -Wno-error=format -Wno-error=format-extra-args -Wno-error=conversion \
                         -Wno-error=incompatible-pointer-types -Wno-error=sign-conversion -Wno-error=pointer-sign \
                         -Wno-error=deprecated -Wno-reorder -Wno-unused-variable -Wno-unused-local-typedef \
                         -Wno-undefined-var-template -Wno-overloaded-virtual -Wno-deprecated-declarations \
                         -Wno-delete-non-abstract-non-virtual-dtor -Wno-unused-but-set-variable \
                         -Wno-inconsistent-missing-override -Wno-shorten-64-to-32 -Wno-switch -Wno-delete-incomplete \
                         -Wno-potentially-evaluated-expression -Wno-deprecated-this-capture -Wno-invalid-offsetof")
    
    set(O2_OBJCXX_FLAGS "-Wall -Wno-error=format -Wno-error=format-extra-args -Wno-error=conversion \
                         -Wno-error=incompatible-pointer-types -Wno-error=sign-conversion -Wno-error=pointer-sign \
                         -Wno-error=deprecated -Wno-reorder -Wno-unused-variable -Wno-unused-local-typedef \
                         -Wno-undefined-var-template -Wno-overloaded-virtual -Wno-deprecated-declarations \
                         -Wno-delete-non-abstract-non-virtual-dtor -Wno-unused-but-set-variable \
                         -Wno-inconsistent-missing-override -Wno-shorten-64-to-32 -Wno-switch -Wno-delete-incomplete \
                         -Wno-potentially-evaluated-expression -Wno-deprecated-this-capture -Wno-invalid-offsetof")

    if (EMSCRIPTEN)
        set(O2_CXX_FLAGS "${O2_CXX_FLAGS} -Wno-unknown-warning-option -Wno-error=missing-template-arg-list-after-template-kw -Wno-error=deprecated-declarations -Wno-error=unused-command-line-argument")
    endif()

    if (ANDROID)
        set(O2_CXX_FLAGS "${O2_CXX_FLAGS} -Wno-error=deprecated-declarations -Wno-error=unused-command-line-argument -Wno-unknown-warning-option")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${O2_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${O2_CXX_FLAGS}")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} ${O2_OBJCXX_FLAGS}")
    
    set(O2_C_FLAGS ${O2_C_FLAGS} PARENT_SCOPE)
    set(O2_CXX_FLAGS ${O2_CXX_FLAGS} PARENT_SCOPE)
    set(O2_OBJCXX_FLAGS ${O2_OBJCXX_FLAGS} PARENT_SCOPE)
endif()

# dependencies
add_subdirectory(Framework/3rdPartyLibs)

# code tool (native host-only — skip when cross-compiling to iOS / WebAssembly / Android)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT EMSCRIPTEN AND NOT ANDROID)
    add_subdirectory(CodeTool)
endif()

# o2 copy builtin framework assets
add_custom_target(o2CopyBuiltinAssets
                  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Assets" 
                                                             "${CMAKE_CURRENT_SOURCE_DIR}/../BuiltAssets/${O2_PLATFORM}/FrameworkData"
                  COMMENT "Copy o2 builtin assets from ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Assets to ${CMAKE_CURRENT_SOURCE_DIR}/../BuiltAssets/${O2_PLATFORM}/FrameworkData"
)

# o2 Framework library
add_library(o2Framework STATIC)

set(o2Framework_SOURCES_DIRS "Framework/Sources/*.h" "Framework/Sources/*.cpp")
if (APPLE)
    list(APPEND o2Framework_SOURCES_DIRS "Framework/Sources/*.mm" "Framework/Sources/*.m")
endif()

file(GLOB_RECURSE o2Framework_SOURCES ${o2Framework_SOURCES_DIRS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2Framework_SOURCES})

set(o2Framework_INCLUDES 
        "Framework/Sources"
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
if(NOT O2_DISABLE_O2FRAMEWORK_PCH)
    target_precompile_headers(o2Framework PUBLIC "Framework/Sources/o2/stdafx.h")
endif()
target_compile_definitions(o2Framework PUBLIC ${O2_COMPILE_DEFINITIONS})

target_link_libraries(o2Framework
    PUBLIC
        Box2D
        FreeType
        pugixml
        spine-cpp
        miniaudio
)
if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "jerryscript")
    target_link_libraries(o2Framework PUBLIC jerry-core jerry-ext jerry-port-default)
endif()
if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "quickjs")
    target_link_libraries(o2Framework PUBLIC quickjs)
endif()

if (O2_ENABLE_LINK_LIBPNG)
    target_link_libraries(o2Framework PUBLIC libpng)
endif()

if (O2_ENABLE_LINK_ZLIB)
    target_link_libraries(o2Framework PUBLIC zlib)
endif()

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

if (EMSCRIPTEN)
    target_compile_options(o2Framework PUBLIC "-sUSE_WEBGL2=1" "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2" "-sFULL_ES3=1")
    target_link_options(o2Framework PUBLIC "-sUSE_WEBGL2=1" "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2" "-sFULL_ES3=1")
elseif (ANDROID)
    find_library(ANDROID_EGL_LIB EGL)
    find_library(ANDROID_GLES2_LIB GLESv2)
    find_library(ANDROID_LOG_LIB log)
    find_library(ANDROID_LIB android)
    target_link_libraries(o2Framework
            PRIVATE
                ${ANDROID_EGL_LIB}
                ${ANDROID_GLES2_LIB}
                ${ANDROID_LOG_LIB}
                ${ANDROID_LIB}
    )
elseif (UNIX)
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
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        find_library(METAL Metal)
        find_library(METALKIT MetalKit)
        find_library(FOUNDATION Foundation)
        find_library(OBJC objc)
        find_library(UIKIT UIKit)

        target_link_libraries(o2Framework
                PRIVATE
                    ${METAL}
                    ${FOUNDATION}
                    ${OBJC}
                    ${METALKIT}
                    ${UIKIT}
        )

        set(METAL_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/Render/iOS")
        add_subdirectory(${METAL_SHADERS_DIR})

        target_include_directories(o2Framework
                PRIVATE
                ${METAL_SHADERS_DIR}
        )
    else()
        find_library(CORE_GRAPHICS CoreGraphics)
        find_library(METAL Metal)
        find_library(APP_KIT AppKit)
        find_library(METALKIT MetalKit)
        find_library(FOUNDATION Foundation)
        find_library(OBJC objc)
        find_library(APPLICATION_SERVICES ApplicationServices)
        find_library(CARBON Carbon)

        target_link_libraries(o2Framework
                PRIVATE
                    ${CORE_GRAPHICS}
                    ${METAL}
                    ${APP_KIT}
                    ${FOUNDATION}
                    ${OBJC}
                    ${METALKIT}
                    ${APPLICATION_SERVICES}
                    ${CARBON}
        )

        set(METAL_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/Render/Mac")
        add_subdirectory(${METAL_SHADERS_DIR})

        target_include_directories(o2Framework
                PRIVATE
                ${METAL_SHADERS_DIR}
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
    target_compile_options(o2Framework PUBLIC ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
    target_sources(o2Framework PRIVATE "Framework/Platforms/Windows/Framework.natvis")
endif()

# Codegen (native host-only — skip under iOS / EMSCRIPTEN / Android; assume codegen was run during native build)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT EMSCRIPTEN AND NOT ANDROID)
    add_custom_target(o2FrameworkCodegen
                      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Framework -sources "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2"
                      COMMENT "Run CodeTool: ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Framework -sources \"${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2\""
    )
    add_dependencies(o2FrameworkCodegen o2CodeTool)
endif()

# dependencies
add_dependencies(o2Framework Box2D FreeType)
if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "jerryscript")
    add_dependencies(o2Framework jerry-core jerry-ext jerry-port-default)
endif()
if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "quickjs")
    add_dependencies(o2Framework quickjs)
endif()

if (O2_ENABLE_LINK_LIBPNG)
    add_dependencies(o2Framework libpng)
endif()

add_dependencies(o2Framework o2CopyBuiltinAssets)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT EMSCRIPTEN AND NOT ANDROID)
    add_dependencies(o2Framework o2CodeTool)
    add_dependencies(o2Framework o2FrameworkCodegen)
endif()

# o2 Editor Library
if (O2_EDITOR)
    add_library(o2Editor STATIC)

    set(o2Editor_SOURCES_DIRS "Editor/Sources/*.cpp" "Editor/Sources/*.h")
    if (APPLE)
        list(APPEND o2Editor_SOURCES_DIRS "Editor/Sources/*.mm" "Editor/Sources/*.m")
    endif()

    file(GLOB_RECURSE o2Framework_SOURCES ${o2Editor_SOURCES_DIRS})

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2Framework_SOURCES})

    target_sources(o2Editor PRIVATE ${o2Framework_SOURCES})
    target_include_directories(o2Editor 
        PUBLIC 
            "Editor/Sources"
            ${o2Framework_INCLUDES}
    )

    target_precompile_headers(o2Editor PRIVATE "Editor/Sources/o2Editor/stdafx.h")
    target_compile_definitions(o2Editor PUBLIC ${O2_COMPILE_DEFINITIONS})
    target_link_libraries(o2Editor PUBLIC o2Framework)

    if(MSVC)
        target_compile_options(o2Editor PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
    endif()

    # Codegen
    add_custom_target(o2EditorCodegen
                      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Editor -sources "${CMAKE_CURRENT_SOURCE_DIR}/Editor/Sources/o2Editor" -parent_projects "${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/CodeToolCache.xml"
                      COMMENT "Run CodeTool: ${CMAKE_CURRENT_SOURCE_DIR}/CodeTool/Bin/o2CodeTool -project o2Editor -sources \"${CMAKE_CURRENT_SOURCE_DIR}/Editor/Sources/o2Editor\" -parent_projects \"${CMAKE_CURRENT_SOURCE_DIR}/Framework/Sources/o2/CodeToolCache.xml\""
    )
    add_dependencies(o2EditorCodegen o2CodeTool o2FrameworkCodegen)

    add_dependencies(o2Editor o2Framework)
    add_dependencies(o2Editor o2EditorCodegen)
    set_target_properties(o2Editor PROPERTIES FOLDER o2)
    set_target_properties(o2EditorCodegen PROPERTIES FOLDER o2/Codegen)
endif()

# assets build tool (native host-only — skip when cross-compiling to WebAssembly / Android)
if(NOT EMSCRIPTEN AND NOT ANDROID)
    add_subdirectory(AssetsBuildTool)
endif()

# tests
if(O2_TESTS)
    add_subdirectory(Tests)
endif()

# group in IDE
set_target_properties(o2Framework PROPERTIES FOLDER o2)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT EMSCRIPTEN AND NOT ANDROID)
    set_target_properties(o2AssetsBuilder PROPERTIES FOLDER o2)
    set_target_properties(o2CopyBuiltinAssets PROPERTIES FOLDER o2)
    set_target_properties(o2FrameworkCodegen PROPERTIES FOLDER o2/Codegen)
    set_target_properties(o2CodeTool PROPERTIES FOLDER o2/Codegen)
endif()

