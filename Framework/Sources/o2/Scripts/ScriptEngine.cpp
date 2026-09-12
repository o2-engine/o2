#include "o2/stdafx.h"

#if IS_SCRIPTING_SUPPORTED
#include "ScriptEngine.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Timer.h"

namespace o2
{
    DECLARE_SINGLETON(ScriptEngine);

    ScriptParseResult::operator bool() const
    {
        return IsOk();
    }

    void ScriptEngine::RegisterTypes()
    {
        Timer t;

        auto global = GetGlobal();
        for (auto func : GetRegisterConstructorFuncs())
        {
            ScriptPrototypeProcessor processor;
            func(0, processor);
        }

        GetRegisterConstructorFuncs().Clear();

        ScriptPrototypesRegistry::ApplySecondaryBases();

        mLog->Out("Registered types in " + (String)t.GetDeltaTime() + " seconds");
    }

    void ScriptEngine::RunBuildtinScripts()
    {
        RunBuiltinScript("Scripts/o2.js");
        RunBuiltinScript("Scripts/Math.js");
        RunBuiltinScript("Scripts/Component.js");

        RegisterBuiltinFunctions();
    }

    void ScriptEngine::RegisterBuiltinFunctions()
    {
        GetGlobal().SetProperty("include", Function<bool(const String&)>([this](const String& path) { return Include(path); }));

        auto fileSystem = ScriptValue::EmptyObject();
        fileSystem.SetProperty("IsFileExist", Function<bool(const String&)>([](const String& path)
        {
            return o2FileSystem.IsFileExist(path);
        }));
        fileSystem.SetProperty("ReadFile", Function<ScriptValue(const String&)>([](const String& path)
        {
            return o2FileSystem.IsFileExist(path) ? ScriptValue(FileSystem::ReadFile(path)) : ScriptValue();
        }));
        fileSystem.SetProperty("WriteFile", Function<bool(const String&, const String&)>([](const String& path, const String& data)
        {
            FileSystem::WriteFile(path, data);
            return o2FileSystem.IsFileExist(path);
        }));
        fileSystem.SetProperty("FileDelete", Function<bool(const String&)>([](const String& path)
        {
            return o2FileSystem.FileDelete(path);
        }));
        GetGlobal().GetProperty("o2").SetProperty("FileSystem", fileSystem);
    }

    bool ScriptEngine::Include(const String& path)
    {
        String builtPath = Assets::IsSingletonInitialzed() ? o2Assets.GetAssetsTree().builtAssetsPath : String(::GetBuiltAssetsPath());
        String fullPath = builtPath + path;
        TimeStamp editDate = o2FileSystem.GetFileInfo(fullPath).editDate;

        if (mIncludedScripts.ContainsKey(path) && mIncludedScripts[path] == editDate)
            return true;

        InFile file(fullPath);
        if (!file.IsOpened())
        {
            mLog->Error("Can't include script " + path + ": no file " + fullPath);
            return false;
        }

        auto parsed = Parse(file.ReadFullData(), path);
        if (!parsed.IsOk())
        {
            mLog->Error("Script " + path + " can't be parsed: " + parsed.GetError());
            return false;
        }

        // Marked before running: scripts that include each other must not loop
        mIncludedScripts[path] = editDate;
        auto result = Run(parsed);
        if (result.GetValueType() == ScriptValue::ValueType::Error)
        {
            mIncludedScripts.Remove(path);
            mLog->Error("Script " + path + " failed: " + result.GetError());
            return false;
        }

        return true;
    }

    void ScriptEngine::RunBuiltinScript(const String& filename)
    {
        Eval(o2FileSystem.ReadFile(GetBuiltinAssetsPath() + filename), filename);
    }

    Vector<ScriptEngine::RegisterConstructorFunc>& ScriptEngine::GetRegisterConstructorFuncs()
    {
        static Vector<ScriptEngine::RegisterConstructorFunc> funcs;
        return funcs;
    }

}

#endif // IS_SCRIPTING_SUPPORTED