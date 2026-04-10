#include "o2/stdafx.h"
#include "EngineSettings.h"

#include <filesystem>
#include <map>
#include <vector>

#if defined PLATFORM_MAC
#include <mach-o/dyld.h>
#endif

namespace
{
    namespace fs = std::filesystem;

    bool IsProjectRoot(const fs::path& path)
    {
        if (path.empty())
            return false;

        std::error_code error;
        return fs::exists(path / "ProjectSettings.json", error) &&
               fs::exists(path / "Assets", error) &&
               fs::exists(path / "o2", error);
    }

    const o2::String& ResolveProjectRootPath()
    {
        static o2::String resolvedPath;
        static bool initialized = false;
        if (initialized)
            return resolvedPath;

        initialized = true;

        std::error_code error;
        std::vector<fs::path> candidates;
        fs::path current = fs::current_path(error);
        if (!error)
        {
            candidates.emplace_back(current);
            candidates.emplace_back(current / "..");
            candidates.emplace_back(current / "../..");
        }

#if defined PLATFORM_MAC
        uint32_t executablePathSize = 0;
        _NSGetExecutablePath(nullptr, &executablePathSize);
        std::string executablePath(executablePathSize, '\0');
        if (_NSGetExecutablePath(executablePath.data(), &executablePathSize) == 0)
        {
            fs::path executableDir = fs::path(executablePath.c_str()).parent_path();
            candidates.emplace_back(executableDir);
            candidates.emplace_back(executableDir / "..");
            candidates.emplace_back(executableDir / "../..");
        }
#endif

        for (auto& candidate : candidates)
        {
            fs::path normalized = candidate.lexically_normal();
            if (!IsProjectRoot(normalized))
                continue;

            resolvedPath = normalized.string().c_str();
            if (!resolvedPath.EndsWith("/"))
                resolvedPath += "/";

            return resolvedPath;
        }

        resolvedPath = current.empty() ? "./" : current.lexically_normal().string().c_str();
        if (!resolvedPath.EndsWith("/"))
            resolvedPath += "/";

        return resolvedPath;
    }

    const o2::String& BuildPath(const char* relativePath)
    {
        static std::map<std::string, o2::String> cache;

        auto existing = cache.find(relativePath);
        if (existing != cache.end())
            return existing->second;

        o2::String value = ResolveProjectRootPath() + relativePath;
        value.ReplaceAll("\\", "/");
        return cache.emplace(relativePath, value).first->second;
    }
}

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
#endif
}

o2::DeviceType GetDeviceType()
{
    auto platform = GetEnginePlatform();
    if (platform == o2::Platform::Windows || platform == o2::Platform::Mac || platform == o2::Platform::Linux)
        return o2::DeviceType::PC;
    
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
    return BuildPath("ProjectSettings.json").Data();
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
    return ResolveProjectRootPath().Data();
#else
    return "";
#endif
}

const char* GetAssetsRootPath()
{
    return "Assets/";
}

const char* GetAssetsPath()
{
    return BuildPath("Assets/").Data();
}

const char* GetBuiltAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return BuildPath("BuiltAssets/Windows/Data/").Data();
#elif defined PLATFORM_ANDROID
    return "AndroidAssets/BuiltAssets/";
#elif defined PLATFORM_MAC
    return BuildPath("BuiltAssets/Mac/Data/").Data();
#elif defined PLATFORM_LINUX
    return BuildPath("BuiltAssets/Linux/Data/").Data();
#elif defined PLATFORM_IOS
    return "Data/";
#endif
}

const char* GetBasicAtlasPath()
{
    return "BasicAtlas.atlas";
}

const char* GetBuiltAssetsTreePath()
{
#if defined PLATFORM_WINDOWS
    return BuildPath("BuiltAssets/Windows/Data.json").Data();
#elif defined PLATFORM_ANDROID
    return "AndroidAssets/AssetsTree.json";
#elif defined PLATFORM_MAC
    return BuildPath("BuiltAssets/Mac/Data.json").Data();
#elif defined PLATFORM_LINUX
    return BuildPath("BuiltAssets/Linux/Data.json").Data();
#elif defined PLATFORM_IOS
    return "Data.json";
#endif
}

const char* GetEditorAssetsPath()
{
#if defined PLATFORM_WINDOWS || defined PLATFORM_MAC || defined PLATFORM_LINUX
    return BuildPath("o2/Editor/Assets/").Data();
#else
    return "";
#endif
}

const char* GetEditorBuiltAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return BuildPath("BuiltAssets/Windows/EditorData/").Data();
#elif defined PLATFORM_MAC
    return BuildPath("BuiltAssets/Mac/EditorData/").Data();
#elif defined PLATFORM_LINUX
    return BuildPath("BuiltAssets/Linux/EditorData/").Data();
#endif
    return "";
}

const char* GetEditorBuiltAssetsTreePath()
{
#if defined PLATFORM_WINDOWS
    return BuildPath("BuiltAssets/Windows/EditorData.json").Data();
#elif defined PLATFORM_MAC
    return BuildPath("BuiltAssets/Mac/EditorData.json").Data();
#elif defined PLATFORM_LINUX
    return BuildPath("BuiltAssets/Linux/EditorData.json").Data();
#endif
    return "";
}

const char* GetBuiltinAssetsPath()
{
#if defined PLATFORM_WINDOWS
    return BuildPath("BuiltAssets/Windows/FrameworkData/").Data();
#elif defined PLATFORM_MAC
    return BuildPath("BuiltAssets/Mac/FrameworkData/").Data();
#elif defined PLATFORM_LINUX
    return BuildPath("BuiltAssets/Linux/FrameworkData/").Data();
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
