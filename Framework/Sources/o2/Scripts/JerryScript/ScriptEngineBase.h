#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "jerryscript/jerry-core/include/jerryscript-core.h"

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
        jerry_value_t mParsedCode = jerry_create_undefined();

        friend class ScriptEngine;
    };

    class ScriptEngineBase
    {
    protected:
        Ref<LogStream> mLog; // Scripting log stream

    protected:
        static void ErrorCallback(const jerry_value_t error_object, void* user_p);
        static jerry_value_t PrintCallback(const jerry_value_t func_obj_val, const jerry_value_t this_p,
                                           const jerry_value_t args_p[], const jerry_length_t args_cnt);

        // Initialized basic prototypes for math and other
        void InitializeBasicPrototypes();
    };
}

#endif
