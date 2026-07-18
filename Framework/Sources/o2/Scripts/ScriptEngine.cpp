#include "o2/stdafx.h"

#if IS_SCRIPTING_SUPPORTED
#include "ScriptEngine.h"
#include "o2/Utils/Debug/Log/LogStream.h"
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

        for (auto& func : ScriptPrototypesRegistry::GetPostRegisterFuncs())
            func();

        ScriptPrototypesRegistry::GetPostRegisterFuncs().Clear();

        mLog->Out("Registered types in " + (String)t.GetDeltaTime() + " seconds");
    }

    void ScriptEngine::RunBuildtinScripts()
    {
        RunBuiltinScript("Scripts/o2.js");
        RunBuiltinScript("Scripts/Math.js");
        RunBuiltinScript("Scripts/Component.js");
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