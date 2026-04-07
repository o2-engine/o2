//@CODETOOLIGNORE

#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Reflection/Attributes.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/StringDef.h"

// Returns type of TYPE
#define TypeOf(TYPE) o2::GetTypeOf<TYPE>()

namespace o2
{
    class DataValue;
    class FieldInfo;
    class FunctionInfo;
    class IAbstractValueProxy;
    class IObject;
    class StaticFunctionInfo;
    class Type;
    struct ITypeSerializer;

    template<typename _type>
    class IValueProxy;

    template<typename _key_type, typename _value_type>
    class Map;

    template<typename _type, typename _getter>
    const Type& GetTypeOf();

    template<typename _element_type>
    struct VectorCountFieldSerializer;

    typedef UInt TypeId;

    // ---------------
    // Type of a value
    // ---------------
    class Type
    {
    public:
        enum class Usage
        {
            Regular, Object, Vector, Map, StringAccessor, Enumeration, Pointer, Reference, Property, Function
        };

        struct BaseType
        {
            const Type* type;
            void* (*dynamicCastUpFunc)(void*);
            void* (*dynamicCastDownFunc)(void*);

            bool operator==(const BaseType& other) const { return type == other.type; }
        };

    public:
        // Default constructor
        Type(const String& name, int size, ITypeSerializer* serializer);

        // Destructor
        virtual ~Type();

        // Check equals operator
        bool operator==(const Type& other) const;

        // Check not equals operator
        bool operator!=(const Type& other) const;

        // Returns name of type
        const String& GetName() const;

        // Returns id of type
        TypeId ID() const;

        // Returns size of type in bytes
        int GetSize() const;

        // Is this type based on other
        bool IsBasedOn(const Type& other) const;

        // Returns type usage
        virtual Usage GetUsage() const;

        // Returns vector of base types
        const Vector<BaseType>& GetBaseTypes() const;

        // Returns fields informations array
        const Vector<FieldInfo>& GetFields() const;

        // Returns fields informations array with all base types
        Vector<const FieldInfo*> GetFieldsWithBaseClasses() const;

        // Returns functions informations array
        const Vector<FunctionInfo*>& GetFunctions() const;

        // Returns static functions informations array
        const Vector<StaticFunctionInfo*>& GetStaticFunctions() const;

        // Returns functions informations array with all base types
        Vector<FunctionInfo*> GetFunctionsWithBaseClasses() const;

        // Returns functions informations array with all base types
        Vector<StaticFunctionInfo*> GetStaticFunctionsWithBaseClasses() const;

        // Returns field information by name
        const FieldInfo* GetField(const String& name) const;

        // Returns function info by name
        const FunctionInfo* GetFunction(const String& name) const;

        // Returns static function info by name
        const StaticFunctionInfo* GetStaticFunction(const String& name) const;

        // Invokes function with name
        template<typename _res_type, typename ... _args>
        _res_type Invoke(const String& name, void* object, _args ... args) const;

        // Invokes function with name
        template<typename _res_type, typename ... _args>
        _res_type InvokeStatic(const String& name, _args ... args) const;

        // Returns derived types
        Vector<const Type*> GetDerivedTypes(bool deep = true) const;

        // Creates sample copy and returns him
        virtual void* CreateSample() const = 0;

        // Creates sample copy and returns him if type is reference
        virtual Ref<RefCounterable> CreateSampleRef() const;

        // Destroys sample
        virtual void DestroySample(void* sample) const = 0;

        // Returns filed pointer by path
        virtual void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const;

        // Returns abstract value proxy for object value
        virtual Ref<IAbstractValueProxy> GetValueProxy(void* object) const = 0;

        // Serializes value from ptr
        virtual void Serialize(void* ptr, DataValue& data) const;

        // Deserializes value from ptr
        virtual void Deserialize(void* ptr, const DataValue& data) const;

        // Returns is values getted from object A and object B equals
        virtual bool IsValueEquals(void* objectA, void* objectB) const;

        // Copies value getter from objectB to value from objectA
        virtual void CopyValue(void* objectA, void* objectB) const;

        // Returns type serializer
        ITypeSerializer* GetSerializer() const;

    public:
        // --------------------
        // Dummy type container
        // --------------------
        struct Dummy { static Type* type; };

        template<class T, class = std::void_t<>>
        struct IsConstructible: std::false_type {};

        template<class T>
        struct IsConstructible<T, std::void_t<decltype(std::declval<T()>())>>: std::true_type {};

    protected:
        TypeId mId;   // Id of type
        String mName; // Name of object type
        int    mSize; // Size of type in bytes

        Vector<BaseType> mBaseTypes; // Base types ids with offset 

        Vector<FieldInfo>           mFields;          // Fields information
        Vector<FunctionInfo*>       mFunctions;       // Functions informations
        Vector<StaticFunctionInfo*> mStaticFunctions; // Functions informations

        mutable Type* mPtrType = nullptr; // Pointer type from this
        mutable Type* mRefType = nullptr; // Reference type from this

        ITypeSerializer* mSerializer = nullptr; // Value serializer

        friend class FieldInfo;
        friend class FunctionInfo;
        friend class PointerType;
        friend class Reflection;
        friend class ReflectionInitializationTypeProcessor;
        friend class VectorType;

        template<typename _type, typename _accessor_type>
        friend class TStringPointerAccessorType;
    };

    // -------------------
    // Type specialization
    // -------------------
    template<typename _type>
    class TType: public Type
    {
    public:
        // Default constructor
        TType(const String& name, int size);

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Creates sample copy and returns him if type is reference
        Ref<RefCounterable> CreateSampleRef() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // -------------
    // Function type
    // -------------
    class FunctionType : public Type
    {
    public:
        // Constructor
        FunctionType(const String& name);

        // Returns type usage
        Usage GetUsage() const override;

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

    public:
        static FunctionType* commonType;       // Common function type container
        static FunctionType* serializableType; // Serializable function type container

    };

    // --------------------------------------------------------------------
    // Type of objects, derived from IObject. Can be casted to/from IObject
    // --------------------------------------------------------------------
    class ObjectType: public Type
    {
    public:
        // Constructor
        ObjectType(const String& name, int size, void* (*castFromFunc)(void*), void* (*castToFunc)(void*), ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const override;

        // Dynamically casts from IObject* to type pointer
        void* DynamicCastFromIObject(IObject* object) const;

        // Dynamically casts from IObject* to type pointer
        IObject* DynamicCastToIObject(void* object) const;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

    protected:
        void* (*mCastFromFunc)(void*); // Dynamic cast function from IObject
        void* (*mCastToFunc)(void*); // Dynamic cast function from IObject
    };

    // -----------------------
    // Specialized object type
    // -----------------------
    template<typename _type>
    class TObjectType: public ObjectType
    {
    public:
        // Default constructor
        TObjectType(const String& name, int size, void* (*castFromFunc)(void*), void* (*castToFunc)(void*));

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Creates sample copy and returns him if type is reference
        Ref<RefCounterable> CreateSampleRef() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // ----------------
    // Fundamental type
    // ----------------
    template<typename _type>
    class FundamentalType: public TType<_type>
    {
    public:
        // Default constructor
        FundamentalType(const String& name);
    };

    // ------------
    // Pointer type
    // ------------
    class PointerType : public Type
    {
    public:
        // Constructor
        PointerType(const Type* baseType, ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const override;

        // Returns base type
        const Type* GetBaseType() const;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

    protected:
        const Type* mBaseType;
    };

    // ------------------------
    // Specialized pointer type
    // ------------------------
    template<typename _type>
    class TPointerType : public PointerType
    {
    public:
        // Default constructor
        TPointerType(const Type* baseType);

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // ----------------------
    // Reference type (Ref<Actor>)
    // ----------------------
    class ReferenceType : public Type
    {
    public:
        // Constructor
        ReferenceType(const Type* baseType, ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const override;

        // Returns base type
        const Type* GetBaseType() const;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Creates sample reference with object
        virtual void* CreateSample(void* object) const = 0;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

        // Returns raw pointer from reference pointer
        virtual void* GetObjectRawPtr(void* refPtr) const = 0;

        // Sets raw pointer to reference
        virtual void SetObjectRawPtr(void* object, void* refPtr) = 0;

    protected:
        const Type* mBaseType;
    };

    // --------------------------
    // Specialized reference type
    // --------------------------
    template<typename _type>
    class TReferenceType : public ReferenceType
    {
    public:
        // Default constructor
        TReferenceType(const Type* baseType);

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

        // Returns raw pointer from reference pointer
        void* GetObjectRawPtr(void* refPtr) const override;

        // Sets raw pointer to reference
        void SetObjectRawPtr(void* object, void* refPtr) override;

        // Creates sample reference with object
        void* CreateSample(void* object) const override;
    };

    // -------------
    // Property type
    // -------------
    class PropertyType: public Type
    {
    public:
        // Default constructor
        PropertyType(const String& name, int size, ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const;

        // Returns type of value
        const Type* GetValueType() const;

        // Returns value from property casted to void*
        virtual void* GetValueAsPtr(void* propertyPtr) const = 0;

    protected:
        const Type* mValueType;
    };

    // -------------------------
    // Specialized Property type
    // -------------------------
    template<typename _value_type, typename _property_type>
    class TPropertyType: public PropertyType
    {
        struct RealFieldGetter;
        struct NullFieldGetter;

    public:
        // Constructor
        TPropertyType();

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

        // Returns value from property casted to void*
        void* GetValueAsPtr(void* propertyPtr) const override;

        // Returns value from property
        _value_type GetValue(void* propertyPtr) const;
    };

    // ----------------------
    // Type of Vector<> value
    // ----------------------
    class VectorType: public Type
    {
    public:
        // Default constructor
        VectorType(const String& name, int size, ITypeSerializer* serializer);

        // Returns type usage
        virtual Usage GetUsage() const override;

        // Returns type of vector element
        const Type* GetElementType() const;

        // Returns size of vector by pointer
        virtual int GetObjectVectorSize(void* object) const = 0;

        // Sets size of vector by pointer
        virtual void SetObjectVectorSize(void* object, int size) const = 0;

        // Returns element's pointer by index
        virtual void* GetObjectVectorElementPtr(void* object, int idx) const = 0;

        // Returns element's value proxy by index
        virtual Ref<IAbstractValueProxy> GetObjectVectorElementProxy(void* object, int idx) const = 0;

        // Removes element at idx in vector
        virtual void RemoveObjectVectorElement(void* object, int idx) const = 0;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

        // Returns element field info. Element field info is same for all elements
        FieldInfo* GetElementFieldInfo() const;

        // Returns count field info. This field info can serialize and deserialize size of vector with storing and restoring elements
        FieldInfo* GetCountFieldInfo() const;

    protected:
        const Type* mElementType;
        FieldInfo* mElementFieldInfo;
        FieldInfo* mCountFieldInfo;

        template<typename _element_type>
        friend struct VectorCountFieldSerializer;
    };

    // -----------------------
    // Specialized vector type
    // -----------------------
    template<typename _element_type>
    class TVectorType: public VectorType
    {
    public:
        // Default constructor
        TVectorType();

        // Returns size of vector by pointer
        int GetObjectVectorSize(void* object) const override;

        // Sets size of vector by pointer
        void SetObjectVectorSize(void* object, int size) const override;

        // Returns element's pointer by index
        void* GetObjectVectorElementPtr(void* object, int idx) const override;

        // Returns element's value proxy by index
        Ref<IAbstractValueProxy> GetObjectVectorElementProxy(void* object, int idx) const override;

        // Removes element at idx in vector
        void RemoveObjectVectorElement(void* object, int idx) const override;

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // --------------------------
    // Type of Dictionary<> value
    // --------------------------
    class MapType: public Type
    {
    public:
        // Default constructor
        MapType(const Type* keyType, const Type* valueType, int size, ITypeSerializer* serializer);

        // Returns type usage
        virtual Usage GetUsage() const override;

        // Returns type of dictionary key
        const Type* GetKeyType() const;

        // Returns type of dictionary value
        const Type* GetValueType() const;

        // Returns size of dictionary by pointer
        int GetObjectDictionarySize(void* object) const;

        // Returns element's key pointer by index
        void* GetObjectDictionaryKeyPtr(void* object, int idx) const;

        // Returns element's value pointer by index
        void* GetObjectDictionaryValuePtr(void* object, int idx) const;

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

    protected:
        const Type* mKeyType;
        const Type* mValueType;

        Function<int(void*)>        mGetDictionaryObjectSizeFunc;
        Function<void* (void*, int)> mGetObjectDictionaryKeyPtrFunc;
        Function<void* (void*, int)> mGetObjectDictionaryValuePtrFunc;
    };

    template<typename _key_type, typename _value_type>
    class TMapType: public MapType
    {
    public:
        // Default constructor
        TMapType();

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // -------------------------------------------
    // Accessor<_return_type*, const String&> type
    // -------------------------------------------
    class StringPointerAccessorType: public Type
    {
    public:
        // Default constructor
        StringPointerAccessorType(const String& name, int size, ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const override;

        // Returns value pointer by key
        virtual void* GetValue(void* object, const String& key) const = 0;

        // Returns all values with keys
        virtual Map<String, void*> GetAllValues(void* object) const = 0;

        // Returns type of return values
        virtual const Type* GetReturnType() const = 0;
    };

    template<typename _return_type, typename _accessor_type>
    class TStringPointerAccessorType: public StringPointerAccessorType
    {
    public:
        TStringPointerAccessorType();

        // Returns filed pointer by path
        void* GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const override;

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;

        // Returns value pointer by key
        void* GetValue(void* object, const String& key) const override;

        // Returns all values with keys
        Map<String, void*> GetAllValues(void* object) const override;

        // Returns type of return values
        const Type* GetReturnType() const override;

    protected:
        const Type* mReturnType;
    };

    // ----------------
    // Enumeration type
    // ----------------
    class EnumType: public Type
    {
    public:
        // Constructor
        EnumType(const String& name, int size, ITypeSerializer* serializer);

        // Returns type usage
        Usage GetUsage() const;

        // Returns enum entries
        const Map<int, String>& GetEntries() const;

    protected:
        Map<int, String> mEntries;

        friend class Reflection;
    };

    // ---------------------
    // Specialized Enum type
    // ---------------------
    template<typename _type>
    class TEnumType: public EnumType
    {
    public:
        // Constructor
        TEnumType(const String& name, int size);

        // Creates sample copy and returns him
        void* CreateSample() const override;

        // Destroys sample
        void DestroySample(void* sample) const override;

        // Returns abstract value proxy for object value
        Ref<IAbstractValueProxy> GetValueProxy(void* object) const override;
    };

    // --------------------------
    // Fundamental type container
    // --------------------------
    template<typename _type>
    class FundamentalTypeContainer
    {
    protected:
        static Type* type;

        template<typename _type_>
        friend const Type& o2::GetTypeOf();

        friend class Reflection;

    public:
        template<typename _processor_type>
        static void InitializeType(_type* object, _processor_type& processor);
    };

    // --------------------------
    // Enumeration type container
    // --------------------------
    template<typename _type>
    class EnumTypeContainer
    {
    public:
        static const Map<int, String>& GetEntries() { return type->GetEntries(); }

    protected:
        static EnumType* type;

        template<typename _type_>
        friend const Type& o2::GetTypeOf();

        friend class Reflection;
    };

    template<typename _type>
    void __SetupType(Type* type)
    {
        _type::type = type;
    }
}

typedef void* (*GetValuePointerFuncPtr)(void*);

#if IS_SCRIPTING_SUPPORTED
#define DECLARE_SCRIPTING(CLASS, TEMPLATE_OPT)                                                                 \
    TEMPLATE_OPT o2::ScriptValue CLASS::GetScriptPrototype()                                                   \
    {                                                                                                          \
        static o2::ScriptValue proto = o2::ScriptValue::EmptyObject();                                         \
        return proto;                                                                                          \
    }                                                                                                          \
    TEMPLATE_OPT void CLASS::SetScriptValueContainer(o2::ScriptValue& value) const                             \
    {                                                                                                          \
        value.SetContainingObject(const_cast<CLASS*>(this));                                                   \
    }                                                                                                          \
    TEMPLATE_OPT void CLASS::ReflectIntoScriptValue(o2::ScriptValue& scriptValue) const                        \
    {}                                                                                                           
#else
#define DECLARE_SCRIPTING(CLASS, TEMPLATE_OPT)
#endif

#define DECLARE_CLASS(CLASS, REGISTRATOR_ID)                                                                   \
    extern void __RegisterClass__##REGISTRATOR_ID()                                                            \
    {                                                                                                          \
        o2::ReflectionInitializationTypeProcessor processor;                                                   \
        CLASS::template ProcessType<o2::ReflectionInitializationTypeProcessor>(0, processor);                  \
    }                                                                                                          \
    o2::Type* CLASS::type = o2::Reflection::InitializeType<CLASS>(#CLASS, false);                              \
    DECLARE_SCRIPTING(CLASS, )

#define DECLARE_TEMPLATE_CLASS(CLASS)                                                                          \
    template<> o2::Type* CLASS::type = o2::Reflection::InitializeType<CLASS>(#CLASS, true);                    \
    DECLARE_SCRIPTING(CLASS, template<>)

#define CLASS_BASES_META(CLASS)                                                                                \
    template<typename _type_processor> void CLASS::ProcessBaseTypes(CLASS* object, _type_processor& processor) \
    {                                                                                                          \
        typedef CLASS thisclass;                                                                               \
        processor.template StartBases<CLASS>(object, type);                                                                 

#define CLASS_FIELDS_META(CLASS)                                                                               \
    template<typename _type_processor> void CLASS::ProcessFields(CLASS* object, _type_processor& processor)    \
    {                                                                                                          \
        typedef CLASS thisclass;                                                                               \
        processor.template StartFields<CLASS>(object, type);                                                                 

#define CLASS_METHODS_META(CLASS)                                                                              \
    template<typename _type_processor> void CLASS::ProcessMethods(CLASS* object, _type_processor& processor)   \
    {                                                                                                          \
        typedef CLASS thisclass;                                                                               \
        processor.template StartMethods<CLASS>(object, type);

#define META_TEMPLATES(...) \
    template<__VA_ARGS__>

#define DECLARE_CLASS_MANUAL(CLASS, REGISTRATOR_ID) \
    DECLARE_CLASS(CLASS, REGISTRATOR_ID)

#define DECLARE_TEMPLATE_CLASS_MANUAL(CLASS) \
    DECLARE_TEMPLATE_CLASS(CLASS)

#define CLASS_BASES_META_MANUAL(CLASS) \
    CLASS_BASES_META(CLASS)

#define CLASS_FIELDS_META_MANUAL(CLASS) \
    CLASS_FIELDS_META(CLASS)

#define CLASS_METHODS_META_MANUAL(CLASS) \
    CLASS_METHODS_META(CLASS)

#define META_TEMPLATES_MANUAL(...) \
    META_TEMPLATES(__VA_ARGS__)

#define FUNDAMENTAL_META(NAME) \
    template<>                                                                                                                                \
    template<typename _type_processor> void FundamentalTypeContainer<NAME>::InitializeType(NAME* object, _type_processor& processor)          \
    {                                                                                                                                         \
        typedef NAME thisclass;                                                                                                               \
        processor.template StartFields<NAME>(object, type);

#define BASE_CLASS(CLASS) \
    processor.template BaseType<thisclass, CLASS>(object, type, #CLASS)

#define FIELD() \
    processor.StartField()

#define NAME(NAME) \
    template FieldBasics<thisclass, decltype(object->NAME)>(object, type, #NAME, (GetValuePointerFuncPtr)([](void* obj) { return (void*)&((thisclass*)obj)->NAME; }), object->NAME)

#define PUBLIC() \
    SetProtectSection(o2::ProtectSection::Public)

#define PRIVATE() \
    SetProtectSection(o2::ProtectSection::Private)

#define PROTECTED() \
    SetProtectSection(o2::ProtectSection::Protected)

#define DEFAULT_VALUE \
    SetDefaultValue

#define ATTRIBUTE(NAME) \
    template AddAttribute<NAME>()

#define ATTRIBUTES(...)

#define FUNCTION() \
    processor.StartFunction()

template<typename _class_type, typename _res_type, typename ... _args>
constexpr auto ResolveMethodPtr(_res_type(_class_type::*ptr)(_args ...)) { return ptr; }

template<typename _class_type, typename _res_type, typename ... _args>
constexpr auto ResolveMethodPtr(_res_type(_class_type::*ptr)(_args ...) const) { return ptr; }

#define SIGNATURE(RETURN_TYPE, NAME, ...) \
    template Signature<ResolveMethodPtr<thisclass, RETURN_TYPE, ##__VA_ARGS__>(&thisclass::NAME)>(object, type, #NAME)

#define CONSTRUCTOR(...) \
    template Constructor<thisclass, ##__VA_ARGS__>(object, type)

#define SIGNATURE_STATIC(RETURN_TYPE, NAME, ...) \
    template SignatureStatic<thisclass, RETURN_TYPE, ##__VA_ARGS__>(object, type, #NAME, &thisclass::NAME)

#define END_META }

#include "o2/Utils/Property.h"
#include "o2/Utils/Reflection/FieldInfo.h"
#include "o2/Utils/Reflection/FunctionInfo.h"
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Types/StringImpl.h"
#include "o2/Utils/Types/UID.h"
#include "o2/Utils/ValueProxy.h"
#include "o2/Utils/Reflection/TypeSerializer.h"
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // -------------------
    // Type implementation
    // -------------------

    template<typename _res_type, typename ... _args>
    _res_type Type::Invoke(const String& name, void* object, _args ... args) const
    {
        if (auto func = GetFunction(name))
            return func->Invoke<_res_type, _args ...>(object, args ...);

        return _res_type();
    }

    template<typename _res_type, typename ... _args>
    _res_type Type::InvokeStatic(const String& name, _args ... args) const
    {
        if (auto func = GetStaticFunction(name))
            return func->Invoke<_res_type, _args ...>(args ...);

        return _res_type();
    }

    // --------------------
    // TType implementation
    // --------------------

    template<typename _type>
    TType<_type>::TType(const String& name, int size):
        Type(name, size, mnew TypeSerializer<_type>())
    {}

    template<typename _type>
    void* TType<_type>::CreateSample() const
    {
        return mnew _type();
    }

    template<typename _type>
    Ref<RefCounterable> TType<_type>::CreateSampleRef() const
    {
        if constexpr (std::is_base_of<RefCounterable, _type>::value)
            return mmake<_type>();
        else
            return nullptr;
    }

    template<typename _type>
    void TType<_type>::DestroySample(void* sample) const
    {
        delete (_type*)sample;
    }

    template<typename _type>
    Ref<IAbstractValueProxy> TType<_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<_type>>((_type*)object);
    }

    // --------------------------
    // TObjectType implementation
    // --------------------------

    template<typename _type>
    TObjectType<_type>::TObjectType(const String& name, int size, void* (*castFromFunc)(void*), void* (*castToFunc)(void*)):
        ObjectType(name, size, castFromFunc, castToFunc, mnew TypeSerializer<_type>())
    {}

    template<typename _type>
    void* TObjectType<_type>::CreateSample() const
    {
        if constexpr (IsConstructible<_type>::value && std::is_constructible<_type>::value)
            return mnew _type();
        else
        {
            Assert(false, "Type isn't constructible");
            return nullptr;
        }
    }

    template<typename _type>
    Ref<RefCounterable> TObjectType<_type>::CreateSampleRef() const
    {
        if constexpr (std::is_base_of<RefCounterable, _type>::value)
        {
            if constexpr (HasCastToRefCounterable<_type>::value)
                return _type::CastToRefCounterable(mmake<_type>());
            else
                return mmake<_type>();
        }
        
        return nullptr;
    }

    template<typename _type>
    void TObjectType<_type>::DestroySample(void* sample) const
    {
        delete (_type*)sample;
    }

    template<typename _type>
    Ref<IAbstractValueProxy> TObjectType<_type>::GetValueProxy(void* object) const
    {
        if constexpr (std::is_copy_constructible<_type>::value)
            return mmake<PointerValueProxy<_type>>((_type*)object);
        else
        {
            Assert(false, "Type isn't copy constructible");
            return nullptr;
        }
    }

    // ------------------------------
    // FundamentalType implementation
    // ------------------------------

    template<typename _type>
    FundamentalType<_type>::FundamentalType(const String& name):
        TType<_type>(name, sizeof(_type))
    {}

    // -----------------------------
    // TPointerType<> implementation
    // -----------------------------

    template<typename _type>
    TPointerType<_type>::TPointerType(const Type * baseType) :
        PointerType(baseType, mnew TypeSerializer<_type*>())
    {}

    template<typename _type>
    void* TPointerType<_type>::CreateSample() const
    {
        return mnew void* ();
    }

    template<typename _type>
    void TPointerType<_type>::DestroySample(void* sample) const
    {
        delete (_type**)sample;
    }

    template<typename _type>
    Ref<IAbstractValueProxy> TPointerType<_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<_type*>>((_type**)object);
    }

    // -------------------------------
    // TReferenceType<> implementation
    // -------------------------------

    template<typename _type>
    TReferenceType<_type>::TReferenceType(const Type* baseType) :
        ReferenceType(baseType, mnew TypeSerializer<Ref<_type>>())
    {}

    template<typename _type>
    void* TReferenceType<_type>::CreateSample() const
    {
        return mnew Ref<_type>();
    }

    template<typename _type>
    Ref<IAbstractValueProxy> TReferenceType<_type>::GetValueProxy(void* object) const
    {
        return mmake<RefPointerValueProxy<_type>>((Ref<_type>*)object);
    }

    template<typename _type>
    void TReferenceType<_type>::DestroySample(void* sample) const
    {
        delete (Ref<_type>*)sample;
    }

    template<typename _type>
    void* TReferenceType<_type>::GetObjectRawPtr(void* refPtr) const
    {
        return ((Ref<_type>*)refPtr)->Get();
    }

    template<typename _type>
    void TReferenceType<_type>::SetObjectRawPtr(void* object, void* refPtr)
    {
        *((Ref<_type>*)refPtr) = Ref<_type>((_type*)object);
    }

    template<typename _type>
    void* TReferenceType<_type>::CreateSample(void* object) const
    {
        return mnew Ref<_type>((_type*)object);
    }

    // ------------------------------
    // TPropertyType<> implementation
    // ------------------------------

    template<typename _value_type, typename _property_type>
    TPropertyType<_value_type, _property_type>::TPropertyType():
        PropertyType((String)(typeid(_property_type).name()) + (String)"<" + TypeOf(_value_type).GetName() + ">", sizeof(_value_type),
                     mnew TypeSerializer<_property_type>())
    {
        mValueType = &GetTypeOf<_value_type>();
    }

    template<typename _value_type, typename _property_type>
    void* TPropertyType<_value_type, _property_type>::CreateSample() const
    {
        return nullptr;
    }

    template<typename _value_type, typename _property_type>
    void TPropertyType<_value_type, _property_type>::DestroySample(void* sample) const
    {}

    template<typename _value_type, typename _property_type>
    Ref<IAbstractValueProxy> TPropertyType<_value_type, _property_type>::GetValueProxy(void* object) const
    {
        return mmake<PropertyValueProxy<_value_type, _property_type>>((_property_type*)object);
    }

    template<typename _value_type, typename _property_type>
    void* TPropertyType<_value_type, _property_type>::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        if constexpr (std::is_pointer<_value_type>::value)
        {
            _property_type* prop = (_property_type*)object;
            _value_type valuePtr = prop->Get();
            return mValueType->GetFieldPtr(&valuePtr, path, fieldInfo);
        }

        return nullptr;
    }

    template<typename _value_type, typename _property_type>
    void* TPropertyType<_value_type, _property_type>::GetValueAsPtr(void* propertyPtr) const
    {
        if constexpr (std::is_pointer<_value_type>::value)
        {
            _property_type* prop = (_property_type*)propertyPtr;
            return prop->Get();
        }

        return nullptr;
    }

    template<typename _value_type, typename _property_type>
    _value_type TPropertyType<_value_type, _property_type>::GetValue(void* propertyPtr) const
    {
        _property_type* prop = (_property_type*)propertyPtr;
        return prop->Get();
    }

    // --------------------------
    // TVectorType implementation
    // --------------------------

    template<typename _element_type>
    struct VectorCountFieldSerializer: public ITypeSerializer
    {
        VectorCountFieldSerializer() { }
        void Serialize(void* object, DataValue& data) const;
        void Deserialize(void* object, DataValue& data) const;
        bool Equals(void* objectA, void* objectB) const;
        void Copy(void* objectA, void* objectB) const;
        ITypeSerializer* Clone() const;
    };

    template<typename _element_type>
    void* TVectorType<_element_type>::GetObjectVectorElementPtr(void* object, int idx) const
    {
        if (idx < 0 || idx >= ((Vector<_element_type>*)object)->Count())
            return nullptr;

        return &((Vector<_element_type>*)object)->Get(idx);
    }

    template<typename _element_type>
    Ref<IAbstractValueProxy> TVectorType<_element_type>::GetObjectVectorElementProxy(void* object, int idx) const
    {
        if (idx < 0 || idx >= ((Vector<_element_type>*)object)->Count())
            return nullptr;

        return mElementType->GetValueProxy(&((Vector<_element_type>*)object)->Get(idx));
    }

    template<typename _element_type>
    void TVectorType<_element_type>::SetObjectVectorSize(void* object, int size) const
    {
        auto vectorObj = ((Vector<_element_type>*)object);
        int oldSize = vectorObj->Count();
        vectorObj->Resize(size);

        for (int i = oldSize; i < size; i++)
            (*vectorObj)[i] = _element_type();
    }

    template<typename _element_type>
    void TVectorType<_element_type>::RemoveObjectVectorElement(void* object, int idx) const
    {
        ((Vector<_element_type>*)object)->RemoveAt(idx);
    }

    template<typename _element_type>
    void* TVectorType<_element_type>::CreateSample() const
    {
        return mnew Vector<_element_type>();
    }

    template<typename _element_type>
    void TVectorType<_element_type>::DestroySample(void* sample) const
    {
        delete (Vector<_element_type>*)sample;
    }

    template<typename _element_type>
    Ref<IAbstractValueProxy> TVectorType<_element_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<Vector<_element_type>>>((Vector<_element_type>*)object);
    }

    template<typename _element_type>
    int TVectorType<_element_type>::GetObjectVectorSize(void* object) const
    {
        return ((Vector<_element_type>*)object)->Count();
    }

    template<typename _element_type>
    TVectorType<_element_type>::TVectorType():
        VectorType((String)"o2::Vector<" + GetTypeOf<_element_type>().GetName() + ">", sizeof(Vector<_element_type>), mnew TypeSerializer<Vector<_element_type>>())
    {
        mElementType = &GetTypeOf<_element_type>();

        mElementFieldInfo = mnew FieldInfo(this, "element", 0, mElementType, ProtectSection::Private);
        mCountFieldInfo = mnew FieldInfo(this, "count", 0, &GetTypeOf<int>(), ProtectSection::Public,
                                         mnew DefaultValue<int>(0),
                                         mnew VectorCountFieldSerializer<_element_type>());
    }

    template<typename _element_type>
    void VectorCountFieldSerializer<_element_type>::Serialize(void* object, DataValue& data) const
    {
        const VectorType& type = (const VectorType&)(GetTypeOf<Vector<_element_type>>());

        int size = type.GetObjectVectorSize(object);
        data["Size"].Set(size);

        DataValue& elements = data["Elements"];
        for (int i = 0; i < size; i++)
        {
            void* elementPtr = type.GetObjectVectorElementPtr(object, i);
            type.mElementFieldInfo->Serialize(elementPtr, elements.AddMember("Element" + (String)i));
        }
    }

    template<typename _element_type>
    void VectorCountFieldSerializer<_element_type>::Deserialize(void* object, DataValue& data) const
    {
        const VectorType& type = (const VectorType&)(GetTypeOf<Vector<_element_type>>());
        int size = type.GetObjectVectorSize(object);
        int newSize;
        data["Size"].Get(newSize);
        type.SetObjectVectorSize(object, newSize);

        if (auto elementsData = data.GetMember("Elements"))
        {
            for (int i = size; i < newSize; i++)
            {
                if (auto elementData = elementsData.GetMember("Element" + (WString)i))
                {
                    void* elementPtr = type.GetObjectVectorElementPtr(object, i);
                    type.mElementFieldInfo->Deserialize(elementPtr, elementData);
                }
            }
        }
    }

    template<typename _element_type>
    bool VectorCountFieldSerializer<_element_type>::Equals(void* objectA, void* objectB) const
    {
        const VectorType& type = (const VectorType&)(GetTypeOf<Vector<_element_type>>());
        return type.GetObjectVectorSize(objectA) == type.GetObjectVectorSize(objectB);
    }

    template<typename _element_type>
    void VectorCountFieldSerializer<_element_type>::Copy(void* objectA, void* objectB) const
    {
        const VectorType& type = (const VectorType&)(GetTypeOf<Vector<_element_type>>());
        type.SetObjectVectorSize(objectA, type.GetObjectVectorSize(objectB));
    }

    template<typename _element_type>
    ITypeSerializer* VectorCountFieldSerializer<_element_type>::Clone() const
    {
        return mnew VectorCountFieldSerializer();
    }

    // ------------------------------
    // TDictionaryType implementation
    // ----------------------------- -

    template<typename _key_type, typename _value_type>
    TMapType<_key_type, _value_type>::TMapType():
        MapType(&GetTypeOf<_key_type>(), &GetTypeOf<_value_type>(), sizeof(Map<_key_type, _value_type>), mnew TypeSerializer<Map<_key_type, _value_type>>())
    {
        mGetDictionaryObjectSizeFunc = [](void* obj) { return ((Map<_key_type, _value_type>*)obj)->Count(); };

        mGetObjectDictionaryKeyPtrFunc = [](void* obj, int idx) {
            auto it = ((Map<_key_type, _value_type>*)obj)->Begin();
            while (idx >= 0) it++;

            return (void*)(&it->first);
        };

        mGetObjectDictionaryValuePtrFunc = [](void* obj, int idx) {
            auto it = ((Map<_key_type, _value_type>*)obj)->Begin();
            while (idx >= 0) it++;

            return (void*)(&it->second);
        };
    }

    template<typename _key_type, typename _value_type>
    void* TMapType<_key_type, _value_type>::CreateSample() const
    {
        return mnew Map<_key_type, _value_type>();
    }

    template<typename _key_type, typename _value_type>
    void TMapType<_key_type, _value_type>::DestroySample(void* sample) const
    {
        delete (Map<_key_type, _value_type>*)sample;
    }

    template<typename _key_type, typename _value_type>
    Ref<IAbstractValueProxy> TMapType<_key_type, _value_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<Map<_key_type, _value_type>>>((Map<_key_type, _value_type>*)object);
    }

    // ----------------------------------------
    // StringPointerAccessorType implementation
    // ----------------------------------------

    template<typename _return_type, typename _accessor_type>
    TStringPointerAccessorType<_return_type, _accessor_type>::TStringPointerAccessorType():
        StringPointerAccessorType((String)(typeid(_accessor_type).name()) + (String)"<" + TypeOf(_return_type).GetName() + ">",
                                  sizeof(_accessor_type), mnew TypeSerializer<_accessor_type>())
    {
        mReturnType = &GetTypeOf<_return_type>();
    }

    template<typename _return_type, typename _accessor_type>
    void* TStringPointerAccessorType<_return_type, _accessor_type>::CreateSample() const
    {
        return nullptr;
    }

    template<typename _return_type, typename _accessor_type>
    void TStringPointerAccessorType<_return_type, _accessor_type>::DestroySample(void* sample) const
    {}

    template<typename _return_type, typename _accessor_type>
    Ref<IAbstractValueProxy> TStringPointerAccessorType<_return_type, _accessor_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<_accessor_type>>((_accessor_type*)object);
    }

    template<typename _return_type, typename _accessor_type>
    void* TStringPointerAccessorType<_return_type, _accessor_type>::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        _accessor_type* accessor = (_accessor_type*)object;
        if (_return_type value = accessor->Get(pathPart))
        {
            if constexpr (IsRef<_return_type>::value)
            {
                if (delPos < 0)
                    return value.Get();
            }

            return mReturnType->GetFieldPtr(&value, path.SubStr(delPos + 1), fieldInfo);
        }

        return nullptr;
    }

    template<typename _return_type, typename _accessor_type>
    void* TStringPointerAccessorType<_return_type, _accessor_type>::GetValue(void* object, const String& key) const
    {
        _accessor_type* accessor = (_accessor_type*)object;
        return (void*)(accessor->Get(key).Get());
    }

    template<typename _return_type, typename _accessor_type>
    Map<String, void*> TStringPointerAccessorType<_return_type, _accessor_type>::GetAllValues(void* object) const
    {
        _accessor_type* accessor = (_accessor_type*)object;
        auto all = accessor->GetAll();
        Map<String, void*> res;
        for (auto& kv : all)
            res.Add(kv.first, (void*)(kv.second.Get()));

        return res;
    }

    template<typename _return_type, typename _accessor_type>
    const Type* TStringPointerAccessorType<_return_type, _accessor_type>::GetReturnType() const
    {
        return mReturnType;
    }

    // ------------------------
    // TEnumType implementation
    // ------------------------

    template<typename _type>
    TEnumType<_type>::TEnumType(const String& name, int size):
        EnumType(name, size, mnew TypeSerializer<_type>())
    {}

    template<typename _type>
    void* TEnumType<_type>::CreateSample() const
    {
        return mnew _type();
    }

    template<typename _type>
    void TEnumType<_type>::DestroySample(void* sample) const
    {
        delete (_type*)sample;
    }

    template<typename _type>
    Ref<IAbstractValueProxy> TEnumType<_type>::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<_type>>((_type*)object);
    }

    FUNDAMENTAL_META(RectF)
    {
        FIELD().PUBLIC().NAME(left);
        FIELD().PUBLIC().NAME(right);
        FIELD().PUBLIC().NAME(top);
        FIELD().PUBLIC().NAME(bottom);
    }
    END_META;

    FUNDAMENTAL_META(RectI)
    {
        FIELD().PUBLIC().NAME(left);
        FIELD().PUBLIC().NAME(right);
        FIELD().PUBLIC().NAME(top);
        FIELD().PUBLIC().NAME(bottom);
    }
    END_META;

    FUNDAMENTAL_META(BorderF)
    {
        FIELD().PUBLIC().NAME(left);
        FIELD().PUBLIC().NAME(right);
        FIELD().PUBLIC().NAME(top);
        FIELD().PUBLIC().NAME(bottom);
    }
    END_META;

    FUNDAMENTAL_META(BorderI)
    {
        FIELD().PUBLIC().NAME(left);
        FIELD().PUBLIC().NAME(right);
        FIELD().PUBLIC().NAME(top);
        FIELD().PUBLIC().NAME(bottom);
    }
    END_META;

    FUNDAMENTAL_META(Vec2F)
    {
        FIELD().PUBLIC().NAME(x);
        FIELD().PUBLIC().NAME(y);
    }
    END_META;

    FUNDAMENTAL_META(Vec2I)
    {
        FIELD().PUBLIC().NAME(x);
        FIELD().PUBLIC().NAME(y);
    }
    END_META;

    FUNDAMENTAL_META(Color4)
    {
        FIELD().PUBLIC().NAME(r);
        FIELD().PUBLIC().NAME(g);
        FIELD().PUBLIC().NAME(b);
        FIELD().PUBLIC().NAME(a);
    }
    END_META;

    FUNDAMENTAL_META(int) END_META;
    FUNDAMENTAL_META(bool) END_META;
    FUNDAMENTAL_META(char) END_META;
    FUNDAMENTAL_META(wchar_t) END_META;
    FUNDAMENTAL_META(short int) END_META;
    FUNDAMENTAL_META(long int) END_META;
    FUNDAMENTAL_META(long long int) END_META;
    FUNDAMENTAL_META(unsigned char) END_META;
    FUNDAMENTAL_META(unsigned short int) END_META;
    FUNDAMENTAL_META(unsigned int) END_META;
    FUNDAMENTAL_META(unsigned long int) END_META;
    FUNDAMENTAL_META(unsigned long long int) END_META;
    FUNDAMENTAL_META(float) END_META;
    FUNDAMENTAL_META(double) END_META;
    FUNDAMENTAL_META(long double) END_META;
    FUNDAMENTAL_META(Basis) END_META;
    FUNDAMENTAL_META(Vertex) END_META;
    FUNDAMENTAL_META(String) END_META;
    FUNDAMENTAL_META(WString) END_META;
    FUNDAMENTAL_META(DataValue) END_META;
    FUNDAMENTAL_META(UID) END_META;
    FUNDAMENTAL_META(ScriptValue) END_META;
}
