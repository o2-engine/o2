#pragma once

#include <functional>
#include <type_traits>
#include "o2/Utils/Types/Containers/Pair.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/StringDef.h"

// Reflection access macros
#define o2Reflection Reflection::Instance()

namespace o2
{
    class EnumType;
    class FieldInfo;
    class FunctionInfo;
    class FunctionType;
    class IAttribute;
    class IObject;
    class MapType;
    class PropertyType;
    class ReflectionInitializationTypeProcessor;
    class StaticFunctionInfo;
    class Type;
    class VectorType;
    struct IDefaultValue;

    template<typename _return_type, typename _accessor_type>
    class TStringPointerAccessorType;

    template<typename _type, typename _getter>
    const Type& GetTypeOf();

    template<typename _type>
    class Ref;

    class RefCounterable;

    typedef UInt TypeId;

    // ------------------------------
    // Reflection in application container
    // ------------------------------
    class Reflection
    {
    public:
        // Returns reflection instance
        static Reflection& Instance();

        // Initializes all types
        static void InitializeTypes();

        // Returns array of all registered types
        static const Map<String, Type*>& GetTypes();

        // Returns a copy of type sample
        static void* CreateTypeSample(const String& typeName);

        // Returns a copy of type sample reference
        static Ref<RefCounterable> CreateTypeSampleRef(const String& typeName);

        // Returns type by name
        static const Type* GetType(const String& name);

        // Returns enum value from string
        template<typename _type>
        static _type GetEnumValue(const String& name);

        // Returns enum name from value
        template<typename _type>
        static String GetEnumName(_type value);

        // Returns is types was initialized
        static bool IsTypesInitialized();

    public:
        // Initializes regular type
        template<typename _type>
        static Type* InitializeType(const char* name, bool autoDefferedInit = false);

        // Initialized fundamental type
        template<typename _type>
        static Type* InitializeFundamentalType(const char* name);

        // Initializes function type
        static FunctionType* InitializeFunctionType(const char* name);

        // Initializes enum
        template<typename _type>
        static EnumType* InitializeEnum(const char* name, std::function<Map<int, String>()> func);

        // Returns pointer type
        template<typename _type>
        static const Type* GetPointerType();

        // Returns reference type
        template<typename _type>
        static const Type* GetReferenceType();

        // Returns property type 
        template<typename _value_type, typename _property_type>
        static const PropertyType* GetPropertyType();

        // Returns vector type
        template<typename _element_type>
        static const VectorType* GetVectorType();

        // Returns dictionary type
        template<typename _key_type, typename _value_type>
        static const MapType* GetMapType();

        // Returns accessor type
        template<typename _return_type, typename _accessor_type>
        static const TStringPointerAccessorType<_return_type, _accessor_type>* GetAccessorType();

        // Type dynamic casting function template
        template<typename _source_type, typename _target_type>
        static void* CastFunc(void* obj) { return dynamic_cast<_target_type*>((_source_type*)obj); }

        // Fake type casting function
        static void* NoCastFunc(void* obj) { return obj; }

    protected:
        typedef void(*TypeInitializingFunc)(void*, ReflectionInitializationTypeProcessor&);
        typedef Vector<TypeInitializingFunc> TypeInitializingFuncsVec;

        static Reflection* mInstance; // Reflection instance

        Map<String, Type*> mTypes;           // All registered types
        UInt               mLastGivenTypeId; // Last given type index

        TypeInitializingFuncsVec mInitializingFunctions; // List of types initializations functions

        bool mTypesInitialized = false;

    protected:
        // Constructor. Initializes dummy type
        Reflection();

        // Destructor. Destroys types
        ~Reflection();

        // Initializes fundamental types
        static void InitializeFundamentalTypes();

        friend class Type;
    };
}

#define DECLARE_FUNDAMENTAL_TYPE(TYPE) \
    template<>                         \
    o2::Type* o2::FundamentalTypeContainer<TYPE>::type = o2::Reflection::InitializeFundamentalType<TYPE>(#TYPE)

#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Reflection/Enum.h"
#include "o2/Utils/Reflection/FieldInfo.h"
#include "o2/Utils/Reflection/FunctionInfo.h"
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Types/Ref.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Reflection/BaseTypeProcessor.h"
#endif

namespace o2 {
    class ReflectionInitializationTypeProcessor : public BaseTypeProcessor {
    public:
        struct FieldProcessor {
            Vector<IAttribute *> attributes;
            IDefaultValue *defaultValue = nullptr;
            ProtectSection section = ProtectSection::Public;

            template<typename _attribute_type, typename ... _args>
            FieldProcessor &AddAttribute(_args ... args);

            template<typename _type>
            FieldProcessor &SetDefaultValue(const _type &value);

            template<typename _object_type, typename _field_type>
            FieldInfo &FieldBasics(_object_type *object, Type *type, const char *name, void *(*pointerGetter)(void *),
                                   _field_type &field);

            FieldProcessor &SetProtectSection(ProtectSection section);
        };

        struct FunctionProcessor {
            Vector<IAttribute *> attributes;
            ProtectSection section = ProtectSection::Public;

            template<typename _attribute_type, typename ... _args>
            FunctionProcessor &AddAttribute(_args ... args);

            FunctionProcessor &SetProtectSection(ProtectSection section);

            template<typename _object_type, typename ... _args>
            void Constructor(_object_type *object, Type *type) {}

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type *object, Type *type, const char *name,
                           _res_type(_object_type::* pointer)(_args ...));

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type *object, Type *type, const char *name,
                           _res_type(_object_type::* pointer)(_args ...) const);

            template<auto pointer, typename _object_type>
            void Signature(_object_type *object, Type *type, const char *name)
            {
                Signature(object, type, name, pointer);
            }

            template<typename _object_type, typename _res_type, typename ... _args>
            void SignatureStatic(_object_type *object, Type *type, const char *name,
                                 _res_type(*pointer)(_args ...));
        };

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type *object, Type *type, const char *name);

        FieldProcessor StartField();

        FunctionProcessor StartFunction();
    };
}

#include "o2/Utils/Reflection/Type.h"

namespace o2
{
    template<typename _type>
    _type Reflection::GetEnumValue(const String& name)
    {
        EnumType* type = (EnumType*)(&GetTypeOf<_type>());
        auto& entries = type->GetEntries();

        if (entries.ContainsValue(name))
            return (_type)(entries.FindValue(name).first);

        return (_type)0;
    }

    template<typename _type>
    String Reflection::GetEnumName(_type value)
    {
        String res;

        EnumType* type = (EnumType*)(&GetTypeOf<_type>());
        auto& entries = type->GetEntries();

        entries.TryGetValue((int)value, res);

        return res;
    }

    template<typename _type>
    Type* Reflection::InitializeType(const char* name, bool autoDefferedInit /*= false*/)
    {
        Type* res = mnew TObjectType<_type>(name, sizeof(_type), &CastFunc<IObject, _type>,
                                            &CastFunc<_type, IObject>);

        res->mId = Reflection::Instance().mLastGivenTypeId++;

        if (autoDefferedInit)
            Reflection::Instance().mInitializingFunctions.Add((TypeInitializingFunc)&_type::template ProcessType<ReflectionInitializationTypeProcessor>);

#if IS_SCRIPTING_SUPPORTED
        ScriptEngine::GetRegisterConstructorFuncs().Add((ScriptEngine::RegisterConstructorFunc)&_type::template ProcessType<ScriptPrototypeProcessor>);
#endif

        mInstance->mTypes[res->GetName()] = res;

        //printf("Reflection::InitializeType(%s): instance:%x - %i\n", name, mInstance, Reflection::Instance().mTypes.Count());

        return res;
    }

    template<typename _type>
    Type* Reflection::InitializeFundamentalType(const char* name)
    {
        Type* res = mnew FundamentalType<_type>(name);

        Reflection::Instance().mInitializingFunctions.Add((TypeInitializingFunc)&FundamentalTypeContainer<_type>::template InitializeType<ReflectionInitializationTypeProcessor>);
        res->mId = Reflection::Instance().mLastGivenTypeId++;
        mInstance->mTypes[res->GetName()] = res;

        return res;
    }

    template<typename _type>
    const Type* Reflection::GetPointerType()
    {
        const Type* baseType = &GetTypeOf<_type>();

        if (baseType->mPtrType)
            return baseType->mPtrType;

        TPointerType<_type>* newType = mnew TPointerType<_type>(baseType);
        newType->mId = mInstance->mLastGivenTypeId++;

        baseType->mPtrType = newType;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _type>
    const Type* Reflection::GetReferenceType()
    {
        const Type* baseType = &GetTypeOf<_type>();

        if (baseType->mRefType)
            return baseType->mRefType;

        TReferenceType<_type>* newType = mnew TReferenceType<_type>(baseType);
        newType->mId = mInstance->mLastGivenTypeId++;

        baseType->mRefType = newType;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _type>
    EnumType* Reflection::InitializeEnum(const char* name, std::function<Map<int, String>()> func)
    {
        EnumType* res = mnew TEnumType<_type>(name, sizeof(_type));

        res->mId = Reflection::Instance().mLastGivenTypeId++;
        mInstance->mTypes[res->GetName()] = res;
        res->mEntries.Add(func());

        return res;
    }

    template<typename _value_type, typename _property_type>
    const PropertyType* Reflection::GetPropertyType()
    {
        String typeName = (String)(typeid(_property_type).name()) + (String)"<" + TypeOf(_value_type).GetName() + ">";

        auto fnd = mInstance->mTypes.find(typeName);
        if (fnd != mInstance->mTypes.End())
            return dynamic_cast<PropertyType*>(fnd->second);

        TPropertyType<_value_type, _property_type>* newType = mnew TPropertyType<_value_type, _property_type>();
        newType->mId = mInstance->mLastGivenTypeId++;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _element_type>
    const VectorType* Reflection::GetVectorType()
    {
        String typeName = "o2::Vector<" + TypeOf(_element_type).GetName() + ">";

        auto fnd = mInstance->mTypes.find(typeName);
        if (fnd != mInstance->mTypes.End())
            return dynamic_cast<VectorType*>(fnd->second);

        TVectorType<_element_type>* newType = mnew TVectorType<_element_type>();
        newType->mId = mInstance->mLastGivenTypeId++;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _key_type, typename _value_type>
    const MapType* Reflection::GetMapType()
    {
        String typeName = "o2::Dictionary<" + TypeOf(_key_type).GetName() + ", " + TypeOf(_value_type).GetName() + ">";

        auto fnd = mInstance->mTypes.find(typeName);
        if (fnd != mInstance->mTypes.End())
            return dynamic_cast<MapType*>(fnd->second);

        auto newType = mnew TMapType<_key_type, _value_type>();
        newType->mId = mInstance->mLastGivenTypeId++;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _return_type, typename _accessor_type>
    const TStringPointerAccessorType<_return_type, _accessor_type>* Reflection::GetAccessorType()
    {
        String typeName = (String)(typeid(_accessor_type).name()) + (String)"<" + TypeOf(_return_type).GetName() + ">";

        auto fnd = mInstance->mTypes.find(typeName);
        if (fnd != mInstance->mTypes.End())
            return dynamic_cast<TStringPointerAccessorType<_return_type, _accessor_type>*>(fnd->second);

        TStringPointerAccessorType<_return_type, _accessor_type>* newType = mnew TStringPointerAccessorType<_return_type, _accessor_type>();
        newType->mId = mInstance->mLastGivenTypeId++;

        mInstance->mTypes[newType->GetName()] = newType;

        return newType;
    }

    template<typename _source_type, typename _target_type>
    struct CastSelector
    {
        struct DefaultCast
        {
            static void* CastFunc(void* obj) { return Reflection::CastFunc<_source_type, _target_type>(obj); }
        };

        struct NoCast
        {
            static void* CastFunc(void* obj) { return nullptr; }
        };

        using CastType = std::conditional<std::is_polymorphic<_source_type>::value, DefaultCast, NoCast>::type;
    };

    template<typename _object_type, typename _base_type>
    void ReflectionInitializationTypeProcessor::BaseType(_object_type* object, Type* type, const char* name)
    {
        using _base_under = typename std::conditional<std::is_base_of<IObject, _base_type>::value, _base_type, Type::Dummy>::type;

        if constexpr (std::is_same<_base_under, Type::Dummy>::value)
            return;

        Type::BaseType baseTypeInfo;
        baseTypeInfo.type = _base_under::type;
        baseTypeInfo.dynamicCastUpFunc = &CastSelector<_object_type, _base_type>::CastType::CastFunc;
        baseTypeInfo.dynamicCastDownFunc = &CastSelector<_base_type, _object_type>::CastType::CastFunc;

        type->mBaseTypes.Add(baseTypeInfo);
    }

    template<typename _attribute_type, typename ... _args>
    ReflectionInitializationTypeProcessor::FieldProcessor& ReflectionInitializationTypeProcessor::FieldProcessor::AddAttribute(_args ... args)
    {
        attributes.Add(mnew _attribute_type(args ...));
        return *this;
    }

    template<typename _type>
    ReflectionInitializationTypeProcessor::FieldProcessor& ReflectionInitializationTypeProcessor::FieldProcessor::SetDefaultValue(const _type& value)
    {
        if constexpr (std::is_copy_constructible<_type>::value && SupportsEqualOperator<_type>::value)
            defaultValue = mnew DefaultValue<_type>(value);

        return *this;
    }

    template<typename _object_type, typename _field_type>
    FieldInfo& ReflectionInitializationTypeProcessor::FieldProcessor::FieldBasics(_object_type* object, Type* type,
                                                                                  const char* name,
                                                                                  void* (*pointerGetter)(void*),
                                                                                  _field_type& field)
    {
        auto valType = &TypeOf(_field_type);
        type->mFields.emplace_back(FieldInfo(type, name, pointerGetter, valType, ProtectSection::Private));
        type->mFields.Last().mAttributes.Add(attributes);
        type->mFields.Last().mDefaultValue = defaultValue;
        type->mFields.Last().SetProtectSection(section);

        return type->mFields.Last();
    }

    template<typename _attribute_type, typename ... _args>
    ReflectionInitializationTypeProcessor::FunctionProcessor& ReflectionInitializationTypeProcessor::FunctionProcessor::AddAttribute(_args ... args)
    {
        attributes.Add(mnew _attribute_type(args ...));
        return *this;
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ReflectionInitializationTypeProcessor::FunctionProcessor::Signature(_object_type* object, Type* type, const char* name,
                                                                             _res_type(_object_type::* pointer)(_args ...))
    {
        auto retType = &TypeOf(_res_type);

        auto funcInfo = mnew SpecFunctionInfo<_object_type, _res_type, _args ...>();
        funcInfo->mName = name;
        funcInfo->mFunctionPtr = pointer;
        funcInfo->mReturnType = retType;
        funcInfo->mIsContant = false;
        funcInfo->mProtectSection = section;
        funcInfo->mOwnerType = type;
        funcInfo->mAttributes = attributes;

        [](...) {}((funcInfo->mParameters.Add(&TypeOf(_args)))...);

        type->mFunctions.Add(funcInfo);
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ReflectionInitializationTypeProcessor::FunctionProcessor::Signature(_object_type* object, Type* type, const char* name,
                                                                             _res_type(_object_type::* pointer)(_args ...) const)
    {
        auto retType = &TypeOf(_res_type);

        auto funcInfo = mnew SpecConstFunctionInfo<_object_type, _res_type, _args ...>();
        funcInfo->mName = name;
        funcInfo->mFunctionPtr = pointer;
        funcInfo->mReturnType = retType;
        funcInfo->mIsContant = false;
        funcInfo->mProtectSection = section;
        funcInfo->mOwnerType = type;
        funcInfo->mAttributes = attributes;
        type->mFunctions.Add(funcInfo);
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ReflectionInitializationTypeProcessor::FunctionProcessor::SignatureStatic(_object_type* object, Type* type,
                                                                                   const char* name, _res_type(*pointer)(_args ...))
    {
        auto retType = &TypeOf(_res_type);

        auto funcInfo = mnew SpecStaticFunctionInfo<_res_type, _args ...>();
        funcInfo->mName = name;
        funcInfo->mFunctionPtr = pointer;
        funcInfo->mReturnType = retType;
        funcInfo->mProtectSection = section;
        funcInfo->mOwnerType = type;
        funcInfo->mAttributes = attributes;
        type->mStaticFunctions.Add(funcInfo);
    }
}
