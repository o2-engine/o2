project(3rdParties)

# warings and errors
if (MSVC)
    add_compile_options(/W2)
    
    if(O2_ASAN)
        add_compile_options(/fsanitize=address)
        add_compile_definitions(_DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION _DISABLE_ITERATOR_ANNOTATION)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    include(CheckGCCVersion.cmake)

    # GCC 15 compiles C as C23 by default, where the K&R function definitions of zlib (and the like) are errors: the C
    # of the third-party libraries stays gnu17, the default of the GCC before it.
    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_EXTENSIONS ON)

    add_compile_options(-Wall -Wno-error=format -Wno-error=format-extra-args -Wno-error=conversion
                        -Wno-error=incompatible-pointer-types -Wno-error=sign-conversion -Wno-error=pointer-sign
                        -Wno-error=class-memaccess -Wno-error=misleading-indentation)

    if(GCC_MAJOR_INT GREATER_EQUAL 12 AND GCC_MINOR_INT GREATER_EQUAL 1)
        add_compile_options(-Wno-error=dangling-pointer)
    endif()
else()
    add_compile_options(-Wno-self-assign -Wno-parentheses-equality -Wno-shorten-64-to-32
                        -Wno-unused-but-set-variable -Wno-deprecated-non-prototype -Wno-unused-local-typedef
                        -Wno-deprecated-declarations)
endif()

# jerry script
set(JERRY_CMDLINE           OFF CACHE BOOL "" FORCE)
set(JERRY_CMDLINE_TEST      OFF CACHE BOOL "" FORCE)
set(JERRY_CMDLINE_SNAPSHOT  OFF CACHE BOOL "" FORCE)
set(JERRY_LIBFUZZER         OFF CACHE BOOL "" FORCE)
set(JERRY_PORT_DEFAULT      ON  CACHE BOOL "" FORCE)
set(JERRY_EXT               ON  CACHE BOOL "" FORCE)
set(JERRY_MATH              OFF CACHE BOOL "" FORCE)
set(UNITTESTS               OFF CACHE BOOL "" FORCE)
set(DOCTESTS                OFF CACHE BOOL "" FORCE)

if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR ANDROID)
    set(BUILD_SHARED_LIBS         OFF CACHE BOOL "" FORCE)
elseif (UNIX)
    set(BUILD_SHARED_LIBS         ON CACHE BOOL "" FORCE)
endif()

set(ENABLE_AMALGAM            OFF CACHE BOOL "" FORCE)
set(ENABLE_LTO                ON  CACHE BOOL "" FORCE)
set(ENABLE_STRIP              ON  CACHE BOOL "" FORCE)
set(ENABLE_COMPILE_COMMANDS   ON  CACHE BOOL "" FORCE)

set(JERRY_CPOINTER_32_BIT           ON  CACHE BOOL "" FORCE)
set(JERRY_DEBUGGER                  ON  CACHE BOOL "" FORCE)
set(JERRY_ERROR_MESSAGES            ON  CACHE BOOL "" FORCE)
set(JERRY_EXTERNAL_CONTEXT          OFF CACHE BOOL "" FORCE)
set(JERRY_PARSER                    ON  CACHE BOOL "" FORCE)
set(JERRY_LINE_INFO                 ON  CACHE BOOL "" FORCE)
set(JERRY_LOGGING                   ON  CACHE BOOL "" FORCE)
set(JERRY_MEM_STATS                 ON  CACHE BOOL "" FORCE)
set(JERRY_MEM_GC_BEFORE_EACH_ALLOC  OFF CACHE BOOL "" FORCE)
set(JERRY_PARSER_DUMP_BYTE_CODE     OFF CACHE BOOL "" FORCE)
set(JERRY_PROFILE                   "es.next" CACHE BOOL "" FORCE)
set(JERRY_REGEXP_STRICT_MODE        OFF CACHE BOOL "" FORCE)
set(JERRY_REGEXP_DUMP_BYTE_CODE     OFF CACHE BOOL "" FORCE)
set(JERRY_SNAPSHOT_EXEC             OFF CACHE BOOL "" FORCE)
set(JERRY_SNAPSHOT_SAVE             OFF CACHE BOOL "" FORCE)
set(JERRY_SYSTEM_ALLOCATOR          OFF CACHE BOOL "" FORCE)
set(JERRY_VALGRIND                  OFF CACHE BOOL "" FORCE)
set(JERRY_VM_EXEC_STOP              OFF CACHE BOOL "" FORCE)
set(JERRY_GLOBAL_HEAP_SIZE          "(5120)" CACHE BOOL "" FORCE)
set(JERRY_GC_LIMIT                  "(0)"    CACHE BOOL "" FORCE)
set(JERRY_STACK_LIMIT               "(0)"    CACHE BOOL "" FORCE)
set(JERRY_GC_MARK_LIMIT             "(8)"    CACHE BOOL "" FORCE)

if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "jerryscript")
    # jerryscript 2.4.0 asks for CMake 2.8.12, which CMake 4 no longer accepts without a policy minimum
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    add_subdirectory(jerryscript)
    unset(CMAKE_POLICY_VERSION_MINIMUM)

    # The debugger of 2.4.0 asserts on a scope variable that isn't an object; Glue/jerryscript/debugger.c is its copy
    # that skips such values
    get_target_property(_jerry_core_sources jerry-core SOURCES)
    list(TRANSFORM _jerry_core_sources REPLACE ".*debugger/debugger\\.c$" "${CMAKE_CURRENT_SOURCE_DIR}/Glue/jerryscript/debugger.c")
    set_target_properties(jerry-core PROPERTIES SOURCES "${_jerry_core_sources}")
endif()

# quickjs (quickjs-ng, git submodule): core interpreter only, no libc/cli
if (O2_SCRIPTING AND O2_SCRIPTING_BACKEND STREQUAL "quickjs")
    add_library(quickjs STATIC
        quickjs/quickjs.c
        quickjs/libregexp.c
        quickjs/libunicode.c
        quickjs/dtoa.c
    )
    target_include_directories(quickjs PUBLIC quickjs)
    target_compile_definitions(quickjs PRIVATE _GNU_SOURCE)
    set_target_properties(quickjs PROPERTIES C_STANDARD 11 FOLDER o2/3rdPartyLibs)

    if (WIN32)
        target_compile_definitions(quickjs PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
    endif()

    # Third-party code: never fail the build on toolchain warnings
    if (MSVC)
        target_compile_options(quickjs PRIVATE /w)
    else()
        target_compile_options(quickjs PRIVATE -Wno-everything)
    endif()

    target_link_libraries(quickjs PUBLIC ${CMAKE_DL_LIBS})
endif()

# Clang newer than the one of jerryscript 2.4.0 promotes some warnings to errors inside jerryscript's own -Werror flag
# group. Override per-target, after jerryscript's add_subdirectory() so these come LAST on the command line.
if (NOT MSVC)
    foreach(_jerry_target jerry-core jerry-ext jerry-port-default)
        if (TARGET ${_jerry_target})
            target_compile_options(${_jerry_target} PRIVATE
                -Wno-error -Wno-error=all -Wno-error=extra -Wno-error=format-nonliteral -Wno-error=init-self
                -Wno-error=conversion -Wno-error=sign-conversion -Wno-error=format-security
                -Wno-error=missing-declarations -Wno-error=shadow -Wno-error=strict-prototypes -Wno-error=undef
                -Wno-error=old-style-definition -Wno-error=logical-op -Wno-error=enum-enum-conversion
                -Wno-error=misleading-indentation -Wno-error=shorten-64-to-32 -Wno-unknown-warning-option)
        endif()
    endforeach()
endif()

# tracy
if (O2_TRACY)
    # These must be set BEFORE add_subdirectory(tracy) so Tracy's own option() calls pick them up on the
    # first configure — otherwise TracyClient is compiled without on-demand and the client buffers events
    # from startup, growing memory even while no profiler is connected
    set(TRACY_ENABLE ON CACHE BOOL "Enable tracy" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "Enable tracy on demand" FORCE)

    # NOT TRACY_CALLSTACK: it is not "callstack support" (that is automatic), it forces every single
    # ZoneScoped to capture a callstack. Each captured frame then has to be symbolized by the client
    # through dladdr, which walks the whole symbol table of the debug .so - on android that pegs a
    # core in the symbol worker and starves the game. Use ZoneScopedS(depth) where a stack is needed
    set(TRACY_CALLSTACK "" CACHE STRING "Override the callstack collection depth for tracy zones" FORCE)
    set(TRACY_FIBERS ON CACHE BOOL "Enable tracy fibers (coroutines as fiber tracks)" FORCE)

    add_subdirectory(tracy)

    add_compile_definitions(TRACY_ENABLE TRACY_ON_DEMAND TRACY_FIBERS)

    set_target_properties(TracyClient PROPERTIES FOLDER o2/3rdPartyLibs)
endif()

# Box2D 2.3.0 (git submodule): its library sources, headers included as "Box2D/..."
file(GLOB_RECURSE BOX2D_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/box2d/Box2D/Box2D/*.cpp")
add_library(Box2D STATIC ${BOX2D_SOURCES})
target_include_directories(Box2D PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/box2d/Box2D")

# box3d (git submodule) — build the core lib inline (like quickjs), NOT via upstream's own CMake,
# which pulls in samples/tests and forces a static MSVC runtime that clashes with o2's.
# SIMD is auto-selected in src/core.h (SSE2 on x86_64, NEON on aarch64); no flags needed.
file(GLOB BOX3D_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/box3d/src/*.c")
add_library(box3d STATIC ${BOX3D_SOURCES})
target_include_directories(box3d
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/box3d/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/box3d/src")
set_target_properties(box3d PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)
if (EMSCRIPTEN)
    # emscripten's <emmintrin.h> compat requires -msse2 on top of -msimd128; scalar path is safer
    target_compile_definitions(box3d PRIVATE BOX3D_DISABLE_SIMD)
endif()

# libm for sqrtf etc. on non-Apple Unix; threads for box3d's timer/scheduler primitives.
if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(box3d PUBLIC m)
endif()
if (NOT WIN32 AND NOT EMSCRIPTEN)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(box3d PUBLIC Threads::Threads)
endif()

# Third-party code: never fail the build on toolchain warnings.
if (MSVC)
    target_compile_options(box3d PRIVATE /w)
else()
    target_compile_options(box3d PRIVATE -w)
endif()

# zlib (git submodule): the library without its build scripts
set(ZLIB_SOURCES adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c inflate.c
                 inftrees.c trees.c uncompr.c zutil.c)
list(TRANSFORM ZLIB_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/zlib/")
add_library(zlib STATIC ${ZLIB_SOURCES})
target_include_directories(zlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
if (NOT WIN32)
    target_compile_definitions(zlib PUBLIC Z_HAVE_UNISTD_H)
endif()

# libpng 1.6 (git submodule): the library with the prebuilt configuration; the ARM NEON and Intel SSE filters switch
# themselves on by the target's architecture
set(LIBPNG_SOURCES png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c pngrio.c pngrtran.c pngrutil.c pngset.c
                   pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c
                   arm/arm_init.c arm/filter_neon_intrinsics.c arm/palette_neon_intrinsics.c
                   intel/intel_init.c intel/filter_sse2_intrinsics.c)
list(TRANSFORM LIBPNG_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/libpng/")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt"
               "${CMAKE_CURRENT_BINARY_DIR}/libpng/pnglibconf.h" COPYONLY)
add_library(libpng STATIC ${LIBPNG_SOURCES})
# (the configuration stays in the build tree: the kits the portal builds games with carry no paths into it - o2Framework,
# the one place png.h is included, takes it privately)
target_include_directories(libpng PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libpng" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libpng")
set(O2_LIBPNG_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/libpng" PARENT_SCOPE)
target_link_libraries(libpng PUBLIC zlib)

# FreeType (git submodule) by its own CMake: static, its bundled zlib for gzip fonts, no optional dependencies
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(freetype)
unset(BUILD_SHARED_LIBS)
# Users of FreeType see its headers only, not its build tree (the configuration it generates there is for its own sources)
set_target_properties(freetype PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

# pugixml (git submodule), wide character mode
add_library(pugixml STATIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src/pugixml.cpp")
target_include_directories(pugixml PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src")
target_compile_definitions(pugixml PUBLIC PUGIXML_WCHAR_MODE)

add_subdirectory(boost)

# miniaudio (git submodule) with the Vorbis decoder of its extras, no encoders; Apple frameworks are linked, not loaded
add_library(miniaudio STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Glue/miniaudio_impl.c")
target_include_directories(miniaudio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio")
target_compile_definitions(miniaudio PUBLIC MA_NO_ENCODING $<$<BOOL:${APPLE}>:MA_NO_RUNTIME_LINKING>)
if (APPLE)
    find_library(MINIAUDIO_CORE_FOUNDATION CoreFoundation)
    find_library(MINIAUDIO_CORE_AUDIO CoreAudio)
    find_library(MINIAUDIO_AUDIO_TOOLBOX AudioToolbox)
    target_link_libraries(miniaudio PUBLIC ${MINIAUDIO_CORE_FOUNDATION} ${MINIAUDIO_CORE_AUDIO} ${MINIAUDIO_AUDIO_TOOLBOX})

    # On mobile Apple miniaudio manages AVAudioSession, so it must be compiled as Objective-C
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        enable_language(OBJC)
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Glue/miniaudio_impl.c" PROPERTIES LANGUAGE OBJC)

        find_library(MINIAUDIO_AVFOUNDATION AVFoundation)
        target_link_libraries(miniaudio PUBLIC ${MINIAUDIO_AVFOUNDATION})
    endif()
elseif (UNIX AND NOT ANDROID AND NOT EMSCRIPTEN)
    target_link_libraries(miniaudio PUBLIC dl pthread m)
endif()

# pl_mpeg (git submodule), a single header library
add_library(pl_mpeg STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Glue/pl_mpeg_impl.c")
target_include_directories(pl_mpeg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/pl_mpeg")

# Third-party code: never fail the build on toolchain warnings
foreach(_o2_quiet_target Box2D zlib libpng freetype pugixml miniaudio pl_mpeg)
    if (MSVC)
        target_compile_options(${_o2_quiet_target} PRIVATE /w)
    else()
        target_compile_options(${_o2_quiet_target} PRIVATE -w)
    endif()
endforeach()

# libwebp, the decoder only (git submodule, v1.4.0): the editor reads the WebP images AI providers answer with
if (O2_EDITOR)
    file(GLOB LIBWEBP_DECODER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/libwebp/src/dec/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/libwebp/src/dsp/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/libwebp/src/utils/*.c")
    add_library(webpdecoder STATIC ${LIBWEBP_DECODER_SOURCES})
    target_include_directories(webpdecoder
        PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/libwebp/src"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libwebp")
    set_target_properties(webpdecoder PROPERTIES C_STANDARD 11 FOLDER o2/3rdPartyLibs)
    if (MSVC)
        target_compile_options(webpdecoder PRIVATE /w ${O2_MSVC_MP_FLAG})
    else()
        target_compile_options(webpdecoder PRIVATE -w)
    endif()
endif()

# spine-cpp of the Spine runtimes (git submodule, the 4.2 line: the format of Spine 4.2 exports)
file(GLOB SPINE_CPP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/spine-runtimes/spine-cpp/spine-cpp/src/spine/*.cpp")
add_library(spine-cpp STATIC ${SPINE_CPP_SOURCES})
target_include_directories(spine-cpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/spine-runtimes/spine-cpp/spine-cpp/include")
if (MSVC)
    target_compile_definitions(spine-cpp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(spine-cpp PRIVATE /w)
else()
    target_compile_options(spine-cpp PRIVATE -w)
endif()

if(MSVC)
    target_compile_options(Box2D PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(box3d PRIVATE ${O2_MSVC_MP_FLAG})
    if (TARGET jerry-core)
        target_compile_options(jerry-core PRIVATE ${O2_MSVC_MP_FLAG})
        target_compile_options(jerry-ext PRIVATE ${O2_MSVC_MP_FLAG})
        target_compile_options(jerry-port-default PRIVATE ${O2_MSVC_MP_FLAG})
    endif()
    target_compile_options(libpng PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(zlib PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(freetype PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(pugixml PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(spine-cpp PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(miniaudio PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(pl_mpeg PRIVATE ${O2_MSVC_MP_FLAG})
endif()

set_target_properties(Box2D PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(box3d PROPERTIES FOLDER o2/3rdPartyLibs)
if (TARGET jerry-core)
    set_target_properties(jerry-core PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(jerry-ext PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(jerry-port-default PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(amalgam PROPERTIES FOLDER o2/3rdPartyLibs)
endif()
set_target_properties(libpng PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(zlib PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(freetype PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(pugixml PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(spine-cpp PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(miniaudio PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(pl_mpeg PROPERTIES FOLDER o2/3rdPartyLibs)

if(MSVC)
    set_target_properties(boost_stacktrace_windbg_cached PROPERTIES FOLDER o2/3rdPartyLibs)
endif()