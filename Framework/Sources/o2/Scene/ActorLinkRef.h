#pragma once

#include "o2/Scene/ActorCreationMode.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/LinkRef.h"

namespace o2
{
    class Actor;

    // --------------------------------------------------------------
    // Actor reference, automatically invalidates when actor deleting
    // --------------------------------------------------------------
    class BaseActorLinkRef : public ISerializable
    {
    public:
        // Default constructor, no reference
        BaseActorLinkRef();

        // Creates a copy of actor and returns reference on it
        BaseActorLinkRef(const BaseActorLinkRef& other);

        // Destructor
        ~BaseActorLinkRef();

        // Assign operator
        BaseActorLinkRef& operator=(const BaseActorLinkRef& other);

        // Returns actor pointer @SCRIPTABLE
        virtual Actor* Get();

        // Returns actor pointer
        virtual const Actor* Get() const;

        // Sets actor pointer
        virtual void Set(Actor* actor);

        // Destroy the actor @SCRIPTABLE
        void Destroy();

        // Returns is reference is valid @SCRIPTABLE
        bool IsValid() const;

        // Returns actor type
        virtual const Type& GetActorType() const;

        // Copying ref without requiring remap
        virtual void CopyWithoutRemap(const BaseActorLinkRef& other);

        // Returns actor type
        static const Type* GetActorTypeStatic();

        // Checks refs are equals for serializing delta
        static bool EqualsDelta(const BaseActorLinkRef& obj, const BaseActorLinkRef& origin);

        SERIALIZABLE(BaseActorLinkRef);

    protected:
        // ------------------------------
        // Reference resolve request data
        // ------------------------------
        struct IRequiredResolveData : public RefCounterable
        {
            // Request resolve reference
            virtual void RequireResolve(BaseActorLinkRef& ref) = 0;

            // Clone data
            virtual Ref<IRequiredResolveData> Clone() const = 0;
        };

        // -------------------------------------------------
        // Reference resolve request data by actor scene uid
        // -------------------------------------------------
        struct SceneRequireResolveData : public IRequiredResolveData
        {
            SceneUID uid;

            // Request resolve reference
            void RequireResolve(BaseActorLinkRef& ref) override;

            // Clone data
            Ref<IRequiredResolveData> Clone() const override;
        };

        // -------------------------------------------
        // Reference resolve request data by asset uid
        // -------------------------------------------
        struct AssetRequireResolveData : public IRequiredResolveData
        {
            UID uid;

            // Request resolve reference
            void RequireResolve(BaseActorLinkRef& ref) override;

            // Clone data
            Ref<IRequiredResolveData> Clone() const override;
        };

        Ref<IRequiredResolveData> mRequiredResolveData; // Reference resolve request data. Used for resolving reference after deserialization.
        // Not null only when reference is required to resolve. Copies in reference copying.

    protected:
        // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        friend class Actor;
        friend class ActorRefResolver;
    };

#define ENABLE_ACTOR typename std::enable_if<IsBaseOf<Actor, _actor_type>::value>::type

    // ---------------------------------------
    // Reference on derived from actor classes
    // Implicitly converts to Ref<Actor> and back
    // ---------------------------------------
    template<typename _actor_type>
    class LinkRef<_actor_type, ENABLE_ACTOR> : public BaseActorLinkRef
    {
    public:
        // Default constructor, no reference
        LinkRef();

        // Nullptr constructor
        LinkRef(std::nullptr_t);

        // Constructor with actor pointer
        explicit LinkRef(_actor_type* ptr);

        // Copy constructor from other reference
        LinkRef(const Ref<_actor_type>& other);

        // Move constructor from other reference
        LinkRef(Ref<_actor_type>&& other);

        // Copy constructor from other actor reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _actor_type*>::value>::type>
        LinkRef(const LinkRef<_other_type>& other);

        // Move constructor from other actor reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _actor_type*>::value>::type>
        LinkRef(LinkRef<_other_type>&& other);

        // Equality operator
        bool operator==(const LinkRef<_actor_type>& other) const;

        // Equality operator
        bool operator==(const _actor_type* other) const;

        // Inequality operator
        bool operator!=(const LinkRef<_actor_type>& other) const;

        // Inequality operator
        bool operator!=(const _actor_type* other) const;

        // Copy operator from other actor reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _actor_type*>::value>::type>
        LinkRef<_actor_type>& operator=(const LinkRef<_other_type>& other);

        // Move operator from other actor reference
        LinkRef<_actor_type>& operator=(Ref<_actor_type>&& other);

        // Move operator from nullptr
        LinkRef<_actor_type>& operator=(std::nullptr_t);

        // Less operator
        bool operator<(const LinkRef<_actor_type>& other) const;

        // Returns is reference is valid
        bool IsValid() const;

        // Returns is reference is valid
        explicit operator bool() const;

        // Returns actor reference
        operator Ref<_actor_type>() const;

        // Returns actor reference
        _actor_type& operator*() const;

        // Returns actor pointer
        _actor_type* operator->() const;

        // Returns actor pointer 
        _actor_type* Get() override;

        // Returns actor pointer
        const _actor_type* Get() const override;

        // Returns reference
        Ref<_actor_type>& GetRef();

        // Returns reference
        const Ref<_actor_type>& GetRef() const;

        // Sets actor pointer
        void Set(Actor* actor) override;

        // Returns actor type
        const Type& GetActorType() const override;

        // Copying ref without requiring remap
        void CopyWithoutRemap(const BaseActorLinkRef& other) override;

        // Returns actor type
        static const Type* GetActorTypeStatic();

    protected:
        Ref<_actor_type> mRef; // Reference to actor

    public:
        typedef LinkRef<_actor_type> _thisType;

        SERIALIZABLE_MAIN(_thisType);
        IOBJECT_SCRIPTING(LinkRef);

        template<typename _type_processor>
        static void ProcessBaseTypes(_thisType* object, _type_processor& processor)
        {
            typedef _thisType thisclass;
            processor.template StartBases<_thisType>(object, type);

            BASE_CLASS(o2::BaseActorLinkRef);
        }

        template<typename _type_processor>
        static void ProcessFields(_thisType* object, _type_processor& processor)
        {
            typedef _thisType thisclass;
            processor.template StartFields<_thisType>(object, type);

            FIELD().PROTECTED().NAME(mRef);
        }

        template<typename _type_processor>
        static void ProcessMethods(_thisType* object, _type_processor& processor)
        {
            typedef _thisType thisclass;
            processor.template StartMethods<_thisType>(object, type);

            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
            FUNCTION().PUBLIC().SIGNATURE(const Type&, GetActorType);
            FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetActorTypeStatic);
        }
    };

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef()
    {}

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(std::nullptr_t) :
        mRef(nullptr)
    {}

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(_actor_type* ptr) :
        mRef(ptr)
    {}

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(const Ref<_actor_type>&other) :
        mRef(other)
    {}

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(Ref<_actor_type> && other) :
        mRef(std::move(other))
    {}

    template<typename _actor_type>
    template<typename _other_type, typename _enable>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(const LinkRef<_other_type>& other) :
        mRef(other.mRef)
    {}

    template<typename _actor_type>
    template<typename _other_type, typename>
    LinkRef<_actor_type, ENABLE_ACTOR>::LinkRef(LinkRef<_other_type>&& other) :
        mRef(std::move(other.mRef))
    {}

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::operator==(const LinkRef<_actor_type>& other) const
    {
        return mRef == other.mRef;
    }

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::operator==(const _actor_type* other) const
    {
        return mRef == other;
    }

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::operator!=(const LinkRef<_actor_type>& other) const
    {
        return mRef != other.mRef;
    }

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::operator!=(const _actor_type* other) const
    {
        return mRef != other;
    }

    template<typename _actor_type>
    template<typename _other_type, typename _enable>
    LinkRef<_actor_type>& LinkRef<_actor_type, ENABLE_ACTOR>::operator=(const LinkRef<_other_type>& other)
    {
        mRef = other.mRef;
        return *this;
    }

    template<typename _actor_type>
    LinkRef<_actor_type>& LinkRef<_actor_type, ENABLE_ACTOR>::operator=(Ref<_actor_type>&& other)
    {
        mRef = std::move(other);
        return *this;
    }

    template<typename _actor_type>
    LinkRef<_actor_type>& LinkRef<_actor_type, ENABLE_ACTOR>::operator=(std::nullptr_t)
    {
        mRef = nullptr;
        return *this;
    }

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::operator<(const LinkRef<_actor_type>& other) const
    {
        return mRef < other.mRef;
    }

    template<typename _actor_type>
    bool LinkRef<_actor_type, ENABLE_ACTOR>::IsValid() const
    {
        return mRef.IsValid();
    }

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::operator bool() const
    {
        return IsValid();
    }

    template<typename _actor_type>
    LinkRef<_actor_type, ENABLE_ACTOR>::operator Ref<_actor_type>() const
    {
        return mRef;
    }

    template<typename _actor_type>
    _actor_type& LinkRef<_actor_type, ENABLE_ACTOR>::operator*() const
    {
        return *mRef;
    }

    template<typename _actor_type>
    _actor_type* LinkRef<_actor_type, ENABLE_ACTOR>::operator->() const
    {
        return mRef.Get();
    }

    template<typename _actor_type>
    _actor_type* LinkRef<_actor_type, ENABLE_ACTOR>::Get()
    {
        return mRef.Get();
    }

    template<typename _actor_type>
    const _actor_type* LinkRef<_actor_type, ENABLE_ACTOR>::Get() const
    {
        return mRef.Get();
    }

    template<typename _actor_type>
    Ref<_actor_type>& LinkRef<_actor_type, ENABLE_ACTOR>::GetRef()
    {
        return mRef;
    }

    template<typename _actor_type>
    const Ref<_actor_type>& LinkRef<_actor_type, ENABLE_ACTOR>::GetRef() const
    {
        return mRef;
    }

    template<typename _actor_type>
    void LinkRef<_actor_type, ENABLE_ACTOR>::Set(Actor* actor)
    {
        CopyWithoutRemap(LinkRef(dynamic_cast<_actor_type*>(actor)));
    }

    template<typename _actor_type>
    const Type& LinkRef<_actor_type, ENABLE_ACTOR>::GetActorType() const
    {
        return TypeOf(_actor_type);
    }

    template<typename _actor_type>
    void LinkRef<_actor_type, ENABLE_ACTOR>::CopyWithoutRemap(const BaseActorLinkRef& other)
    {
        mRef = Ref(dynamic_cast<_actor_type*>(const_cast<Actor*>(other.Get())));
        mRequiredResolveData = nullptr;
    }

    template<typename _actor_type>
    const Type* LinkRef<_actor_type, ENABLE_ACTOR>::GetActorTypeStatic()
    {
        return &TypeOf(_actor_type);
    }
}
// --- META ---

CLASS_BASES_META(o2::BaseActorLinkRef)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::BaseActorLinkRef)
{
    FIELD().PROTECTED().NAME(mRequiredResolveData);
}
END_META;
CLASS_METHODS_META(o2::BaseActorLinkRef)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const BaseActorLinkRef&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Actor*, Get);
    FUNCTION().PUBLIC().SIGNATURE(const Actor*, Get);
    FUNCTION().PUBLIC().SIGNATURE(void, Set, Actor*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Destroy);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsValid);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetActorType);
    FUNCTION().PUBLIC().SIGNATURE(void, CopyWithoutRemap, const BaseActorLinkRef&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetActorTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, EqualsDelta, const BaseActorLinkRef&, const BaseActorLinkRef&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
