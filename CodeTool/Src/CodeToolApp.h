#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "CodeToolCache.h"
#include "CppSyntaxParser.h"
#include "MetaGenerator.h"

class Timer
{
public:
    Timer();

    // Returns time in seconds from construction or last GetDeltaTime() call
    float GetDeltaTime();

private:
    std::chrono::time_point<std::chrono::steady_clock> mLastElapsedTime;
};

// ---------------------
// Code tool application
// ---------------------
class CodeToolApplication
{
public:
    // Sets arguments from main()
    void SetArguments(char** args, int nargs);

    // Generates new reflection
    void Process();

protected:
    string mProjectName; // Project name from -project, names the registrators source and its initialization function
    string mSourcesPath; // Sources root from -sources, scanned recursively

    CppSyntaxParser mParser;
    CodeToolCache   mCache;
    MetaGenerator   mGenerator{ mCache };

    vector<SyntaxFile*> mParsedFiles;
    vector<string>      mSourceFiles; // All files under sources path, sorted by path

    vector<string> mEnumRegistrators;               // Enum register ids collected from cpp sources
    vector<string> mTemplateClassManualRegistrators; // Template class register ids collected from cpp sources

protected:
    // Returns sorted list of all files in path and sub paths
    vector<string> GetFolderFiles(const string& path) const;

    // Parses startup arguments into a name->value map
    map<string, string> ParseArguments(char** args, int nargs) const;

    // Loads parent projects' caches
    void LoadCache();

    // Saves this project's cache
    void SaveCache();

    // Parses all headers and regenerates reflection metas
    void UpdateCodeReflection();

    // Parses one header file
    void ParseSource(const string& path);

    // Scans all .cpp files for ENUM_META calls and collects register ids; done by text scan
    // so hand-written metas in @CODETOOLIGNORE sources are collected too
    void CollectEnumRegistrators();

    // Scans all .cpp files for DECLARE_TEMPLATE_CLASS_MANUAL_ID calls and collects register ids
    void CollectTemplateClassManualRegistrators();

    // Generates the registrators list source file <project>.cpp
    void UpdateRegistratorsSource();
};
