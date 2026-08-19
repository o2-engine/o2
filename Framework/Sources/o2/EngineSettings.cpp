#include "o2/stdafx.h"
#include "EngineSettings.h"

o2::Platform GetEnginePlatform()
{
#ifdef PLATFORM_WINDOWS
    return o2::Platform::Windows;
#elif defined PLATFORM_ANDROID
    return o2::Platform::Android;
#elif defined PLATFORM_MAC
    return o2::Platform::Mac;
#elif defined PLATFORM_IOS
    return o2::Platform::iOS;
#elif defined PLATFORM_LINUX
    return o2::Platform::Linux;
#elif defined PLATFORM_WASM
    return o2::Platform::WebAssembly;
#endif
}

o2::DeviceType GetDeviceType()
{
    auto platform = GetEnginePlatform();
    if (platform == o2::Platform::Windows || platform == o2::Platform::Mac || platform == o2::Platform::Linux)
        return o2::DeviceType::PC;

#ifdef PLATFORM_WASM
    // The web editor is mouse-driven: PC input model gives it the permanent
    // hover cursor (Input ctor adds cursor 0 for PC only) that under-cursor
    // tracking, wheel and right-click dispatch all depend on. The game build
    // keeps the touch model for mobile browsers.
    if constexpr (IS_EDITOR)
        return o2::DeviceType::PC;
#endif

    return o2::DeviceType::Phone;
}

const char* GetProjectPath()
{
    if constexpr (IS_EDITOR)
        return "../..";
    else
        return "AndroidAssets/";
}

const char* GetProjectSettingPath()
{
#if defined PLATFORM_MAC || defined PLATFORM_WINDOWS || defined PLATFORM_LINUX
    return "../../ProjectSettings.json";
#elif defined PLATFORM_WASM
    // The web editor runs from /project/Bin/WebAssembly over a server-synced
    // project tree and uses desktop-style relative paths; the game build keeps
    // the flat preloaded layout at the VFS root
    if constexpr (IS_EDITOR)
        return "../../ProjectSettings.json";
    else
        return "/ProjectSettings.json";
#else
    return "ProjectSettings.json";
#endif
}

bool IsStoppingOnLogErrors()
{
    return false;
}

bool IsUIDebugEnabled()
{
    return false;
}

bool IsDevMode()
{
    return true;
}

bool IsReleaseBuild()
{
#ifdef DEBUG
    return false;
#else
    return true;
#endif
}

bool IsRenderDrawCallsDebugEnabled()
{
    return false;
}

const char* GetProjectRootPath()
{
#if defined PLATFORM_MAC || defined PLATFORM_WINDOWS || defined PLATFORM_LINUX
    return "../../";
#elif defined PLATFORM_WASM
    if constexpr (IS_EDITOR)
        return "../../";
    else
        return "/";
#else
    return "";
#endif
}

const char* GetAssetsRootPath()
{
    return "Assets/";
}

namespace
{
    // Holds a runtime override for the assets directory. Empty means "use the
    // platform default". Static char buffer so GetAssetsPath() can keep returning
    // const char* with stable lifetime.
    char assetsPathOverride[512] = "";
    bool assetsPathOverrideSet = false;
}

const char* GetAssetsPath()
{
    if (assetsPathOverrideSet)
        return assetsPathOverride;

    static char path[256] = "";
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        strcat(path, GetProjectRootPath());
        strcat(path, GetAssetsRootPath());
    }

    return path;
}

void SetAssetsPathOverride(const char* path)
{
    if (!path || path[0] == '\0')
    {
        assetsPathOverride[0] = '\0';
        assetsPathOverrideSet = false;
        return;
    }

    strncpy(assetsPathOverride, path, sizeof(assetsPathOverride) - 1);
    assetsPathOverride[sizeof(assetsPathOverride) - 1] = '\0';
    assetsPathOverrideSet = true;
}

const char* GetBuiltAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return "../../BuiltAssets/Windows/Data/";
#elif defined PLATFORM_ANDROID
    return "Data/";
#elif defined PLATFORM_MAC
    return "../../BuiltAssets/Mac/Data/";
#elif defined PLATFORM_LINUX
    return "../../BuiltAssets/Linux/Data/";
#elif defined PLATFORM_IOS
    return "Data/";
#elif defined PLATFORM_WASM
    if constexpr (IS_EDITOR)
        return "../../BuiltAssets/WebAssembly/Data/";
    else
        return "/Data/";
#endif
}

const char* GetBasicAtlasPath()
{
    return "BasicAtlas.atlas";
}

const char* GetBuiltAssetsTreePath()
{
#if defined PLATFORM_WINDOWS
    return "../../BuiltAssets/Windows/Data.json";
#elif defined PLATFORM_ANDROID
    return "Data.json";
#elif defined PLATFORM_MAC
    return "../../BuiltAssets/Mac/Data.json";
#elif defined PLATFORM_LINUX
    return "../../BuiltAssets/Linux/Data.json";
#elif defined PLATFORM_IOS
    return "Data.json";
#elif defined PLATFORM_WASM
    if constexpr (IS_EDITOR)
        return "../../BuiltAssets/WebAssembly/Data.json";
    else
        return "/Data.json";
#endif
}

const char* GetEditorAssetsPath()
{
#if defined PLATFORM_WINDOWS || defined PLATFORM_MAC || defined PLATFORM_LINUX || defined PLATFORM_WASM
    return "../../o2/Editor/Assets/";
#else
    return "";
#endif
}

const char* GetEditorBuiltAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return "../../BuiltAssets/Windows/EditorData/";
#elif defined PLATFORM_MAC
    return "../../BuiltAssets/Mac/EditorData/";
#elif defined PLATFORM_LINUX
    return "../../BuiltAssets/Linux/EditorData/";
#elif defined PLATFORM_WASM
    return "../../BuiltAssets/WebAssembly/EditorData/";
#endif
    return "";
}

const char* GetEditorBuiltAssetsTreePath()
{
#if defined PLATFORM_WINDOWS
    return "../../BuiltAssets/Windows/EditorData.json";
#elif defined PLATFORM_MAC
    return "../../BuiltAssets/Mac/EditorData.json";
#elif defined PLATFORM_LINUX
    return "../../BuiltAssets/Linux/EditorData.json";
#elif defined PLATFORM_WASM
    return "../../BuiltAssets/WebAssembly/EditorData.json";
#endif
    return "";
}

const char* GetBuiltinAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return "../../BuiltAssets/Windows/FrameworkData/";
#elif defined PLATFORM_MAC
    return "../../BuiltAssets/Mac/FrameworkData/";
#elif defined PLATFORM_LINUX
    return "../../BuiltAssets/Linux/FrameworkData/";
#elif defined PLATFORM_IOS
    return "FrameworkData/";
#elif defined PLATFORM_WASM
    if constexpr (IS_EDITOR)
        return "../../BuiltAssets/WebAssembly/FrameworkData/";
    else
        return "/FrameworkData/";
#elif defined PLATFORM_ANDROID
    return "FrameworkData/";
#else
    return "FrameworkAssets/";
#endif
}

#ifdef PLATFORM_ANDROID

const char* GetAndroidAssetsPath()
{
    return "AndroidAssets/";
}

#endif
