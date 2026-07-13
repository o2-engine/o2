#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_QUICKJS)
#include "o2/Scripts/QuickJS/QuickJSCore.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    using namespace QuickJs;

    namespace
    {
        String FormatErrorLogMessage(JSValueConst error_value)
        {
            ScriptValue errorValue;
            errorValue.AcquireValue(error_value);

            String msg = errorValue.ToString();

            ScriptValue stack = errorValue.GetProperty("stack");
            if (stack.GetValueType() == ScriptValue::ValueType::String)
                msg += " at " + stack.ToString();

            return msg;
        }
    } // namespace

    void ScriptEngineBase::ErrorCallback(JSValueConst error_value, void* user_p)
    {
        (void)user_p;
        o2Scripts.mLog->ErrorStr(FormatErrorLogMessage(error_value));
    }

    JSValue ScriptEngineBase::PrintCallback(JSValueConst func_obj_val, JSValueConst this_p,
                                            JSValueConst* args_p, int args_cnt)
    {
        for (int i = 0; i < args_cnt; i++)
        {
            ScriptValue v;
            v.AcquireValue(args_p[i]);
            o2Scripts.mLog->OutStr(v.GetValue<String>());
        }

        return JS_UNDEFINED;
    }

    ScriptParseResultBase::~ScriptParseResultBase()
    {
        JS_FreeValue(Ctx(), mParsedCode);
    }

    ScriptParseResultBase::ScriptParseResultBase(const ScriptParseResultBase& other)
    {
        mParsedCode = JS_DupValue(Ctx(), other.mParsedCode);
        mParseError = other.mParseError;
    }

    bool ScriptParseResult::IsOk() const
    {
        return !mParseError;
    }

    String ScriptParseResult::GetError() const
    {
        ScriptValue tmp;
        tmp.AcquireValue(mParsedCode);
        tmp.mIsError = mParseError;
        return tmp.GetError();
    }

    ScriptEngine::ScriptEngine(RefCounter* refCounter):
        Singleton<ScriptEngine>(refCounter)
    {
        mLog = mmake<LogStream>("Scripting");
        o2Debug.GetLog()->BindStream(mLog);

        QuickJs::Initialize();
        QuickJs::SetErrorCallback(&ErrorCallback, NULL);

        ScriptValue printFunc;
        printFunc.Accept(NewExternalFunction(&PrintCallback));
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
        const char* name = filename.Length() > 0 ? filename.Data() : "<script>";

        ScriptParseResult res;
        JSValue compiled = JS_Eval(Ctx(), script.Data(), script.Length(), name,
                                   JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(compiled))
        {
            res.mParsedCode = TakeThrown();
            res.mParseError = true;
        }
        else
            res.mParsedCode = compiled;

        return res;
    }

    ScriptValue ScriptEngine::Run(const ScriptParseResult& parseResult)
    {
        ScriptValue res;
        if (parseResult.mParseError)
        {
            res.AcquireValue(parseResult.mParsedCode);
            res.mIsError = true;
            return res;
        }

        JSContext* ctx = Ctx();
        JSValue evalRes = JS_EvalFunction(ctx, JS_DupValue(ctx, parseResult.mParsedCode));
        if (JS_IsException(evalRes))
            res.AcceptThrown();
        else
            res.Accept(evalRes);

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
        res.Accept(JS_GetGlobalObject(Ctx()));
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
    {
        JS_RunGC(Runtime());
    }

    int ScriptEngine::GetUsedMemory() const
    {
        JSMemoryUsage usage = {};
        JS_ComputeMemoryUsage(Runtime(), &usage);
        return (int)usage.memory_used_size;
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
