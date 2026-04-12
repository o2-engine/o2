#pragma once
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Function/ISerializableFunction.h"
#include "o2/Utils/Serialization/Serializable.h"

#if IS_SCRIPTING_SUPPORTED
#include <o2/Scene/Components/ScriptableComponent.h>
#endif

namespace o2
{
    class IActorSubscription : public ISerializableFunction
    {
    public:
        LinkRef<Actor>     actorRef;     // Target actor
        LinkRef<Component> componentRef; // Target component
        String             method;       // Method name
    };

    template <typename UnusedType>
    class ActorSubscription;

    template<typename _res_type, typename ... _args>
    class ActorSubscription<_res_type(_args ...)> : public IFunction<_res_type(_args ...)>, public IActorSubscription
    {
    public:
        // Constructor
        ActorSubscription()
        {}

        // Constructor
        ActorSubscription(const LinkRef<Actor>& actor, const String& method)
        {
            this->actorRef = actor;
            this->method = method;
        }

        // Copy-constructor
        ActorSubscription(const ActorSubscription& other)
        {
            actorRef = other.actorRef;
            method = other.method;
        }

        // Copy-operator
        ActorSubscription& operator=(const ActorSubscription& other)
        {
            actorRef = other.actorRef;
            method = other.method;

            return *this;
        }

        // Equal operator
        bool operator==(const ActorSubscription& other) const
        {
            return actorRef == other.actorRef && method == other.method;
        }

        // Not equal operator
        bool operator!=(const ActorSubscription& other) const
        {
            return actorRef != other.actorRef || method != other.method;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew ActorSubscription(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) ActorSubscription(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const override
        {
            const IObject* callSource = nullptr;
            if (auto component = componentRef.Get())
                callSource = component;
            else if (auto actor = actorRef.Get())
                callSource = static_cast<const IObject*>(static_cast<const ActorBase*>(actor));

            auto& objType = dynamic_cast<const ObjectType&>(callSource->GetType());
             if constexpr (IS_SCRIPTING_SUPPORTED)
             {
                 if (auto scriptableComponent = dynamic_cast<const ScriptableComponent*>(callSource))
                 {
                     ScriptValue scriptMethod = scriptableComponent->GetInstance().GetProperty(method);
                     if (scriptMethod.IsFunction())
                         return scriptMethod.Invoke<_res_type, _args ...>(scriptableComponent->GetInstance(), args ...);
                 }
             }

            void* obj = objType.DynamicCastFromIObject(const_cast<IObject*>(dynamic_cast<const IObject*>(callSource)));
            return objType.Invoke<_res_type, _args ...>(method, obj, args ...);
        }

        // Returns true if functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const override
        {
            const ActorSubscription* otherFuncPtr = dynamic_cast<const ActorSubscription*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return false;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }

        // Serializes a function
        void Serialize(DataValue& data) const override
        {
            data["type"] = "ActorFunction";
            if (componentRef)
                data["component"] = componentRef;
            else
                data["actor"] = actorRef;

            data["func"] = method;
        }

        // Deserializes a function
        void Deserialize(const DataValue& data) override
        {
            if (auto actorData = data.FindMember("actor"))
                actorData->Get(actorRef);

            if (auto componentData = data.FindMember("component"))
                componentData->Get(componentRef);

            method = data["func"];
        }
    };

    template<typename _res_type, typename ... _args>
    void SerializableFunction<_res_type(_args ...)>::AddActorSubscription()
    {
        Base::Add(mnew ActorSubscription<_res_type(_args...)>());
    }
}

#include "o2/Utils/Function/FunctionDataValueConverter.h"
