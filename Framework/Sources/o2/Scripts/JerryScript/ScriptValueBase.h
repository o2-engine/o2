#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "3rdPartyLibs/jerryscript/jerry-core/include/jerryscript-core.h"
#include "o2/Utils/Function/Function.h"

#include <cstddef>
#include <new>
#include <utility>

namespace o2
{
    class IObject;
    class Type;

    // ---------------------------------------------------------
    // Base wrapper around JerryScript value and native bindings
    // ---------------------------------------------------------
    class ScriptValueBase
    {
    public:
        // Stored JerryScript value handle.
        mutable jerry_value_t jvalue;

    public:
        // Releases stored JerryScript value.
        virtual ~ScriptValueBase();

        // Acquires passed JerryScript value and releases current one.
        void AcquireValue(jerry_value_t v);

        // Accepts ownership of passed JerryScript value and releases current one.
        void Accept(jerry_value_t v);

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
            virtual jerry_value_t Invoke(jerry_value_t thisValue, jerry_value_t* args, int argsCount) = 0;
        };

        // -----------------------------------
        // Base interface for setter wrappers
        // -----------------------------------
        struct ISetterWrapperContainer : public IDataContainer
        {
            // Writes converted value into wrapped target.
            virtual void Set(jerry_value_t value) = 0;
        };

        // -----------------------------------
        // Base interface for getter wrappers
        // -----------------------------------
        struct IGetterWrapperContainer : public IDataContainer
        {
            // Reads wrapped value and converts it to JerryScript value.
            virtual jerry_value_t Get() = 0;
        };

        // ---------------------------------------------
        // Base interface for prototype property getters
        // ---------------------------------------------
        struct IPrototypeGetter : public IDataContainer
        {
            // Reads prototype-backed value from target object.
            virtual jerry_value_t GetFrom(jerry_value_t this_val) = 0;
        };

        // ---------------------------------------------
        // Base interface for prototype property setters
        // ---------------------------------------------
        struct IPrototypeSetter : public IDataContainer
        {
            // Writes prototype-backed value to target object.
            virtual void SetTo(jerry_value_t this_val, jerry_value_t value) = 0;
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
            void Set(jerry_value_t value) override;
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
            jerry_value_t Get() override;
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
            void Set(jerry_value_t value) override;
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
            jerry_value_t Get() override;
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
            void Set(jerry_value_t value) override;
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
            jerry_value_t Get() override;
        };

        // ---------------------------------------
        // JerryScript native pointer finalizer
        // ---------------------------------------
        struct DataContainerDeleter
        {
            // JerryScript native info descriptor.
            jerry_object_native_info_t info;

            // Initializes native info descriptor.
            DataContainerDeleter();

            // Frees native container instance.
            static void Free(void* ptr);
        };

        // Returns shared native-container deleter descriptor.
        static DataContainerDeleter& GetDataDeleter();

        // Returns native container bound to JerryScript object.
        static IDataContainer* GetNativeContainer(jerry_value_t jval);

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
        // JerryScript callback for wrapped native functions.
        static jerry_value_t CallFunction(const jerry_value_t function_obj,
                                          const jerry_value_t this_val,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count);

        // JerryScript callback for property setter wrappers.
        static jerry_value_t DescriptorSetter(const jerry_value_t function_obj,
                                              const jerry_value_t this_val,
                                              const jerry_value_t args_p[],
                                              const jerry_length_t args_count);

        // JerryScript callback for property getter wrappers.
        static jerry_value_t DescriptorGetter(const jerry_value_t function_obj,
                                              const jerry_value_t this_val,
                                              const jerry_value_t args_p[],
                                              const jerry_length_t args_count);

        // JerryScript callback for prototype property getters.
        static jerry_value_t PrototypeDescriptorGetter(const jerry_value_t function_obj,
                                                       const jerry_value_t this_val,
                                                       const jerry_value_t args_p[],
                                                       const jerry_length_t args_count);

        // JerryScript callback for prototype property setters.
        static jerry_value_t PrototypeDescriptorSetter(const jerry_value_t function_obj,
                                                       const jerry_value_t this_val,
                                                       const jerry_value_t args_p[],
                                                       const jerry_length_t args_count);

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
