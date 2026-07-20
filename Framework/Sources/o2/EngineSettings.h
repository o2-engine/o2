#pragma once
#include "o2/Utils/Types/CommonTypes.h"

#define DEBUG true

// Enables memory managing
#if defined DEBUG
#define ENABLE_MEMORY_MANAGE true
#else
#define ENABLE_MEMORY_MANAGE false
#endif

// Enables render debugging; off in release — per-call glGetError is very expensive on WebGL
#ifndef NDEBUG
#define RENDER_DEBUG true
#else
#define RENDER_DEBUG false
#endif

#if defined MEMORY_ANALYZE_ENABLE
#define ENABLE_MEMORY_ANALYZE true
#else
#define ENABLE_MEMORY_ANALYZE false
#endif

// Describes that engine running as editor or not
#if defined O2_EDITOR_ENABLED
#define IS_EDITOR true
#else
#define IS_EDITOR false
#endif

// Is enabled scripting engine
#if defined(SCRIPTING_BACKEND_JERRYSCRIPT) || defined(SCRIPTING_BACKEND_BROWSERJS) || defined(SCRIPTING_BACKEND_QUICKJS)
#define IS_SCRIPTING_SUPPORTED true
#else
#define IS_SCRIPTING_SUPPORTED false
#endif

#if defined(O2_PLATFORM_INITIALIZATION_ENABLED)
#define IS_PLATFORM_INITIALIZATION_ENABLED true
#else
#define IS_PLATFORM_INITIALIZATION_ENABLED false
#endif

// Current working platform
o2::Platform GetEnginePlatform();

// Returns current device type
o2::DeviceType GetDeviceType();

// Project config file path. Relative from executable
const char* GetProjectSettingPath();

// Enables stopping on log errors
bool IsStoppingOnLogErrors();

// Enables debug ui rectangles drawing
bool IsUIDebugEnabled();

// Is development mode
bool IsDevMode();

// Is build release
bool IsReleaseBuild();

// Is render draw calls debug enabled; Every drawn entity will be printed in console
bool IsRenderDrawCallsDebugEnabled();

// ----------------------------
// Assets configuration section
// ----------------------------

// Project root path. Relative from executable
const char* GetProjectRootPath();

// Assets path. Relative from project root
const char* GetAssetsRootPath();

// Basic atlas path (from assets path)
const char* GetBasicAtlasPath();

// Assets path. Relative from executable
const char* GetAssetsPath();

// Overrides the assets path that GetAssetsPath() returns. Test runners use this to
// point the engine at a sandbox directory instead of the project's real Assets/.
// Pass an empty string to clear the override and fall back to the platform default.
void SetAssetsPathOverride(const char* path);

// Built assets path with built assets. Relative from executable
const char* GetBuiltAssetsPath();

// Built assets assets tree path
const char* GetBuiltAssetsTreePath();

// Editor's assets path. Relative from executable
const char* GetEditorAssetsPath();

// Editor's built assets path with built assets. Relative from executable
const char* GetEditorBuiltAssetsPath();

// Editor's built assets assets tree path
const char* GetEditorBuiltAssetsTreePath();

// Built in assets path. Relative from executable
const char* GetBuiltinAssetsPath();


// ----------------------
// Platform configuration
// ----------------------

#ifdef PLATFORM_ANDROID

// Returns android assets path prefix
const char* GetAndroidAssetsPath();

#endif

// --------------------
// Other configurations
// --------------------
