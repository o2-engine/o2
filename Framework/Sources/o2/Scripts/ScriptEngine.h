#pragma once

#if IS_SCRIPTING_SUPPORTED

#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "o2/Scripts/JerryScript/ScriptEngineBase.h"
#elif defined(SCRIPTING_BACKEND_BROWSERJS)
#include "o2/Scripts/BrowserJS/ScriptEngineBase.h"
#elif defined(SCRIPTING_BACKEND_QUICKJS)
#include "o2/Scripts/QuickJS/ScriptEngineBase.h"
#endif

// Scripts engine access macros
#define  o2Scripts o2::ScriptEngine::Instance()

namespace o2
{
    class ScriptValue;
    struct ScriptPrototypeProcessor;

    // -------------------
    // Script parse result
    // -------------------
    class ScriptParseResult: public ScriptParseResultBase
    {
    public:
        // Returns is result ok
        bool IsOk() const;

        // Returns error description
        String GetError() const;

        // Bool cast operator, returns is result ok
        operator bool() const;
    };

    // ----------------------------------------------------------------
    // Scripting engine. Depends on scripting backend. 
    // Can parse, run and evaluate scripts. Can return global namespace
    // ----------------------------------------------------------------
    class ScriptEngine: public Singleton<ScriptEngine>, public ScriptEngineBase
    {
    public:
        // Default constructor, initializes scripting engine
        ScriptEngine(RefCounter* refCounter);

        // Default destructor, cleans up
        ~ScriptEngine();

        // Parses script from string and returns parse result
        ScriptParseResult Parse(const String& script, const String& filename = "");

        // Runs parsed script, and returns retult
        ScriptValue Run(const ScriptParseResult& parseResult);

        // Evaluates script
        ScriptValue Eval(const String& script, const String& filename = "");

        // Creates new realm
        ScriptValue CreateRealm();

        // Sets current realm
        ScriptValue SetCurrentRealm(const ScriptValue& realm);

        // Returns global namespace
        ScriptValue GetGlobal() const;

        // Collects garbage
        void CollectGarbage() const; 
        
        // Returns used memory in bytes
        int GetUsedMemory() const;

        // Starts debugging session and waits for connect
        void ConnectDebugger() const;

    private:
        // Registers all types from reflection
        void RegisterTypes();

        // Runs built in script with math and etc, required to work framework
        void RunBuildtinScripts();

        // Runs built in script from file
        void RunBuiltinScript(const String& filename);

    private:
        typedef void(*RegisterConstructorFunc)(void*, ScriptPrototypeProcessor&);
        static Vector<RegisterConstructorFunc>& GetRegisterConstructorFuncs();

        friend class Reflection;
        friend void AddReflectionScriptConstructorFunc(void(*func)(void*, ScriptPrototypeProcessor&));
    };
}

#endif // IS_SCRIPTING_SUPPORTED
