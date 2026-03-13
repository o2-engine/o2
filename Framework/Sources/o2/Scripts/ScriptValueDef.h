#pragma once

#include "o2/EngineSettings.h"

#if IS_SCRIPTING_SUPPORTED

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "o2/Scripts/JerryScript/ScriptValueBase.h"
#endif

#include "o2/Utils/Types/StringDef.h"
#include "o2/Utils/Types/Ref.h"

namespace o2 {
    // ------------------------------------------------------------------------------
    // Script value. Can contain script object, engine reflected object or Function<>
    // ------------------------------------------------------------------------------
    class ScriptValue : public ScriptValueBase {
    public:
        enum class ValueType {
            None = 0, Undefined, Null, Bool, Number, String, Object, Function, Error, Symbol, BigInt, Array
        };

    public:
        template<typename _type, typename _enable = void>
        struct Converter {
            static constexpr bool isSupported = true;
            using __type = typename std::conditional<std::is_same<void, _type>::value, int, _type>::type;

            static void Write(const __type &value, ScriptValue &data);

            static void Read(__type &value, const ScriptValue &data);
        };

    public:
        // Default constructor. Creates undefined value
        ScriptValue();

        // Constructor from value
        template<typename _type>
        explicit ScriptValue(const _type &value);

        // Copy constructor. Copies reference on value
        ScriptValue(const ScriptValue &other);

        // Check equals operator
        bool operator==(const ScriptValue &other) const;

        // Check not equals operator
        bool operator!=(const ScriptValue &other) const;

        // Cast to type operator
        template<typename _type>
        operator _type() const;

        // Property accessor operator
        ScriptValue operator[](const ScriptValue &name) const;

        // Array index operator
        ScriptValue operator[](int idx) const;

        // Copy-operator. Copies reference on value
        ScriptValue &operator=(const ScriptValue &other);

        // Assign value operator
        template<typename _type>
        ScriptValue &operator=(const _type &value);

        // Returns type of value
        ValueType GetValueType() const;

        // Returns is value undefined
        bool IsUndefined() const;

        // Creates deep copy of value
        ScriptValue Copy() const;

        // Returns error description, if value type is error
        String GetError() const;

        // Dumps data into string
        String Dump(const String &tab = "") const;

        // Object methods
        // Creates empty object
        static ScriptValue EmptyObject();

        // Returns is value object
        bool IsObject() const;

        // Returns is object is native container
        bool IsObjectContainer() const;

        // Returns type of containing native object type
        const Type *GetObjectContainerType() const;

        // Sets containing object. Stores Ref<_type> for RefCounterable types, value for others
        template<typename _type>
        void SetContainingObject(_type *object);

        // Returns pointer to containing object
        void *GetContainingObject() const;

        // Sets prototype property wrapper for prototype-based field access
        template<typename _object_type, typename _field_type>
        void SetPrototypePropertyWrapper(const ScriptValue &name, void* (*pointerGetter)(void*));

        // Constructs object from this
        ScriptValue Construct(const Vector<ScriptValue> &args);

        // Invokes function
        template<typename ... _args>
        ScriptValue Construct(_args ... args) const;

        // Iterates properties in object
        void ForEachProperties(const Function<bool(const ScriptValue &name, const ScriptValue &value)> &func,
                               bool withPrototypes = true) const;

        // Returns property value
        ScriptValue GetProperty(const ScriptValue &name) const;

        // Returns internal property value
        ScriptValue GetInternalProperty(const ScriptValue &name) const;

        // Returns own property value
        ScriptValue GetOwnProperty(const ScriptValue &name) const;

        // Returns list of all property names
        ScriptValue GetPropertyNames() const;

        // Returns property value
        ScriptValue GetProperty(const char *name) const;

        // Sets property value
        void SetProperty(const ScriptValue &name, const ScriptValue &value);

        // Sets internal property value
        void SetInternalProperty(const ScriptValue &name, const ScriptValue &value);

        // Sets property value
        void SetProperty(const char *name, const ScriptValue &value);

        // Sets property value
        template<typename _type>
        void SetProperty(const char *name, const _type &value);

        // Sets property value
        template<typename _class_type, typename _res_type, typename ... _args>
        void SetProperty(const char *name, _class_type *object, _res_type(_class_type::* functionPtr)(_args ... args));

        // Sets property value
        template<typename _class_type, typename _res_type, typename ... _args>
        void SetProperty(const char *name, _class_type *object, _res_type(_class_type::* functionPtr)(_args ... args) const);

        // Sets property wrapper. Uses value reference to get/set value from script
        template<typename _type>
        void SetPropertyWrapper(const ScriptValue &name, _type &value);

        // Sets property wrapper. Uses setter and getter functions to get/set value from script
        template<typename _type>
        void SetPropertyWrapper(const ScriptValue &name, const Function<void(const _type &value)> &setter,
                                const Function<_type()> &getter);

        // Removes property
        void RemoveProperty(const ScriptValue &name);

        // Sets prototype object
        void SetPrototype(const ScriptValue &proto);

        // Returns prototype object
        ScriptValue GetPrototype() const;

        // Array methods
        // Creates empty array
        static ScriptValue EmptyArray();

        // Returns is array
        bool IsArray() const;

        // Returns length of array
        int GetLength() const;

        // Sets array index
        void SetElement(const ScriptValue &value, int idx);

        // Returns array index
        ScriptValue GetElement(int idx) const;

        // Adds array element at end
        void AddElement(const ScriptValue &value);

        // Removes array element
        void RemoveElement(int idx);

        //Value methods
        // Boolean cast
        bool ToBool() const;

        // Number cast
        float ToNumber() const;

        // String cast
        String ToString() const;

        // Returns value. Trying to cast value, returns default if cant
        template<typename _type>
        _type GetValue() const;

        // Sets value
        template<typename _type>
        void SetValue(const _type &value);

        // Function methods
        // Returns is function
        bool IsFunction() const;

        // Returns is constructor function
        bool IsConstructor() const;

        // Invokes function
        ScriptValue InvokeRaw(const Vector<ScriptValue> &args) const;

        // Invokes member function
        ScriptValue InvokeRaw(const ScriptValue &thisValue, const Vector<ScriptValue> &args) const;

        // Invokes function
        template<typename _res_type, typename ... _args>
        _res_type Invoke(_args ... args) const;

        // Invokes member function
        template<typename _res_type, typename ... _args>
        _res_type Invoke(const ScriptValue &thisValue, _args ... args) const;

        // Sets function with passed this value
        template<typename _res_type, typename ... _args>
        void SetThisFunction(const Function<_res_type(ScriptValue, _args ...)> &func);

        // Sets function from class. This must contain
        template<typename _class_type, typename _res_type, typename ... _args>
        void SetClassFunction(_res_type(_class_type::* functionPtr)(_args ... args));

        // Sets function from class. This must contain
        template<typename _class_type, typename _res_type, typename ... _args>
        void SetClassFunction(_res_type(_class_type::* functionPtr)(_args ... args) const);

        // Creates script value from class function
        template<typename _class_type, typename _res_type, typename ... _args>
        static ScriptValue ClassFunction(_res_type(_class_type::* functionPtr)(_args ... args));

        // Creates script value from class function
        template<typename _class_type, typename _res_type, typename ... _args>
        static ScriptValue ClassFunction(_res_type(_class_type::* functionPtr)(_args ... args) const);

    private:
        template<typename ... _args>
        static void PackArgs(Vector<ScriptValue> &argsValues, _args ... args) {
            ([&](auto &arg) { argsValues.Add(ScriptValue(arg)); }(args), ...);
        }
    };
}

#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // -----------------------------------
    // Basic script value property wrapper
    // -----------------------------------
    struct IScriptValueProperty: public RefCounterable
    {
        virtual ~IScriptValueProperty() {}
        virtual ScriptValue Get() const = 0;
        virtual void Set(const ScriptValue& value) = 0;
        virtual IScriptValueProperty* Clone() const = 0;
    };

    // ------------------------------
    // Script object property wrapper
    // ------------------------------
    struct ScriptValueProperty : public IScriptValueProperty
    {
        ScriptValue object; // Property owner object
        ScriptValue name;   // Property name

    public:
        ScriptValueProperty() {}
         ScriptValueProperty(const ScriptValue& object, const ScriptValue& name) :
             object(object), name(name) {}

        ScriptValue Get() const override;
        void Set(const ScriptValue& value) override;

        IScriptValueProperty* Clone() const override;

        bool operator==(const ScriptValueProperty& other) const;
    };

    // -------------------------------------
    // Script array element property wrapper
    // -------------------------------------
    struct ScriptValueArrayElement : public IScriptValueProperty
    {
        ScriptValue object;  // Property owner array
        int         idx = 0; // Value idx

    public:
        ScriptValueArrayElement() {}
        ScriptValueArrayElement(const ScriptValue& object, int idx) :
            object(object), idx(idx) {}

        ScriptValue Get() const override;
        void Set(const ScriptValue& value) override;

        IScriptValueProperty* Clone() const override;

        bool operator==(const ScriptValueArrayElement& other) const;
    };

    // ----------------------------------------------
    // Script value simple types prototypes container
    // ----------------------------------------------
    struct ScriptValuePrototypes
    {
        static ScriptValue*& GetVec2Prototype();
        static ScriptValue*& GetRectPrototype();
        static ScriptValue*& GetBorderPrototype();
        static ScriptValue*& GetColor4Prototype();
    };
}

#endif // IS_SCRIPTING_SUPPORTED
// --- META ---

PRE_ENUM_META(o2::ScriptValue::ValueType);
// --- END META ---
