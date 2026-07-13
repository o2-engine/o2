include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
set_target_properties(gtest PROPERTIES FOLDER o2/3rdPartyLibs)
set_target_properties(gtest_main PROPERTIES FOLDER o2/3rdPartyLibs)

include(GoogleTest)
include(O2TestSuites)

# Shared helpers for systems and rendered tiers — TestComponent, TestScriptObject,
# SceneTestHelpers, UITestHelpers. Compiled once into a static lib that both tiers link.
add_subdirectory(Sources/Support)

# Three test executables, in order of increasing init cost:
#   o2UtilTests   — no Application, no asset tree load (~70 ms cold start)
#   o2SystemTests — Application::Initialize headless (~135 ms cold start)
#   o2RenderTests — full Application::Initialize with window + render (~640 ms cold start)
add_subdirectory(Sources/Util)
add_subdirectory(Sources/Systems)
add_subdirectory(Sources/Rendered)

if(O2_EDITOR)
    add_subdirectory(EditorTests)
endif()
