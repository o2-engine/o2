#include "o2/stdafx.h"
#include "Reflection.h"

#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/UID.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptEngine.h"

namespace o2
{
    void AddReflectionScriptConstructorFunc(ReflectionScriptConstructorFunc func)
    {
        ScriptEngine::GetRegisterConstructorFuncs().Add((ScriptEngine::RegisterConstructorFunc)func);
    }
}
#endif

extern void InitializeTypeso2Framework();

namespace o2
{
    Reflection* Reflection::mInstance;

    template<>
    Type* FundamentalTypeContainer<void>::type = mnew TType<int>("void", 0);

    Type* IObject::type = mnew TType<IObject>("IObject", 0);
    Type* Type::Dummy::type = mnew TType<int>("Unknown", 0);

    FunctionType* FunctionType::commonType = Reflection::InitializeFunctionType("Function");
    FunctionType* FunctionType::serializableType = Reflection::InitializeFunctionType("SerializableFunction");

    DECLARE_FUNDAMENTAL_TYPE(int);
    DECLARE_FUNDAMENTAL_TYPE(bool);
    DECLARE_FUNDAMENTAL_TYPE(char);
    DECLARE_FUNDAMENTAL_TYPE(wchar_t);
    DECLARE_FUNDAMENTAL_TYPE(short int);
    DECLARE_FUNDAMENTAL_TYPE(long int);
    DECLARE_FUNDAMENTAL_TYPE(long long int);
    DECLARE_FUNDAMENTAL_TYPE(unsigned char);
    DECLARE_FUNDAMENTAL_TYPE(unsigned short int);
    DECLARE_FUNDAMENTAL_TYPE(unsigned int);
    DECLARE_FUNDAMENTAL_TYPE(unsigned long int);
    DECLARE_FUNDAMENTAL_TYPE(unsigned long long int);
    DECLARE_FUNDAMENTAL_TYPE(float);
    DECLARE_FUNDAMENTAL_TYPE(double);
    DECLARE_FUNDAMENTAL_TYPE(long double);
    DECLARE_FUNDAMENTAL_TYPE(o2::AABB);
    DECLARE_FUNDAMENTAL_TYPE(o2::Basis);
    DECLARE_FUNDAMENTAL_TYPE(o2::Basis3D);
    DECLARE_FUNDAMENTAL_TYPE(o2::Color4);
    DECLARE_FUNDAMENTAL_TYPE(o2::RectF);
    DECLARE_FUNDAMENTAL_TYPE(o2::RectI);
    DECLARE_FUNDAMENTAL_TYPE(o2::BorderF);
    DECLARE_FUNDAMENTAL_TYPE(o2::BorderI);
    DECLARE_FUNDAMENTAL_TYPE(o2::Vec2F);
    DECLARE_FUNDAMENTAL_TYPE(o2::Vec2I);
    DECLARE_FUNDAMENTAL_TYPE(o2::Vec3F);
    DECLARE_FUNDAMENTAL_TYPE(o2::Vec3I);
    DECLARE_FUNDAMENTAL_TYPE(o2::Quat);
    DECLARE_FUNDAMENTAL_TYPE(o2::Mat4);
    DECLARE_FUNDAMENTAL_TYPE(o2::Vertex);
    DECLARE_FUNDAMENTAL_TYPE(o2::String);
    DECLARE_FUNDAMENTAL_TYPE(o2::WString);
    DECLARE_FUNDAMENTAL_TYPE(o2::DataValue);
    DECLARE_FUNDAMENTAL_TYPE(o2::UID);
    DECLARE_FUNDAMENTAL_TYPE(o2::ScriptValue);

    Reflection::Reflection():
        mLastGivenTypeId(1)
    {
        mInstance = this;
    }

    Reflection::~Reflection()
    {
        for (auto kv : mTypes)
            delete kv.second;
    }

    Reflection& Reflection::Instance()
    {
        if (!mInstance)
            mInstance = new Reflection();

        return *mInstance;
    }

    void Reflection::InitializeTypes()
    {
        InitializeTypeso2Framework();
        InitializeFundamentalTypes();

        ReflectionInitializationTypeProcessor processor;
        for (auto func : mInstance->mInitializingFunctions)
            func(0, processor);

        mInstance->mInitializingFunctions.Clear();
        mInstance->mTypesInitialized = true;
    }

    const Map<String, Type*>& Reflection::GetTypes()
    {
        return mInstance->mTypes;
    }

    void* Reflection::CreateTypeSample(const String& typeName)
    {
        if (auto type = GetType(typeName))
            return type->CreateSample();

        return nullptr;
    }

    Ref<RefCounterable> Reflection::CreateTypeSampleRef(const String& typeName)
    {
        if (auto type = GetType(typeName))
            return type->CreateSampleRef();

        return nullptr;
    }

    const Type* Reflection::GetType(const String& name)
    {
        auto fnd = mInstance->mTypes.find(name);
        if (fnd != mInstance->mTypes.End())
            return fnd->second;

        return nullptr;
    }

    bool Reflection::IsTypesInitialized()
    {
        return mInstance->mTypesInitialized;
    }

    FunctionType* Reflection::InitializeFunctionType(const char* name)
    {
        FunctionType* res = mnew FunctionType(name);

        res->mId = Reflection::Instance().mLastGivenTypeId++;
        mInstance->mTypes[res->GetName()] = res;

        return res;
    }

    void Reflection::InitializeFundamentalTypes()
    {
        IObject::type->mId = mInstance->mLastGivenTypeId++;
        FundamentalTypeContainer<void>::type->mId = mInstance->mLastGivenTypeId++;
        Type::Dummy::type->mId = mInstance->mLastGivenTypeId++;

        mInstance->mTypes[FundamentalTypeContainer<void>::type->GetName()] = FundamentalTypeContainer<void>::type;
    }

    ReflectionInitializationTypeProcessor::FieldProcessor ReflectionInitializationTypeProcessor::StartField()
    {
        return FieldProcessor();
    }

    ReflectionInitializationTypeProcessor::FunctionProcessor ReflectionInitializationTypeProcessor::StartFunction()
    {
        return FunctionProcessor();
    }

    ReflectionInitializationTypeProcessor::FieldProcessor& ReflectionInitializationTypeProcessor::FieldProcessor::SetProtectSection(ProtectSection section)
    {
        this->section = section;
        return *this;
    }

    ReflectionInitializationTypeProcessor::FunctionProcessor& ReflectionInitializationTypeProcessor::FunctionProcessor::SetProtectSection(ProtectSection section)
    {
        this->section = section;
        return *this;
    }

}
