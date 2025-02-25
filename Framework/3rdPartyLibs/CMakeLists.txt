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

if (WIN32)
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

add_subdirectory(jerryscript)

# tracy
if (O2_TRACY)
    add_subdirectory(tracy)

    set(TRACY_ENABLE ON CACHE BOOL "Enable tracy" FORCE)
    set(TRACY_CALLSTACK ON CACHE BOOL "Enable tracy callstack" FORCE)

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

# spine
set(SPINE_SANITIZE OFF)
add_subdirectory(Spine)

if(MSVC)
    target_compile_options(Box2D PRIVATE "/MP")
    target_compile_options(jerry-core PRIVATE "/MP")
    target_compile_options(jerry-ext PRIVATE "/MP")
    target_compile_options(jerry-port-default PRIVATE "/MP")
    target_compile_options(libpng PRIVATE "/MP")
    target_compile_options(zlib PRIVATE "/MP")
    target_compile_options(FreeType PRIVATE "/MP")
    target_compile_options(pugixml PRIVATE "/MP")
    target_compile_options(spine-cpp PRIVATE "/MP")
endif()

set_target_properties(Box2D PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(jerry-core PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(jerry-ext PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(jerry-port-default PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(amalgam PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(libpng PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(zlib PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(FreeType PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(pugixml PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(spine-cpp PROPERTIES FOLDER o2/3rdPartyLibs)

if(MSVC)
    set_target_properties(boost_stacktrace_windbg_cached PROPERTIES FOLDER o2/3rdPartyLibs)
endif()