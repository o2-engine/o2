#include "CodeToolApp.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

#include "Log.h"
#include "TextUtils.h"

Timer::Timer():
    mLastElapsedTime(std::chrono::steady_clock::now())
{}

float Timer::GetDeltaTime()
{
    auto curTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> res = curTime - mLastElapsedTime;
    mLastElapsedTime = curTime;
    return res.count();
}

void CodeToolApplication::SetArguments(char** args, int nargs)
{
    auto argsMap = ParseArguments(args, nargs);

    mProjectName = argsMap["project"];
    mSourcesPath = argsMap["sources"];

    SetVerboseLog(argsMap.find("verbose") != argsMap.end() || argsMap.find("v") != argsMap.end());

    mCache.parentProjects = Split(argsMap["parent_projects"], ' ');
}

void CodeToolApplication::Process()
{
    Timer timer;

    LoadCache();
    UpdateCodeReflection();
    UpdateRegistratorsSource();
    SaveCache();

    Log("Code reflection generated for %.3f seconds\n", timer.GetDeltaTime());
}

vector<string> CodeToolApplication::GetFolderFiles(const string& path) const
{
    vector<string> res;

    for (const auto& entry : filesystem::recursive_directory_iterator(path))
    {
        if (!entry.is_directory())
            res.push_back(entry.path().string());
    }

    sort(res.begin(), res.end());

    return res;
}

map<string, string> CodeToolApplication::ParseArguments(char** args, int nargs) const
{
    map<string, string> res;
    string lastArgName = "unnamed";
    for (int i = 0; i < nargs; i++)
    {
        if (args[i][0] == '-')
        {
            lastArgName = args[i] + 1;
            res[lastArgName] = "";
        }
        else
        {
            if (res[lastArgName].empty())
                res[lastArgName] = args[i];
            else
                res[lastArgName] += string(" ") + args[i];
        }
    }

    return res;
}

void CodeToolApplication::LoadCache()
{
    // Copy: Load() appends nested parents to mCache.parentProjects, which would invalidate this loop
    auto parents = mCache.parentProjects;
    for (auto& file : parents)
        mCache.Load(file, false);
}

void CodeToolApplication::SaveCache()
{
    mCache.Save(mSourcesPath + "/CodeToolCache.xml");
}

void CodeToolApplication::UpdateCodeReflection()
{
    mSourceFiles = GetFolderFiles(mSourcesPath);

    // Parse all headers
    for (auto& path : mSourceFiles)
    {
        if (EndsWith(path, ".h"))
            ParseSource(path);
    }

    mCache.UpdateGlobalNamespace();

    // Update reflection
    for (auto file : mParsedFiles)
        mGenerator.UpdateSourceReflection(file);

    CollectEnumRegistrators();
    CollectTemplateClassManualRegistrators();
}

void CodeToolApplication::ParseSource(const string& path)
{
    auto syntaxFile = make_unique<SyntaxFile>();
    mParser.ParseFile(*syntaxFile, path);

    mParsedFiles.push_back(mCache.AddFile(std::move(syntaxFile), true));

    VerboseLog("Parsed %s\n", path.c_str());
}

void CodeToolApplication::CollectEnumRegistrators()
{
    mEnumRegistrators.clear();

    for (auto& path : mSourceFiles)
    {
        if (!EndsWith(path, ".cpp"))
            continue;

        string data = ReadFile(path);

        size_t pos = 0;
        while ((pos = data.find("ENUM_META(", pos)) != string::npos)
        {
            size_t start = pos + strlen("ENUM_META(");
            size_t end = data.find(')', start);
            if (end == string::npos)
                break;

            string args = data.substr(start, end - start);
            size_t comma = args.find(',');
            if (comma == string::npos)
            {
                pos = end + 1;
                continue;
            }

            string id = args.substr(comma + 1);
            Trim(id, " \t\n\r");

            if (!id.empty() && find(mEnumRegistrators.begin(), mEnumRegistrators.end(), id) == mEnumRegistrators.end())
                mEnumRegistrators.push_back(id);

            pos = end + 1;
        }
    }
}

void CodeToolApplication::CollectTemplateClassManualRegistrators()
{
    mTemplateClassManualRegistrators.clear();

    const string needle = "DECLARE_TEMPLATE_CLASS_MANUAL_ID(";

    for (auto& path : mSourceFiles)
    {
        if (!EndsWith(path, ".cpp"))
            continue;

        string data = ReadFile(path);

        size_t pos = 0;
        while ((pos = data.find(needle, pos)) != string::npos)
        {
            size_t start = pos + needle.length();

            // The class name may contain template commas - find the top-level comma
            int angle = 0;
            size_t commaPos = string::npos;
            size_t endPos = string::npos;
            for (size_t i = start; i < data.size(); ++i)
            {
                char c = data[i];
                if (c == '<')
                    ++angle;
                else if (c == '>')
                    --angle;
                else if (c == ',' && angle == 0)
                    commaPos = i;
                else if (c == ')' && angle == 0)
                {
                    endPos = i;
                    break;
                }
            }

            if (commaPos == string::npos || endPos == string::npos)
                break;

            string id = data.substr(commaPos + 1, endPos - commaPos - 1);
            Trim(id, " \t\n\r");

            if (!id.empty() &&
                find(mTemplateClassManualRegistrators.begin(), mTemplateClassManualRegistrators.end(), id) ==
                    mTemplateClassManualRegistrators.end())
            {
                mTemplateClassManualRegistrators.push_back(id);
            }

            pos = endPos + 1;
        }
    }
}

void CodeToolApplication::UpdateRegistratorsSource()
{
    string registratorsSourcePath = mSourcesPath + "/" + mProjectName + ".cpp";

    const vector<string>& classRegistrators = mGenerator.GetClassRegistrators();

    string fileData;

    for (auto& id : mEnumRegistrators)
        fileData += "extern void __RegisterEnum__" + id + "();\n";

    for (auto& id : mTemplateClassManualRegistrators)
        fileData += "extern void __RegisterTemplateClass__" + id + "();\n";

    for (auto& id : classRegistrators)
        fileData += "extern void __RegisterClass__" + id + "();\n";

    fileData += "\n\n";

    fileData += "extern void InitializeTypes" + mProjectName + "()\n{\n";

    for (auto& id : mEnumRegistrators)
        fileData += "    __RegisterEnum__" + id + "();\n";

    for (auto& id : mTemplateClassManualRegistrators)
        fileData += "    __RegisterTemplateClass__" + id + "();\n";

    for (auto& id : classRegistrators)
        fileData += "    __RegisterClass__" + id + "();\n";

    fileData += "}";

    string oldFileData = ReadFile(registratorsSourcePath);

    if (oldFileData != fileData)
        WriteFile(registratorsSourcePath, fileData);
}
