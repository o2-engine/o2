#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_BROWSERJS)
#include "o2/Scripts/BrowserJS/BrowserJSCore.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    namespace
    {
        String FormatErrorLogMessage(o2js_value_t error_value)
        {
            ScriptValue errorValue;
            errorValue.AcquireValue(error_value);

            String msg;
            {
                ScriptValue tmp = errorValue;
                msg = tmp.ToString();
            }

            if (errorValue.GetValueType() == ScriptValue::ValueType::Object)
            {
                ScriptValue stack = errorValue.GetProperty("stack");
                if (stack.GetValueType() == ScriptValue::ValueType::String)
                    msg += " at " + stack.ToString();
            }

            return msg;
        }
    } // namespace

    void ScriptEngineBase::ErrorCallback(o2js_value_t error_value, void* user_p)
    {
        (void)user_p;
        o2Scripts.mLog->ErrorStr(FormatErrorLogMessage(error_value));
    }

    o2js_value_t ScriptEngineBase::PrintCallback(o2js_value_t func_obj_val, o2js_value_t this_p,
                                                 const o2js_value_t args_p[], int args_cnt)
    {
        for (int i = 0; i < args_cnt; i++)
        {
            ScriptValue v;
            v.AcquireValue(args_p[i]);
            o2Scripts.mLog->OutStr(v.GetValue<String>());
        }

        return o2js_undefined();
    }

    ScriptParseResultBase::~ScriptParseResultBase()
    {
        o2js_release(mParsedCode);
    }

    ScriptParseResultBase::ScriptParseResultBase(const ScriptParseResultBase& other)
    {
        mParsedCode = o2js_acquire(other.mParsedCode);
    }

    bool ScriptParseResult::IsOk() const
    {
        return !o2js_is_error(mParsedCode);
    }

    String ScriptParseResult::GetError() const
    {
        ScriptValue tmp;
        tmp.AcquireValue(mParsedCode);
        return tmp.GetError();
    }

    ScriptEngine::ScriptEngine(RefCounter* refCounter):
        Singleton<ScriptEngine>(refCounter)
    {
        mLog = mmake<LogStream>("Scripting");
        o2Debug.GetLog()->BindStream(mLog);

        o2js_initialize();
        o2js_set_error_created_callback(&ErrorCallback, NULL);

        ScriptValue printFunc;
        printFunc.Accept(o2js_external_function(&PrintCallback));
        GetGlobal().SetProperty("print", printFunc);

        GetGlobal().SetProperty("Dump", Function<String(const ScriptValue&)>([](const ScriptValue& v) { return v.Dump(); }));

        RunBuildtinScripts();
        InitializeBasicPrototypes();
        RegisterTypes();
    }

    ScriptEngine::~ScriptEngine()
    {
        delete ScriptValuePrototypes::GetVec2Prototype();
        delete ScriptValuePrototypes::GetRectPrototype();
        delete ScriptValuePrototypes::GetBorderPrototype();
        delete ScriptValuePrototypes::GetColor4Prototype();
        ScriptValuePropertyKeys::Deinitialize();
    }

    ScriptParseResult ScriptEngine::Parse(const String& script, const String& filename /*= ""*/)
    {
        ScriptParseResult res;
        res.mParsedCode = o2js_parse(script.Data(), script.Length(), filename.Data(), filename.Length());
        return res;
    }

    ScriptValue ScriptEngine::Run(const ScriptParseResult& parseResult)
    {
        ScriptValue res;
        res.Accept(o2js_run(parseResult.mParsedCode));
        return res;
    }

    ScriptValue ScriptEngine::Eval(const String& script, const String& filename /*= ""*/)
    {
        auto parseRes = Parse(script, filename);
        if (parseRes.IsOk())
            return Run(parseRes);

        return ScriptValue();
    }

    ScriptValue ScriptEngine::GetGlobal() const
    {
        ScriptValue res;
        res.Accept(o2js_get_global());
        return res;
    }

    ScriptValue ScriptEngine::CreateRealm()
    {
        return GetGlobal();
    }

    ScriptValue ScriptEngine::SetCurrentRealm(const ScriptValue& realm)
    {
        return GetGlobal();
    }

    void ScriptEngine::CollectGarbage() const
    {}

    int ScriptEngine::GetUsedMemory() const
    {
        return o2js_get_used_memory();
    }

    void ScriptEngine::ConnectDebugger() const
    {}

    void ScriptEngineBase::InitializeBasicPrototypes()
    {
        auto global = o2Scripts.GetGlobal();
        ScriptValuePrototypes::GetVec2Prototype() = mnew ScriptValue(o2Scripts.Eval("Vec2.prototype"));
        ScriptValuePrototypes::GetRectPrototype() = mnew ScriptValue(o2Scripts.Eval("Rect.prototype"));
        ScriptValuePrototypes::GetBorderPrototype() = mnew ScriptValue(o2Scripts.Eval("Border.prototype"));
        ScriptValuePrototypes::GetColor4Prototype() = mnew ScriptValue(o2Scripts.Eval("Color4.prototype"));
        ScriptValuePropertyKeys::Initialize();
    }
}

#endif
