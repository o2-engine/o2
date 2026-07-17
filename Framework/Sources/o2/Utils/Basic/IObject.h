#pragma once

#include <typeinfo>

#include "o2/Utils/Memory/MemoryManager.h"
#include "o2/Utils/Types/Ref.h"
#include <type_traits>

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptValueDef.h"
#endif

namespace o2
{
    class Type;

    template<typename _type>
    const Type& GetTypeOf();

    class ReflectionInitializationTypeProcessor;
    class Reflection;

    // ----------------------------------------------------
    // Basic object interface with type information support
    // ----------------------------------------------------
    class IObject
    {
    public:
        // Virtual destructor
        virtual ~IObject() {}

        // Returns type
        virtual const Type& GetType() const { return *type; }

    private:
        static Type* type;

#if IS_SCRIPTING_SUPPORTED
    public:
        // Returns script value with wrapped object
        virtual ScriptValue GetScriptValue() const;

        // Reflects object into script value. Adds properties, functions etc
        virtual void ReflectIntoScriptValue(ScriptValue& scriptValue) const;

        // Script type prototype
        static ScriptValue GetScriptPrototype();

    protected:
        // Sets this into script value as containing object
        virtual void SetScriptValueContainer(ScriptValue& value) const;
#endif

        template<typename _type>
        friend const Type& GetTypeOf();

        template<typename _type>
        friend void SetupType(Type* type);

        friend class ReflectionInitializationTypeProcessor;
        friend class Reflection;
    };
}

// -------------------------------
// Types meta information macroses
// -------------------------------
 
#if IS_SCRIPTING_SUPPORTED
// The prototype lives in the central registry keyed by type: a local inline static would get
// duplicated across static libraries when weak symbols don't coalesce, splitting the prototype
#define IOBJECT_SCRIPTING(CLASS)                                                                                     \
    void SetScriptValueContainer(o2::ScriptValue& value) const override                                              \
    { value.SetContainingObject(const_cast<std::remove_cv_t<std::remove_pointer_t<decltype(this)>>*>(this)); }       \
    void ReflectIntoScriptValue(o2::ScriptValue&) const override {}                                                  \
    static o2::ScriptValue GetScriptPrototype()                                                                      \
    { return o2::ScriptPrototypesRegistry::Get(typeid(CLASS).name()); }                                              \
    template<typename __type>                                                                                        \
    friend struct o2::ScriptValueBase::DataContainer
#else
#define IOBJECT_SCRIPTING(CLASS)
#endif

#define IOBJECT_MAIN(CLASS)                                                                                     \
private:                                                                                                        \
    inline static o2::Type* type = nullptr;                                                                     \
                                                                                                                \
    template<typename __type>                                                                                   \
    friend const o2::Type& o2::GetTypeOf();                                                                     \
                                                                                                                \
    template<typename __type>                                                                                   \
    friend void o2::SetupType(o2::Type* type);                                                                  \
                                                                                                                \
    template<typename __type>                                                                                   \
    friend class o2::TObjectType;                                                                               \
                                                                                                                \
    template<typename __type>                                                                                   \
    friend class o2::PointerValueProxy;                                                                         \
                                                                                                                \
    template<typename __type>                                                                                   \
    friend class o2::IValueProxy;                                                                               \
                                                                                                                \
    friend class o2::ReflectionInitializationTypeProcessor;                                                     \
    friend class o2::Reflection;                                                                                \
    friend class o2::DataValue;                                                                                 \
                                                                                                                \
public:                                                                                                         \
    typedef CLASS thisclass;                                                                                    \
    const o2::Type& GetType() const override { return *type; };                                                 \
                                                                                                                \
    template<typename _type_processor> static void ProcessType(CLASS* object, _type_processor& processor)       \
    {                                                                                                           \
        processor.template Start<CLASS>(object, type);                                                          \
        ProcessBaseTypes<_type_processor>(object, processor);                                                   \
        ProcessFields<_type_processor>(object, processor);                                                      \
        ProcessMethods<_type_processor>(object, processor);                                                     \
    }

#define IOBJECT(CLASS)                                                                                          \
    IOBJECT_MAIN(CLASS)                                                                                         \
    IOBJECT_SCRIPTING(CLASS);                                                                                   \
                                                                                                                \
    template<typename _type_processor> static void ProcessBaseTypes(CLASS* object, _type_processor& processor); \
    template<typename _type_processor> static void ProcessFields(CLASS* object, _type_processor& processor);    \
    template<typename _type_processor> static void ProcessMethods(CLASS* object, _type_processor& processor)


#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptValue.h"

namespace o2
{
    inline ScriptValue IObject::GetScriptValue() const
    {
        ScriptValue result = ScriptValue::EmptyObject();
        SetScriptValueContainer(result);
        return result;
    }

    inline void IObject::ReflectIntoScriptValue(ScriptValue& scriptValue) const
    {}

    inline void IObject::SetScriptValueContainer(ScriptValue& value) const
    {
        value.SetContainingObject(const_cast<IObject*>(this));
    }

    inline ScriptValue IObject::GetScriptPrototype()
    {
        return ScriptPrototypesRegistry::Get(typeid(IObject).name());
    }

}
#endif
