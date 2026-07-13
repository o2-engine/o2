#if defined(SCRIPTING_BACKEND_QUICKJS)

#include "o2/Scripts/QuickJS/QuickJSCore.h"

#include "o2/Utils/Function/Function.h"

#include <cstddef>
#include <new>
#include <utility>

namespace o2
{
    class IObject;
    class Type;

    // -----------------------------------------------------------
    // Base wrapper around QuickJS value and native bindings
    // -----------------------------------------------------------
    class ScriptValueBase
    {
    public:
        // Stored value; thrown values carry the error flag
        mutable JSValue mValue = JS_UNDEFINED;
        mutable bool mIsError = false;

    public:
        // Releases stored value.
        virtual ~ScriptValueBase();

        // Acquires passed value and releases current one.
        void AcquireValue(JSValueConst v);

        // Accepts ownership of passed value and releases current one.
        void Accept(JSValue v);

        // Accepts the pending context exception as an error value.
        void AcceptThrown();

    public:
        // ------------------------------------------------
        // Base interface for native value container binding
        // ------------------------------------------------
        struct IDataContainer
        {
            // Destroys container instance.
            virtual ~IDataContainer() = default;

            // Destroys current container through the allocator-aware path.
            virtual void Destroy() = 0;

            // Returns raw stored data pointer.
            virtual void* GetData() const { return nullptr; }

            // Tries to cast stored data to IObject.
            virtual IObject* TryCastToIObject() const { return nullptr; }

            // Returns runtime type of stored data.
            virtual const Type* GetType() const { return nullptr; }

            // Creates container clone when supported.
            virtual IDataContainer* Clone() const { return nullptr; }
        };

        // --------------------------------------------------------
        // Pool-allocated container base with allocator-aware delete
        // --------------------------------------------------------
        template<typename _container, typename _base = IDataContainer>
        struct TPoolContainer : public _base
        {
            // Destroys current container instance.
            void Destroy() override;
        };

        // --------------------------------
        // Container storing value by copy
        // --------------------------------
        template<typename _type>
        struct DataContainer : public TPoolContainer<DataContainer<_type>>
        {
            // Stored value.
            _type data;

            // Initializes container from copied value.
            DataContainer(const _type& d);

            // Initializes container from moved value.
            DataContainer(_type&& d);

            // Destroys stored value.
            ~DataContainer() override = default;

            // Returns pointer to stored data.
            void* GetData() const override;

            // Tries to cast stored value to IObject.
            IObject* TryCastToIObject() const override;

            // Returns runtime type of stored value.
            const Type* GetType() const override;

            // Clones current container when value is copy-constructible.
            IDataContainer* Clone() const override;
        };

        // ----------------------------------
        // Container storing raw object pointer
        // ----------------------------------
        template<typename _type>
        struct PointerDataContainer : public TPoolContainer<PointerDataContainer<_type>>
        {
            // Stored raw pointer.
            _type* data = nullptr;

            // Initializes container with raw pointer.
            explicit PointerDataContainer(_type* d);

            // Destroys container.
            ~PointerDataContainer() override = default;

            // Returns stored pointer.
            void* GetData() const override;

            // Tries to cast stored pointer to IObject.
            IObject* TryCastToIObject() const override;

            // Returns pointed object type.
            const Type* GetType() const override;

            // Clones current container.
            IDataContainer* Clone() const override;
        };

        // ------------------------------------
        // Base interface for native callables
        // ------------------------------------
        struct IFunctionContainer : public IDataContainer
        {
            // Invokes stored callable.
            virtual JSValue Invoke(JSValueConst thisValue, JSValueConst* args, int argsCount) = 0;
        };

        // -----------------------------------
        // Base interface for setter wrappers
        // -----------------------------------
        struct ISetterWrapperContainer : public IDataContainer
        {
            // Writes converted value into wrapped target.
            virtual void Set(JSValueConst value) = 0;
        };

        // -----------------------------------
        // Base interface for getter wrappers
        // -----------------------------------
        struct IGetterWrapperContainer : public IDataContainer
        {
            // Reads wrapped value and converts it to script value.
            virtual JSValue Get() = 0;
        };

        // ---------------------------------------------
        // Base interface for prototype property getters
        // ---------------------------------------------
        struct IPrototypeGetter : public IDataContainer
        {
            // Reads prototype-backed value from target object.
            virtual JSValue GetFrom(JSValueConst this_val) = 0;
        };

        // ---------------------------------------------
        // Base interface for prototype property setters
        // ---------------------------------------------
        struct IPrototypeSetter : public IDataContainer
        {
            // Writes prototype-backed value to target object.
            virtual void SetTo(JSValueConst this_val, JSValueConst value) = 0;
        };

        // ---------------------------------------
        // Setter wrapper around direct data pointer
        // ---------------------------------------
        template<typename _type>
        struct PointerSetterWrapperContainer : public TPoolContainer<PointerSetterWrapperContainer<_type>, ISetterWrapperContainer>
        {
            // Wrapped value pointer.
            _type* dataPtr = nullptr;

            // Converts and writes passed value.
            void Set(JSValueConst value) override;
        };

        // ---------------------------------------
        // Getter wrapper around direct data pointer
        // ---------------------------------------
        template<typename _type>
        struct PointerGetterWrapperContainer : public TPoolContainer<PointerGetterWrapperContainer<_type>, IGetterWrapperContainer>
        {
            // Wrapped value pointer.
            _type* dataPtr = nullptr;

            // Reads wrapped value.
            JSValue Get() override;
        };

        // -----------------------------------------
        // Setter wrapper around reflected property
        // -----------------------------------------
        template<typename _property_type>
        struct PropertySetterWrapperContainer : public TPoolContainer<PropertySetterWrapperContainer<_property_type>, ISetterWrapperContainer>
        {
            // Wrapped property pointer.
            _property_type* propertyPtr = nullptr;

            // Converts and writes passed value.
            void Set(JSValueConst value) override;
        };

        // -----------------------------------------
        // Getter wrapper around reflected property
        // -----------------------------------------
        template<typename _property_type>
        struct PropertyGetterWrapperContainer : public TPoolContainer<PropertyGetterWrapperContainer<_property_type>, IGetterWrapperContainer>
        {
            // Wrapped property pointer.
            _property_type* propertyPtr = nullptr;

            // Reads wrapped property value.
            JSValue Get() override;
        };

        // ----------------------------------------
        // Setter wrapper around functional binding
        // ----------------------------------------
        template<typename _type>
        struct FunctionalSetterWrapperContainer : public TPoolContainer<FunctionalSetterWrapperContainer<_type>, ISetterWrapperContainer>
        {
            // Bound setter callback.
            Function<void(const _type& value)> setter;

            // Converts and forwards passed value.
            void Set(JSValueConst value) override;
        };

        // ----------------------------------------
        // Getter wrapper around functional binding
        // ----------------------------------------
        template<typename _type>
        struct FunctionalGetterWrapperContainer : public TPoolContainer<FunctionalGetterWrapperContainer<_type>, IGetterWrapperContainer>
        {
            // Bound getter callback.
            Function<_type()> getter;

            // Reads value through callback.
            JSValue Get() override;
        };

        // Destroys native container instance; used as engine finalizer callback
        static void FreeDataContainer(void* ptr);

        // Returns native container bound to script object.
        static IDataContainer* GetNativeContainer(JSValueConst val);

        // Allocates and constructs pooled container.
        template<typename _container, typename ... _args>
        static _container* CreateContainer(_args&&... args);

        // Destroys and frees pooled container.
        template<typename _container>
        static void DestroyContainer(_container* container);

        // Allocates raw memory for native container.
        static void* AllocateContainerMemory(size_t size, size_t alignment = alignof(std::max_align_t));

        // Frees raw memory allocated for native container.
        static void  FreeContainerMemory(void* ptr);

    protected:
        // Engine callback for wrapped native functions.
        static JSValue CallFunction(JSValueConst function_obj, JSValueConst this_val,
                                    JSValueConst* args_p, int args_count);

        // Engine callback for property setter wrappers.
        static JSValue DescriptorSetter(JSValueConst function_obj, JSValueConst this_val,
                                        JSValueConst* args_p, int args_count);

        // Engine callback for property getter wrappers.
        static JSValue DescriptorGetter(JSValueConst function_obj, JSValueConst this_val,
                                        JSValueConst* args_p, int args_count);

        // Engine callback for prototype property getters.
        static JSValue PrototypeDescriptorGetter(JSValueConst function_obj, JSValueConst this_val,
                                                 JSValueConst* args_p, int args_count);

        // Engine callback for prototype property setters.
        static JSValue PrototypeDescriptorSetter(JSValueConst function_obj, JSValueConst this_val,
                                                 JSValueConst* args_p, int args_count);

        friend class ScriptEngine;
    };

    template<typename _container, typename _base>
    void ScriptValueBase::TPoolContainer<_container, _base>::Destroy()
    {
        ScriptValueBase::DestroyContainer(static_cast<_container*>(this));
    }

    template<typename _container, typename ... _args>
    _container* ScriptValueBase::CreateContainer(_args&&... args)
    {
        void* memory = AllocateContainerMemory(sizeof(_container), alignof(_container));
        return new (memory) _container(std::forward<_args>(args)...);
    }

    template<typename _container>
    void ScriptValueBase::DestroyContainer(_container* container)
    {
        if (!container)
            return;

        container->~_container();
        FreeContainerMemory(container);
    }
}

#endif
