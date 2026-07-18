#pragma once

#include "o2/Scene/Component.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/LinkRef.h"

namespace o2
{
    // ---------------------------------------------------------------------
    // Component reference, automatically invalidates when component deletes
    // ---------------------------------------------------------------------
    class BaseComponentLinkRef: public ISerializable
    {
    public:
        // Default constructor, no reference
        BaseComponentLinkRef();

        // Creates a copy of component and returns reference on it
        BaseComponentLinkRef(const BaseComponentLinkRef& other);

        // Destructor
        ~BaseComponentLinkRef();

        // Sets component @SCRIPTABLE
        virtual void Set(Component* component);

        // Returns component pointer @SCRIPTABLE
        virtual Component* Get();

        // Returns component pointer
        virtual const Component* Get() const;

        // Destroy the component @SCRIPTABLE
        void Destroy();

        // Returns component type
        virtual const Type& GetComponentType() const;

        // Copying ref without requiring remap
        virtual void CopyWithoutRemap(const BaseComponentLinkRef& other);

        // Returns component type
        static const Type* GetComponentTypeStatic();

        // Checks refs are equals for serializing delta
        static bool EqualsDelta(const BaseComponentLinkRef& obj, const BaseComponentLinkRef& origin);

        SERIALIZABLE(BaseComponentLinkRef);

    protected:
        // ------------------------------
        // Reference resolve request data
        // ------------------------------
        struct IRequiredResolveData: public RefCounterable
        {
            SceneUID componentUID;

            // Request resolve reference
            virtual void RequireResolve(BaseComponentLinkRef& ref) = 0;

            // Clone data
            virtual Ref<IRequiredResolveData> Clone() const = 0;
        };

        // -------------------------------------------------
        // Reference resolve request data by component scene uid
        // -------------------------------------------------
        struct SceneRequireResolveData : public IRequiredResolveData
        {
            SceneUID uid;

            // Request resolve reference
            void RequireResolve(BaseComponentLinkRef& ref) override;

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
            void RequireResolve(BaseComponentLinkRef& ref) override;

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

        friend class Component;
        friend class ComponentRefResolver;
    };

#define ENABLE_COMPONENT typename std::enable_if<IsBaseOf<Component, _component_type>::value>::type

    // ---------------------------------------
    // Reference on derived from component classes
    // ---------------------------------------
    template<typename _component_type>
    class LinkRef<_component_type, ENABLE_COMPONENT> : public BaseComponentLinkRef
    {
    public:
        // Default constructor, no reference
        LinkRef();

        // Nullptr constructor
        LinkRef(std::nullptr_t);

        // Constructor with component pointer
        explicit LinkRef(_component_type* ptr);

        // Copy constructor from other reference
        LinkRef(const Ref<_component_type>& other);

        // Move constructor from other reference
        LinkRef(Ref<_component_type>&& other);

        // Copy constructor from other component reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _component_type*>::value>::type>
        LinkRef(const LinkRef<_other_type>& other);

        // Move constructor from other component reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _component_type*>::value>::type>
        LinkRef(LinkRef<_other_type>&& other);

        // Equality operator
        bool operator==(const LinkRef<_component_type>& other) const;

        // Equality operator
        bool operator==(const _component_type* other) const;

        // Inequality operator
        bool operator!=(const LinkRef<_component_type>& other) const;

        // Inequality operator
        bool operator!=(const _component_type* other) const;

        // Copy operator from other component reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _component_type*>::value>::type>
        LinkRef<_component_type>& operator=(const LinkRef<_other_type>& other);

        // Move operator from other component reference
        LinkRef<_component_type>& operator=(Ref<_component_type>&& other);

        // Move operator from nullptr
        LinkRef<_component_type>& operator=(std::nullptr_t);

        // Less operator
        bool operator<(const LinkRef<_component_type>& other) const;

        // Returns is reference is valid
        bool IsValid() const;

        // Returns is reference is valid
        explicit operator bool() const;

        // Returns component reference
        operator Ref<_component_type>() const;

        // Returns component reference
        _component_type& operator*() const;

        // Returns component pointer
        _component_type* operator->() const;

        // Returns component pointer 
        _component_type* Get() override;

        // Returns component pointer
        const _component_type* Get() const override;

        // Returns reference
        Ref<_component_type>& GetRef();

        // Returns reference
        const Ref<_component_type>& GetRef() const;

        // Sets component pointer
        void Set(Component* component) override;

        // Returns component type
        const Type& GetComponentType() const override;

        // Copying ref without requiring remap
        void CopyWithoutRemap(const BaseComponentLinkRef& other) override;

        // Returns component type
        static const Type* GetComponentTypeStatic();

    protected:
        Ref<_component_type> mRef; // Reference to component

    public:
        typedef LinkRef<_component_type> _thisType;

        SERIALIZABLE_MAIN(_thisType);
        IOBJECT_SCRIPTING(LinkRef);

        template<typename _type_processor>
        static void ProcessBaseTypes(_thisType* object, _type_processor& processor)
        {
            typedef _thisType thisclass;
            processor.template StartBases<_thisType>(object, type);

            BASE_CLASS(o2::BaseComponentLinkRef);
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

            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(_component_type*);
            FUNCTION().PUBLIC().SIGNATURE(const Type&, GetComponentType);
            FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetComponentTypeStatic);
        }
    };

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef()
    {}

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(std::nullptr_t) :
        mRef(nullptr)
    {}

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(_component_type* ptr) :
        mRef(ptr)
    {}

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(const Ref<_component_type>& ref) :
        mRef(ref)
    {}

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(Ref<_component_type> && other):
        mRef(std::move(other))
    {}

    template<typename _component_type>
    template<typename _other_type, typename _enable>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(const LinkRef<_other_type>& other) :
        mRef(other.mRef)
    {}

    template<typename _component_type>
    template<typename _other_type, typename>
    LinkRef<_component_type, ENABLE_COMPONENT>::LinkRef(LinkRef<_other_type>&& other) :
        mRef(std::move(other.mRef))
    {}

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::operator==(const LinkRef<_component_type>& other) const
    {
        return mRef == other.mRef;
    }

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::operator==(const _component_type* other) const
    {
        return mRef == other;
    }

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::operator!=(const LinkRef<_component_type>& other) const
    {
        return mRef != other.mRef;
    }

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::operator!=(const _component_type* other) const
    {
        return mRef != other;
    }

    template<typename _component_type>
    template<typename _other_type, typename _enable>
    LinkRef<_component_type>& LinkRef<_component_type, ENABLE_COMPONENT>::operator=(const LinkRef<_other_type>& other)
    {
        mRef = other.mRef;
        return *this;
    }

    template<typename _component_type>
    LinkRef<_component_type>& LinkRef<_component_type, ENABLE_COMPONENT>::operator=(Ref<_component_type>&& other)
    {
        mRef = std::move(other);
        return *this;
    }

    template<typename _component_type>
    LinkRef<_component_type>& LinkRef<_component_type, ENABLE_COMPONENT>::operator=(std::nullptr_t)
    {
        mRef = nullptr;
        return *this;
    }

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::operator<(const LinkRef<_component_type>& other) const
    {
        return mRef < other.mRef;
    }

    template<typename _component_type>
    bool LinkRef<_component_type, ENABLE_COMPONENT>::IsValid() const
    {
        return mRef.IsValid();
    }

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::operator bool() const
    {
        return IsValid();
    }

    template<typename _component_type>
    LinkRef<_component_type, ENABLE_COMPONENT>::operator Ref<_component_type>() const
    {
        return mRef;
    }

    template<typename _component_type>
    _component_type& LinkRef<_component_type, ENABLE_COMPONENT>::operator*() const
    {
        return *mRef;
    }

    template<typename _component_type>
    _component_type* LinkRef<_component_type, ENABLE_COMPONENT>::operator->() const
    {
        return mRef.Get();
    }

    template<typename _component_type>
    _component_type* LinkRef<_component_type, ENABLE_COMPONENT>::Get()
    {
        return mRef.Get();
    }

    template<typename _component_type>
    const _component_type* LinkRef<_component_type, ENABLE_COMPONENT>::Get() const
    {
        return mRef.Get();
    }

    template<typename _component_type>
    Ref<_component_type>& LinkRef<_component_type, ENABLE_COMPONENT>::GetRef()
    {
        return mRef;
    }

    template<typename _component_type>
    const Ref<_component_type>& LinkRef<_component_type, ENABLE_COMPONENT>::GetRef() const
    {
        return mRef;
    }

    template<typename _component_type>
    void LinkRef<_component_type, ENABLE_COMPONENT>::Set(Component* component)
    {
        *this = LinkRef(dynamic_cast<_component_type*>(component));
    }

    template<typename _component_type>
    const Type& LinkRef<_component_type, ENABLE_COMPONENT>::GetComponentType() const
    {
        return TypeOf(_component_type);
    }

    template<typename _component_type>
    void LinkRef<_component_type, ENABLE_COMPONENT>::CopyWithoutRemap(const BaseComponentLinkRef& other)
    {
        mRef = Ref(dynamic_cast<_component_type*>(const_cast<Component*>(other.Get())));
        mRequiredResolveData = nullptr;
    }

    template<typename _component_type>
    const Type* LinkRef<_component_type, ENABLE_COMPONENT>::GetComponentTypeStatic()
    {
        return &TypeOf(_component_type);
    }
}
// --- META ---

CLASS_BASES_META(o2::BaseComponentLinkRef)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::BaseComponentLinkRef)
{
    FIELD().PROTECTED().NAME(mRequiredResolveData);
}
END_META;
CLASS_METHODS_META(o2::BaseComponentLinkRef)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const BaseComponentLinkRef&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Set, Component*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Component*, Get);
    FUNCTION().PUBLIC().SIGNATURE(const Component*, Get);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Destroy);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetComponentType);
    FUNCTION().PUBLIC().SIGNATURE(void, CopyWithoutRemap, const BaseComponentLinkRef&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetComponentTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, EqualsDelta, const BaseComponentLinkRef&, const BaseComponentLinkRef&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
