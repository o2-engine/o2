#if defined(SCRIPTING_BACKEND_BROWSERJS)

#include "o2/Scripts/BrowserJS/BrowserJSCore.h"

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
        o2js_value_t mParsedCode = o2js_undefined();

        friend class ScriptEngine;
    };

    class ScriptEngineBase
    {
    protected:
        Ref<LogStream> mLog; // Scripting log stream

    protected:
        static void ErrorCallback(o2js_value_t error_value, void* user_p);
        static o2js_value_t PrintCallback(o2js_value_t func_obj_val, o2js_value_t this_p,
                                          const o2js_value_t args_p[], int args_cnt);

        // Initialized basic prototypes for math and other
        void InitializeBasicPrototypes();
    };
}

#endif
