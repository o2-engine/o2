#include "o2/stdafx.h"
#include "Type.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/System/Time/Timer.h"

namespace o2
{
    Type::Type(const String& name, int size, ITypeSerializer* serializer):
        mId(0), mPtrType(nullptr), mName(name), mSize(size), mSerializer(serializer)
    {}

    Type::~Type()
    {
        for (auto func : mFunctions)
            delete func;
    }

    bool Type::operator!=(const Type& other) const
    {
        return other.mId != mId;
    }

    bool Type::operator==(const Type& other) const
    {
        return other.mId == mId;
    }

    const String& Type::GetName() const
    {
        return mName;
    }

    TypeId Type::ID() const
    {
        return mId;
    }

    int Type::GetSize() const
    {
        return mSize;
    }

    bool Type::IsBasedOn(const Type& other) const
    {
        if (mId == other.mId)
            return true;

        for (auto typeInfo : mBaseTypes)
        {
            if (typeInfo.type->mId == mId)
                return true;

            if (typeInfo.type->IsBasedOn(other))
                return true;
        }

        return false;
    }

    Type::Usage Type::GetUsage() const
    {
        return Usage::Regular;
    }

    const Vector<Type::BaseType>& Type::GetBaseTypes() const
    {
        return mBaseTypes;
    }

    const Vector<FieldInfo>& Type::GetFields() const
    {
        return mFields;
    }

    Vector<const FieldInfo*> Type::GetFieldsWithBaseClasses() const
    {
        Vector<const FieldInfo*> res;

        for (auto baseType : mBaseTypes)
            res += baseType.type->GetFieldsWithBaseClasses();

        res += mFields.Convert<const FieldInfo*>([](auto& x) { return &x; });

        return res;
    }

    const Vector<FunctionInfo*>& Type::GetFunctions() const
    {
        return mFunctions;
    }

    const Vector<StaticFunctionInfo*>& Type::GetStaticFunctions() const
    {
        return mStaticFunctions;
    }

    Vector<FunctionInfo*> Type::GetFunctionsWithBaseClasses() const
    {
        Vector<FunctionInfo*> res;

        for (auto baseType : mBaseTypes)
            res += baseType.type->GetFunctionsWithBaseClasses();

        res += mFunctions;

        return res;
    }

    Vector<StaticFunctionInfo*> Type::GetStaticFunctionsWithBaseClasses() const
    {
        Vector<StaticFunctionInfo*> res;

        for (auto baseType : mBaseTypes)
            res += baseType.type->GetStaticFunctionsWithBaseClasses();

        res += mStaticFunctions;

        return res;
    }

    const FieldInfo* Type::GetField(const String& name) const
    {
        for (auto& field : mFields)
        {
            if (field.GetName() == name)
                return &field;
        }

        for (auto baseType : mBaseTypes)
        {
            if (auto res = baseType.type->GetField(name))
                return res;
        }

        return nullptr;
    }

    const FunctionInfo* Type::GetFunction(const String& name) const
    {
        for (auto func : mFunctions)
        {
            if (func->mName == name)
                return func;
        }

        for (auto& base : mBaseTypes)
        {
            if (auto func = base.type->GetFunction(name))
                return func;
        }

        return nullptr;
    }

    const StaticFunctionInfo* Type::GetStaticFunction(const String& name) const
    {
        for (auto func : mStaticFunctions)
        {
            if (func->mName == name)
                return func;
        }

        for (auto& base : mBaseTypes)
        {
            if (auto func = base.type->GetStaticFunction(name))
                return func;
        }

        return nullptr;
    }

    Vector<const Type*> Type::GetDerivedTypes(bool deep /*= true*/) const
    {
        Vector<const Type*> res;
        for (auto kv : Reflection::GetTypes())
        {
            auto baseTypes = kv.second->GetBaseTypes();
            for (auto& baseType : baseTypes)
            {
                if (baseType.type->mId == mId)
                    res.Add(kv.second);
            }
        }

        if (deep)
        {
            auto resCopy = res;
            for (auto type : resCopy)
                res += type->GetDerivedTypes(true);
        }

        return res;
    }

    Ref<RefCounterable> Type::CreateSampleRef() const
    {
        return nullptr;
    }

    void* Type::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        for (auto& field : mFields)
        {
            if (field.mName == pathPart)
            {
                fieldInfo = &field;

                if (delPos == -1)
                {
                    return field.GetValuePtrStrong(object);
                }
                else
                {
                    void* val = field.GetValuePtr(object);

                    if (!val)
                        return nullptr;

                    return field.SearchFieldPtr(val, path.SubStr(delPos + 1), fieldInfo);
                }
            }
        }

        for (auto baseType : mBaseTypes)
        {
            if (auto res = baseType.type->Type::GetFieldPtr((*baseType.dynamicCastUpFunc)(object), path, fieldInfo))
                return res;
        }

        return nullptr;
    }

    void Type::Serialize(void* ptr, DataValue& data) const
    {
        mSerializer->Serialize(ptr, data);
    }

    void Type::Deserialize(void* ptr, const DataValue& data) const
    {
        mSerializer->Deserialize(ptr, data);
    }

    bool Type::IsValueEquals(void* objectA, void* objectB) const
    {
        return mSerializer->Equals(objectA, objectB);
    }

    void Type::CopyValue(void* objectA, void* objectB) const
    {
        mSerializer->Copy(objectA, objectB);
    }

    ITypeSerializer* Type::GetSerializer() const
    {
        return mSerializer;
    }

    VectorType::VectorType(const String& name, int size, ITypeSerializer* serializer) :
        Type(name, size, serializer)
    {}

    Type::Usage VectorType::GetUsage() const
    {
        return Usage::Vector;
    }

    const Type* VectorType::GetElementType() const
    {
        return mElementType;
    }

    void* VectorType::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        if (pathPart == "count")
        {
            fieldInfo = mCountFieldInfo;
            return object;
        }

        if (pathPart.IsEmpty() || pathPart[0] < '0' || pathPart[0] > '9')
            return nullptr;

        int count = GetObjectVectorSize(object);
        int idx = (int)pathPart;

        if (idx >= count || idx < 0)
            return nullptr;

        if (delPos < 0)
        {
            fieldInfo = mElementFieldInfo;
            return GetObjectVectorElementPtr(object, idx);
        }

        return mElementType->GetFieldPtr(GetObjectVectorElementPtr(object, idx), path.SubStr(delPos + 1), fieldInfo);
    }

    FieldInfo* VectorType::GetElementFieldInfo() const
    {
        return mElementFieldInfo;
    }

    FieldInfo* VectorType::GetCountFieldInfo() const
    {
        return mCountFieldInfo;
    }

    MapType::MapType(const Type* keyType, const Type* valueType, int size, ITypeSerializer* serializer):
        Type((String)"o2::Dictionary<" + keyType->GetName() + ", " + valueType->GetName() + ">", size, serializer),
        mKeyType(keyType), mValueType(valueType)
    {}

    Type::Usage MapType::GetUsage() const
    {
        return Usage::Map;
    }

    const Type* MapType::GetKeyType() const
    {
        return mKeyType;
    }

    const Type* MapType::GetValueType() const
    {
        return mValueType;
    }

    int MapType::GetObjectDictionarySize(void* object) const
    {
        return mGetDictionaryObjectSizeFunc(object);
    }

    void* MapType::GetObjectDictionaryKeyPtr(void* object, int idx) const
    {
        return mGetObjectDictionaryKeyPtrFunc(object, idx);
    }

    void* MapType::GetObjectDictionaryValuePtr(void* object, int idx) const
    {
        return mGetObjectDictionaryValuePtrFunc(object, idx);
    }

    void* MapType::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
//         int delPos = path.Find("/");
//         String pathPart = path.SubStr(0, delPos);
//         DataValue data;
//         data = pathPart;
// 
//         int count = GetObjectDictionarySize(object);
//         int idx = (int)pathPart;
// 
//         if (idx < count)
//             return mElementType->GetFieldPtr(GetObjectVectorElementPtr(object, idx), path.SubStr(delPos + 1), fieldInfo);

        return nullptr;
    }

    EnumType::EnumType(const String& name, int size, ITypeSerializer* serializer):
        Type(name, size, serializer)
    {}

    Type::Usage EnumType::GetUsage() const
    {
        return Usage::Enumeration;
    }

    const Map<int, String>& EnumType::GetEntries() const
    {
        return mEntries;
    }

    PointerType::PointerType(const Type* baseType, ITypeSerializer* serializer):
        Type(baseType->GetName() + "*", sizeof(void*), serializer),
        mBaseType(baseType)
    {}

    Type::Usage PointerType::GetUsage() const
    {
        return Usage::Pointer;
    }

    const Type* PointerType::GetBaseType() const
    {
        return mBaseType;
    }

    void* PointerType::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        return mBaseType->GetFieldPtr(*(void**)object, path, fieldInfo);
    }

    void* PointerType::CreateSample() const
    {
        return mBaseType->CreateSample();
    }

    Ref<IAbstractValueProxy> PointerType::GetValueProxy(void* object) const
    {
        if (auto objectUnptrType = dynamic_cast<const ObjectType*>(mBaseType))
            return objectUnptrType->GetValueProxy(*(void**)object);

        return mmake<PointerValueProxy<void*>>((void**)object);
    }

    ReferenceType::ReferenceType(const Type* baseType, ITypeSerializer* serializer) :
        Type("Ref<" + baseType->GetName() + ">", sizeof(Ref<RefCounterable>), serializer),
        mBaseType(baseType)
    {}

    Type::Usage ReferenceType::GetUsage() const
    {
        return Usage::Reference;
    }

    const Type* ReferenceType::GetBaseType() const
    {
        return mBaseType;
    }

    void* ReferenceType::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        return mBaseType->GetFieldPtr(reinterpret_cast<Ref<RefCounterable>*>(object)->Get(), path, fieldInfo);
    }

    void* ReferenceType::CreateSample() const
    {
        return mBaseType->CreateSample();
    }

    Ref<IAbstractValueProxy> ReferenceType::GetValueProxy(void* object) const
    {
        return mmake<PointerValueProxy<void*>>((void**)object);
    }

    PropertyType::PropertyType(const String& name, int size, ITypeSerializer* serializer) :
        Type(name, size, serializer)
    {}

    Type::Usage PropertyType::GetUsage() const
    {
        return Usage::Property;
    }

    const Type* PropertyType::GetValueType() const
    {
        return mValueType;
    }

    ObjectType::ObjectType(const String& name, int size, void*(*castFromFunc)(void*), void*(*castToFunc)(void*),
                           ITypeSerializer* serializer):
        Type(name, size, serializer), mCastToFunc(castToFunc), mCastFromFunc(castFromFunc)
    {}

    Type::Usage ObjectType::GetUsage() const
    {
        return Usage::Object;
    }

    void* ObjectType::DynamicCastFromIObject(IObject* object) const
    {
        return (*mCastFromFunc)(object);
    }

    IObject* ObjectType::DynamicCastToIObject(void* object) const
    {
        return (IObject*)(*mCastToFunc)(object);
    }

    void* ObjectType::GetFieldPtr(void* object, const String& path, const FieldInfo*& fieldInfo) const
    {
        if (!object)
            return nullptr;

        IObject* iobject = DynamicCastToIObject(object);
        if (iobject)
        {
            const Type* realType = &(iobject->GetType());
            if (realType != this)
                return realType->GetFieldPtr(dynamic_cast<const ObjectType*>(realType)->DynamicCastFromIObject(iobject), path, fieldInfo);
        }

        return Type::GetFieldPtr(object, path, fieldInfo);
    }

    StringPointerAccessorType::StringPointerAccessorType(const String& name, int size, ITypeSerializer* serializer) :
        Type(name, size, serializer)
    {}

    Type::Usage StringPointerAccessorType::GetUsage() const
    {
        return Usage::StringAccessor;
    }

    FunctionType::FunctionType(const String& name):
        Type(name, sizeof(Function<void()>), mnew TypeSerializer<Function<void()>>())
    {}

    Type::Usage FunctionType::GetUsage() const
    {
        return Type::Usage::Function;
    }

    void* FunctionType::CreateSample() const
    {
        return nullptr;
    }

    void FunctionType::DestroySample(void* sample) const
    {}

    Ref<IAbstractValueProxy> FunctionType::GetValueProxy(void* object) const
    {
        static int offs = (std::ptrdiff_t)((AbstractFunction*)((Function<void()>*)1)) - (std::ptrdiff_t)(Function<void()>*)1;
        auto btpr = reinterpret_cast<std::byte*>(object);
        auto ptr = btpr + offs;

        return mmake<PointerValueProxy<AbstractFunction>>(reinterpret_cast<AbstractFunction*>(ptr));
    }
}

ENUM_META(o2::Type::Usage, o2__Type__Usage)
{
    ENUM_ENTRY(Map);
    ENUM_ENTRY(Enumeration);
    ENUM_ENTRY(Pointer);
    ENUM_ENTRY(Property);
    ENUM_ENTRY(Regular);
    ENUM_ENTRY(StringAccessor);
    ENUM_ENTRY(Vector);
}
END_ENUM_META;
