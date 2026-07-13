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
    add_subdirectory(jerryscript)
endif()

# quickjs (quickjs-ng), vendored: core interpreter only, no libc/cli
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

    # Vendored third-party code: never fail the build on toolchain warnings
    if (MSVC)
        target_compile_options(quickjs PRIVATE /w)
    else()
        target_compile_options(quickjs PRIVATE -Wno-everything)
    endif()

    target_link_libraries(quickjs PUBLIC ${CMAKE_DL_LIBS})
endif()

# Android NDK clang is newer than o2's local clang and promotes some warnings
# to errors by default inside jerryscript's own -Werror=all flag group.
# Override per-target, after jerryscript's add_subdirectory() so these come LAST on the command line.
if (ANDROID)
    foreach(_jerry_target jerry-core jerry-ext jerry-port-default)
        if (TARGET ${_jerry_target})
            target_compile_options(${_jerry_target} PRIVATE
                -Wno-error=enum-enum-conversion
                -Wno-error=misleading-indentation
                -Wno-error=sign-conversion
                -Wno-error=conversion
                -Wno-error=shorten-64-to-32
                -Wno-error=format-nonliteral
                -Wno-unknown-warning-option)
        endif()
    endforeach()
endif()

# tracy
if (O2_TRACY)
    add_subdirectory(tracy)

    set(TRACY_ENABLE ON CACHE BOOL "Enable tracy" FORCE)
    set(TRACY_CALLSTACK ON CACHE BOOL "Enable tracy callstack" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "Enable tracy on demand" FORCE)

    add_compile_definitions(TRACY_ENABLE)
    
    set_target_properties(TracyClient PROPERTIES FOLDER o2/3rdPartyLibs)
endif()

# box2d
set(BOX2D_BUILD_STATIC TRUE)
set(BOX2D_VERSION 1)
add_subdirectory(Box2D)

# other libraries
add_subdirectory(libpng)
add_subdirectory(zlib)
add_subdirectory(FreeType)
add_subdirectory(pugixml)
add_subdirectory(boost)
add_subdirectory(miniaudio)

# spine
set(SPINE_SANITIZE OFF)
add_subdirectory(Spine)

if(MSVC)
    target_compile_options(Box2D PRIVATE ${O2_MSVC_MP_FLAG})
    if (TARGET jerry-core)
        target_compile_options(jerry-core PRIVATE ${O2_MSVC_MP_FLAG})
        target_compile_options(jerry-ext PRIVATE ${O2_MSVC_MP_FLAG})
        target_compile_options(jerry-port-default PRIVATE ${O2_MSVC_MP_FLAG})
    endif()
    target_compile_options(libpng PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(zlib PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(FreeType PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(pugixml PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(spine-cpp PRIVATE ${O2_MSVC_MP_FLAG})
    target_compile_options(miniaudio PRIVATE ${O2_MSVC_MP_FLAG})
endif()

set_target_properties(Box2D PROPERTIES FOLDER o2/3rdPartyLibs)
if (TARGET jerry-core)
    set_target_properties(jerry-core PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(jerry-ext PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(jerry-port-default PROPERTIES FOLDER o2/3rdPartyLibs)
    set_target_properties(amalgam PROPERTIES FOLDER o2/3rdPartyLibs)
endif()
set_target_properties(libpng PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(zlib PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(FreeType PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(pugixml PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(spine-cpp PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(miniaudio PROPERTIES FOLDER o2/3rdPartyLibs)

if(MSVC)
    set_target_properties(boost_stacktrace_windbg_cached PROPERTIES FOLDER o2/3rdPartyLibs)
endif()