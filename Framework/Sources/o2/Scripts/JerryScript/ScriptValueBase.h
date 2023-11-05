#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "jerryscript/jerry-core/include/jerryscript-core.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{
    class IObject;
    class Type;

    class ScriptValueBase
    {
    public:
        mutable jerry_value_t jvalue;

    public:
        virtual ~ScriptValueBase();

        // Releases current value and acquires new
        void AcquireValue(jerry_value_t v);

        // Releases current value and accepts new. After this class will release value
        void Accept(jerry_value_t v);

    public:
        struct IDataContainer
        {
            bool isDataOwner = true;

            virtual ~IDataContainer() = default;

            virtual void* GetData() const { return nullptr; }
            virtual IObject* TryCastToIObject() const { return nullptr; }
            virtual const Type* GetType() const { return nullptr; }
        };

        template<typename _type>
        struct DataContainer : public IDataContainer
        {
            _type* data;

            DataContainer(_type* data);
            ~DataContainer() override;

            void* GetData() const override;
            IObject* TryCastToIObject() const override;
            const Type* GetType() const override;
        };

        struct IFunctionContainer
        {
            virtual jerry_value_t Invoke(jerry_value_t thisValue, jerry_value_t* args, int argsCount) = 0;
        };

        template<typename _res_type, typename _this_type, typename ... _args>
        struct TFunctionContainer: public IFunctionContainer
        {
            jerry_value_t Invoke(jerry_value_t thisValue, jerry_value_t* args, int argsCount) override;

            virtual _res_type InvokeT(_this_type this_, _args ... args) const = 0;
        };

        struct ISetterWrapperContainer : public IDataContainer
        {
            virtual void Set(jerry_value_t value) = 0;
        };

        struct IGetterWrapperContainer : public IDataContainer
        {
            virtual jerry_value_t Get() = 0;
        };

        template<typename _type>
        struct PointerSetterWrapperContainer : public ISetterWrapperContainer
        {
            _type* dataPtr = nullptr;

            void Set(jerry_value_t value) override;
        };

        template<typename _type>
        struct PointerGetterWrapperContainer : public IGetterWrapperContainer
        {
            _type* dataPtr = nullptr;

            jerry_value_t Get() override;
        };

        template<typename _property_type>
        struct PropertySetterWrapperContainer : public ISetterWrapperContainer
        {
            _property_type* propertyPtr = nullptr;

            void Set(jerry_value_t value) override;
        };

        template<typename _property_type>
        struct PropertyGetterWrapperContainer : public IGetterWrapperContainer
        {
            _property_type* propertyPtr = nullptr;

            jerry_value_t Get() override;
        };

        template<typename _type>
        struct FunctionalSetterWrapperContainer : public ISetterWrapperContainer
        {
            Function<void(const _type& value)> setter;

            void Set(jerry_value_t value) override;
        };

        template<typename _type>
        struct FunctionalGetterWrapperContainer : public IGetterWrapperContainer
        {
            Function<_type()> getter;

            jerry_value_t Get() override;
        };

        struct DataContainerDeleter
        {
            jerry_object_native_info_t info;

            DataContainerDeleter();
            static void Free(void* ptr);
        };

    protected:
        static DataContainerDeleter& GetDataDeleter();

        static jerry_value_t CallFunction(const jerry_value_t function_obj,
                                          const jerry_value_t this_val,
                                          const jerry_value_t args_p[],
                                          const jerry_length_t args_count);

        static jerry_value_t DescriptorSetter(const jerry_value_t function_obj,
                                              const jerry_value_t this_val,
                                              const jerry_value_t args_p[],
                                              const jerry_length_t args_count);

        static jerry_value_t DescriptorGetter(const jerry_value_t function_obj,
                                              const jerry_value_t this_val,
                                              const jerry_value_t args_p[],
                                              const jerry_length_t args_count);

        template<size_t _i = 0, size_t _j = 0, typename... _args>
        static void UnpackArgs(std::tuple<_args ...>& argst, jerry_value_t* args, int argsCount);

        friend class ScriptEngine;
    };
}

#endif
