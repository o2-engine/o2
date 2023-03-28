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
#endif
}

o2::DeviceType GetDeviceType()
{
	auto platform = GetEnginePlatform();
	if (platform == o2::Platform::Windows || platform == o2::Platform::Mac)
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
#if defined PLATFORM_MAC || defined PLATFORM_WINDOWS
	return "../../ProjectSettings.json";
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

bool IsAssetsPrebuildEnabled()
{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC)
	return true;
#else
	return false;
#endif
}

const char* GetProjectRootPath()
{
#if defined PLATFORM_MAC || defined PLATFORM_WINDOWS
	return "../../";
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

const char* GetBuiltAssetsPath()
{
#if defined PLATFORM_WINDOWS
	return "../../BuiltAssets/Windows/Data/";
#elif defined PLATFORM_ANDROID
	return "AndroidAssets/BuiltAssets/";
#elif defined PLATFORM_MAC
	return "../../BuiltAssets/Mac/Data/";
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
	return "../../BuiltAssets/Windows/Data.json";
#elif defined PLATFORM_ANDROID
	return "AndroidAssets/AssetsTree.json";
#elif defined PLATFORM_MAC
	return "../../BuiltAssets/Mac/Data.json";
#elif defined PLATFORM_IOS
	return "Data.json";
#endif
}

const char* GetEditorAssetsPath()
{
#if defined PLATFORM_WINDOWS || defined PLATFORM_MAC
	return "../../deps/o2/Editor/Assets/";
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
#endif
	return "";
}

const char* GetEditorBuiltAssetsTreePath()
{
#if defined PLATFORM_WINDOWS
	return "../../BuiltAssets/Windows/EditorData.json";
#elif defined PLATFORM_MAC
	return "../../BuiltAssets/Mac/EditorData.json";
#endif
	return "";
}

const char* GetBuiltitAssetsPath()
{
#if defined PLATFORM_MAC || defined PLATFORM_WINDOWS
	return "../../deps/o2/Framework/Assets/";
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
