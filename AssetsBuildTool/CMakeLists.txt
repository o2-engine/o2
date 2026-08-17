add_library(o2AssetsBuilder STATIC)

add_compile_definitions(${O2_COMPILE_DEFINITIONS_EXPORT})

file(GLOB_RECURSE o2AssetsBuilder_SOURCES
    "Sources/*.cpp" "Sources/*.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${o2AssetsBuilder_SOURCES})

# ARM astcenc (Apache 2.0, git submodule pinned to 5.3.0): the ASTC encoder for built assets.
# The CLI sources are excluded — only the codec core is needed. SIMD is enabled per host
# architecture (NEON on arm64, SSE4.1 on x86_64): without it a single atlas page encodes for
# minutes. Non-host targets (wasm) fall back to the portable no-SIMD build.
file(GLOB o2AssetsBuilder_ASTCENC_SOURCES "3rdPartyLibs/astc-encoder/Source/*.cpp")
list(FILTER o2AssetsBuilder_ASTCENC_SOURCES EXCLUDE REGEX "astcenccli_")

# On Apple, CMAKE_OSX_ARCHITECTURES overrides the host processor (cross-building
# x86_64 on arm64 Macs for the cocos2d-x prebuilt libs); fall back to the host otherwise
set(ASTCENC_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _astc_osx_arch_count)
    if(_astc_osx_arch_count EQUAL 1)
        set(ASTCENC_TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    else()
        set(ASTCENC_TARGET_ARCH "universal") # mixed archs — use the portable no-SIMD path
    endif()
endif()

if(NOT EMSCRIPTEN AND ASTCENC_TARGET_ARCH MATCHES "arm64|aarch64|ARM64")
    set(ASTCENC_SIMD_DEFINITIONS "ASTCENC_SSE=0;ASTCENC_AVX=0;ASTCENC_NEON=1;ASTCENC_SVE=0;ASTCENC_POPCNT=0;ASTCENC_F16C=0")
elseif(NOT EMSCRIPTEN AND ASTCENC_TARGET_ARCH MATCHES "x86_64|AMD64")
    set(ASTCENC_SIMD_DEFINITIONS "ASTCENC_SSE=41;ASTCENC_AVX=0;ASTCENC_NEON=0;ASTCENC_SVE=0;ASTCENC_POPCNT=1;ASTCENC_F16C=0")
    if(NOT MSVC) # MSVC allows SSE4.1 intrinsics without extra flags
        set_source_files_properties(${o2AssetsBuilder_ASTCENC_SOURCES} PROPERTIES COMPILE_OPTIONS "-msse4.1;-mpopcnt")
    endif()
else()
    set(ASTCENC_SIMD_DEFINITIONS "ASTCENC_SSE=0;ASTCENC_AVX=0;ASTCENC_NEON=0;ASTCENC_SVE=0;ASTCENC_POPCNT=0;ASTCENC_F16C=0")
endif()

set_source_files_properties(${o2AssetsBuilder_ASTCENC_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "${ASTCENC_SIMD_DEFINITIONS}"
)

target_include_directories(o2AssetsBuilder PUBLIC "Sources" "3rdPartyLibs/astc-encoder/Source")
target_sources(o2AssetsBuilder PRIVATE ${o2AssetsBuilder_SOURCES} ${o2AssetsBuilder_ASTCENC_SOURCES})

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
                            "-parent_projects \"${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Sources/o2/CodeToolCache.xml\"\n")
endif()

add_dependencies(o2AssetsBuilder o2Framework)

if(MSVC)
    target_compile_options(o2AssetsBuilder PUBLIC ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus")
elseif (UNIX)
    target_compile_options(o2AssetsBuilder PUBLIC ${DEMO_WARNING_OPTION} -Wno-pedantic)
endif()