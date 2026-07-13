#if defined(SCRIPTING_BACKEND_QUICKJS)

#include "o2/Scripts/QuickJS/QuickJSCore.h"

#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    class LogStream;

    class ScriptParseResultBase
    {
    public:
        ScriptParseResultBase() = default;
        ScriptParseResultBase(const ScriptParseResultBase& other);
        virtual ~ScriptParseResultBase();

    protected:
        JSValue mParsedCode = JS_UNDEFINED;
        bool mParseError = false;

        friend class ScriptEngine;
    };

    class ScriptEngineBase
    {
    protected:
        Ref<LogStream> mLog; // Scripting log stream

    protected:
        static void ErrorCallback(JSValueConst error_value, void* user_p);
        static JSValue PrintCallback(JSValueConst func_obj_val, JSValueConst this_p,
                                     JSValueConst* args_p, int args_cnt);

        // Initialized basic prototypes for math and other
        void InitializeBasicPrototypes();
    };
}

#endif
