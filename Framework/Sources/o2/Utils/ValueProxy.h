#pragma once

#include "o2/Utils/Function/Function.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptValueDef.h"
#endif

namespace o2
{
    class Type;

    // ----------------------------------------------------------------------------
    // Abstract value proxy. Accepts by void*, value by pointer must match the type
    // ----------------------------------------------------------------------------
    class IAbstractValueProxy : public RefCounterable
    {
    public:
        // Destructor
        virtual ~IAbstractValueProxy() {}

        // Sets value by pointer. The type of value should match the type of proxy
        virtual void SetValuePtr(void* value) = 0;

        // Gets value by pointer. The type of value should match the type of proxy
        virtual void GetValuePtr(void* value) const = 0;

        // Returns the type of proxying value
        virtual const Type& GetType() const = 0;
    };

    // -----------------
    // Typed value proxy
    // -----------------
    template<typename _type>
    class IValueProxy : public IAbstractValueProxy
    {
    public:
        // Sets value by reference
        virtual void SetValue(const _type& value) = 0;

        // Gets value by value
        virtual _type GetValue() const = 0;

        // Returns the type of proxying value
        void SetValuePtr(void* value) override;

        // Gets value by pointer
        void GetValuePtr(void* value) const override;

        // Returns the type of proxying value
        const Type& GetType() const override;
    };

    // ------------------------------------------------------------
    // Pointer value proxy interface. Can returns to source pointer
    // ------------------------------------------------------------
    class IPointerValueProxy : public IRefCounterable
    {
    public:
        // Destructor
        virtual ~IPointerValueProxy() {}
        
        // Returns the pointer to value, stored in proxy
        virtual void* GetValueVoidPointer() const = 0;
    };

    // ------------------------------------------------------------------------
    // Typed pointer value proxy. Stores the pointer to value and works with it
    // ------------------------------------------------------------------------
    template<typename _type>
    class PointerValueProxy : public IValueProxy<_type>, public IPointerValueProxy
    {
    public:
        // Default constructor
        PointerValueProxy() {}

        // Constructor with value pointer
        PointerValueProxy(_type* valuePtr) :mValuePtr(valuePtr) {}

        // Sets value by reference
        void SetValue(const _type& value) override { *mValuePtr = value; }

        // Gets value by value
        _type GetValue() const override { return *mValuePtr; }

        // Sets value by pointer
        _type* GetValuePointer() const { return mValuePtr; }

        // Sets value by pointer
        void* GetValueVoidPointer() const override { return (void*)mValuePtr; }

        REF_COUNTERABLE_IMPL_TEMPLATE(PointerValueProxy<_type>, IValueProxy<_type>);

    protected:
        _type* mValuePtr = nullptr; // Proxying value pointer
    };

    // ----------------------------------------------------
    // Reference pointer value proxy. Works with Ref<_type>
    // ----------------------------------------------------
    class IRefPointerValueProxy
    {
    public:
        // Returns the pointer to value, stored in proxy
        virtual void* GetRefValueVoidPtr() const = 0;

        // Sets the pointer to value, stored in proxy
        virtual void SetRefValuePtr(void* value) = 0;
    };

    // ----------------------------------------------------
    // Reference pointer value proxy. Works with Ref<_type>
    // ----------------------------------------------------
    template<typename _type>
    class RefPointerValueProxy : public PointerValueProxy<Ref<_type>>, public IRefPointerValueProxy
    {
    public:
        using Base = PointerValueProxy<Ref<_type>>;

    public:
        // Default constructor
        RefPointerValueProxy() {}

        // Constructor with value pointer
        RefPointerValueProxy(Ref<_type>* valuePtr) :Base(valuePtr) {}

        // Returns the pointer to value, stored in proxy
        void* GetRefValueVoidPtr() const override { return Base::GetValuePointer()->Get(); }

        // Sets the pointer to value, stored in proxy
        void SetRefValuePtr(void* value) override { (*Base::GetValuePointer()) = Ref((_type*)value); }
    };


    // ----------------------------------------------------
    // Functional proxy, uses function to set and get value
    // ----------------------------------------------------
    template<typename _type>
    class FunctionalValueProxy : public IValueProxy<_type>
    {
    public:
        // Default constructor
        FunctionalValueProxy() {}

        // Constructor with setter and getter functions
        FunctionalValueProxy(const Function<void(_type)> setter, const Function<_type()> getter) :
            mSetter(setter), mGetter(getter)
        {}

        // Sets value by reference
        void SetValue(const _type& value) override { mSetter(value); }

        // Gets value by value
        _type GetValue() const override { return mGetter(); }

    protected:
        Function<void(_type)> mSetter; // Setter function
        Function<_type()>     mGetter; // Getter function
    };

    // -------------------------------------------------------------------------
    // Property proxy, uses setter and getter from property to set and get value
    // -------------------------------------------------------------------------
    template<typename _type, typename _property_type>
    class PropertyValueProxy : public IValueProxy<_type>
    {
    public:
        // Default constructor
        PropertyValueProxy() {}

        // Constructor with property pointer
        PropertyValueProxy(_property_type* ptr) :mProperty(ptr) {}

        // Equality operator
        bool operator==(const PropertyValueProxy<_type, _property_type>& other) const { return mProperty == other.mProperty; }

        // Sets value by reference
        void SetValue(const _type& value) override { mProperty->Set(value); }

        // Returns value from proxy
        _type GetValue() const override { return mProperty->Get(); }

    protected:
        _property_type* mProperty = nullptr; // Pointer to proxied property
    };


#if IS_SCRIPTING_SUPPORTED
    // --------------------------
    // Unified script value proxy
    // --------------------------
    class ScriptValueProxy : public IAbstractValueProxy
    {
    public:
        IScriptValueProperty* scriptProperty = nullptr; // Script value property

    public:
        // Default constructor
        ScriptValueProxy();

        // Constructor with script property
        ScriptValueProxy(IScriptValueProperty* prop);

        // Destructor
        ~ScriptValueProxy() override;

        // Sets value by pointer
        void SetValuePtr(void* value) override;

        // Gets value by pointer
        void GetValuePtr(void* value) const override;

        // Returns the type of proxying value
        const Type& GetType() const override;
    };

    // ------------------------
    // Script value typed proxy
    // ------------------------
    template<typename _type>
    class TypeScriptValueProxy : public IValueProxy<_type>, public ScriptValueProperty
    {
    public:
        IScriptValueProperty* scriptProperty = nullptr; // Script value property

    public:
        // Default constructor
        TypeScriptValueProxy();

        // Constructor with script property
        TypeScriptValueProxy(IScriptValueProperty* prop);

        // Destructor
        ~TypeScriptValueProxy() override;

        // Sets value by reference
        void SetValue(const _type& value) override;

        // Returns value
        _type GetValue() const override;

        REF_COUNTERABLE_IMPL_TEMPLATE(TypeScriptValueProxy<_type>, IValueProxy<_type>);
    };

    // ----------------------------------------
    // Pointer script value proxy, using _type*
    // ----------------------------------------
    template<typename _type>
    class PtrScriptValueProxy : public IValueProxy<_type>, public ScriptValueProperty
    {
    public:
        IScriptValueProperty* scriptProperty = nullptr; // Script value property

    public:
        // Default constructor
        PtrScriptValueProxy();

        // Constructor with script property
        PtrScriptValueProxy(IScriptValueProperty* prop);

        // Destructor
        ~PtrScriptValueProxy() override;

        // Sets value by pointer
        void SetValue(const _type& value) override;

        // Returns value
        _type GetValue() const override;

        // Returns type of value
        const Type& GetType() const override;
    };
#endif
}

#include "o2/Utils/Reflection/Type.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptValue.h"
#endif

namespace o2
{
    template<typename _type>
    void IValueProxy<_type>::SetValuePtr(void* value)
    {
        SetValue(*(_type*)value);
    }

    template<typename _type>
    void IValueProxy<_type>::GetValuePtr(void* value) const
    {
        *(_type*)value = GetValue();
    }

    template<typename _type>
    const Type& IValueProxy<_type>::GetType() const
    {
        return TypeOf(_type);
    }

#if IS_SCRIPTING_SUPPORTED
    template<typename _type>
    TypeScriptValueProxy<_type>::TypeScriptValueProxy()
    {}

    template<typename _type>
    TypeScriptValueProxy<_type>::TypeScriptValueProxy(IScriptValueProperty* prop)
        :scriptProperty(prop)
    {}

    template<typename _type>
    TypeScriptValueProxy<_type>::~TypeScriptValueProxy()
    {
        if (scriptProperty)
            delete scriptProperty;
    }

    template<typename _type>
    void TypeScriptValueProxy<_type>::SetValue(const _type& value)
    {
        scriptProperty->Set(ScriptValue(value));
    }

    template<typename _type>
    _type TypeScriptValueProxy<_type>::GetValue() const
    {
        return scriptProperty->Get().GetValue<_type>();
    }

    template<typename _type>
    PtrScriptValueProxy<_type>::PtrScriptValueProxy()
    {}

    template<typename _type>
    PtrScriptValueProxy<_type>::PtrScriptValueProxy(IScriptValueProperty* prop)
        :scriptProperty(prop)
    {}

    template<typename _type>
    PtrScriptValueProxy<_type>::~PtrScriptValueProxy()
    {
        if (scriptProperty)
            delete scriptProperty;
    }

    template<typename _type>
    void PtrScriptValueProxy<_type>::SetValue(const _type& value)
    {
        scriptProperty->Set(ScriptValue(mnew _type(value)));
    }

    template<typename _type>
    _type PtrScriptValueProxy<_type>::GetValue() const
    {
        return *scriptProperty->Get().GetValue<_type*>();
    }

    template<typename _type>
    const Type& PtrScriptValueProxy<_type>::GetType() const
    {
        if constexpr (std::is_base_of<IObject, _type>::value)
            return scriptProperty->Get().GetValue<_type*>()->GetType();
        else
            return TypeOf(_type);
    }
#endif
}
