#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "jerryscript/jerry-ext/include/jerryscript-ext/debugger.h"
#include "jerryscript/jerry-ext/include/jerryscript-ext/handler.h"
#include "jerryscript/jerry-port/default/include/jerryscript-port-default.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    namespace
    {
        String FormatErrorLogMessage(const jerry_value_t error_object)
        {
            jerry_value_t strValue = jerry_value_to_string(error_object);
            ScriptValue msgSv;
            msgSv.AcquireValue(strValue);
            jerry_release_value(strValue);
            String msg = msgSv.GetValue<String>();

            jerry_value_t backtrace = jerry_get_backtrace(32);
            if (!jerry_value_is_array(backtrace))
            {
                jerry_release_value(backtrace);
                return msg;
            }

            const uint32_t len = jerry_get_array_length(backtrace);
            if (len == 0)
            {
                jerry_release_value(backtrace);
                return msg;
            }

            msg += " at ";
            for (uint32_t i = 0; i < len; i++)
            {
                jerry_value_t frame = jerry_get_property_by_index(backtrace, i);
                if (i > 0)
                    msg += " <- ";

                ScriptValue frameSv;
                frameSv.AcquireValue(frame);
                jerry_release_value(frame);
                msg += frameSv.GetValue<String>();
            }

            jerry_release_value(backtrace);
            return msg;
        }
    } // namespace

    void ScriptEngineBase::ErrorCallback(const jerry_value_t error_object, void* user_p)
    {
        (void)user_p;
        o2Scripts.mLog->ErrorStr(FormatErrorLogMessage(error_object));
    }

    jerry_value_t ScriptEngineBase::PrintCallback(const jerry_value_t func_obj_val, const jerry_value_t this_p,
                                                  const jerry_value_t args_p[], const jerry_length_t args_cnt)
    {
        for (int i = 0; i < (int)args_cnt; i++)
        {
            ScriptValue v;
            v.AcquireValue(args_p[i]);
            o2Scripts.mLog->OutStr(v.GetValue<String>());
        }

        return jerry_create_undefined();
    }

    ScriptParseResultBase::~ScriptParseResultBase()
    {
        jerry_release_value(mParsedCode);
    }

    ScriptParseResultBase::ScriptParseResultBase(const ScriptParseResultBase& other)
    {
        mParsedCode = jerry_acquire_value(other.mParsedCode);
    }

    bool ScriptParseResult::IsOk() const
    {
        return !jerry_value_is_error(mParsedCode);
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

        jerry_init(JERRY_INIT_EMPTY);
        jerryx_handler_register_global((const jerry_char_t*)"print", PrintCallback);
        jerry_set_error_object_created_callback(&ErrorCallback, NULL);

        GetGlobal().SetProperty("Dump", Function<String(const ScriptValue&)>([](const ScriptValue& v) { return v.Dump(); }));

        RunBuildtinScripts();
        InitializeBasicPrototypes();
        RegisterTypes();

        jerry_port_default_set_log_level(JERRY_LOG_LEVEL_DEBUG);

        //ConnectDebugger();
    }

    ScriptEngine::~ScriptEngine()
    {
        delete ScriptValuePrototypes::GetVec2Prototype();
        delete ScriptValuePrototypes::GetRectPrototype();
        delete ScriptValuePrototypes::GetBorderPrototype();
        delete ScriptValuePrototypes::GetColor4Prototype();
        ScriptValuePropertyKeys::Deinitialize();
        //jerry_cleanup();
    }

    ScriptParseResult ScriptEngine::Parse(const String& script, const String& filename /*= ""*/)
    {
        ScriptParseResult res;
        res.mParsedCode = jerry_parse((jerry_char_t*)filename.Data(), filename.Length(),
                                      (jerry_char_t*)script.Data(), script.Length(), JERRY_PARSE_NO_OPTS);

        return res;
    }

    ScriptValue ScriptEngine::Run(const ScriptParseResult& parseResult)
    {
        ScriptValue res;
        res.Accept(jerry_run(parseResult.mParsedCode));
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
        res.Accept(jerry_get_global_object());
        return res;
    }

    ScriptValue ScriptEngine::CreateRealm()
    {
        ScriptValue res;
        res.Accept(jerry_create_realm());
        return res;
    }

    ScriptValue ScriptEngine::SetCurrentRealm(const ScriptValue& realm)
    {
        ScriptValue res;
        res.AcquireValue(jerry_set_realm(realm.jvalue));
        return res;
    }

    void ScriptEngine::CollectGarbage() const
    {
        jerry_gc(JERRY_GC_PRESSURE_HIGH);
    }
    
    int ScriptEngine::GetUsedMemory() const
    {
        jerry_heap_stats_t stats = { 0 };
        bool get_stats_ret = jerry_get_memory_stats(&stats);
        return (int)stats.allocated_bytes;
    }

    void ScriptEngine::ConnectDebugger() const
    {
        jerryx_debugger_after_connect(jerryx_debugger_tcp_create(5001) && jerryx_debugger_ws_create());
    }

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
