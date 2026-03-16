#pragma once
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"

namespace o2
{
    class ISerializable;

    template<class T, class = void_t<>>
    struct HasCheckSerialize: std::false_type { };

    template<class T>
    struct HasCheckSerialize<T, void_t<decltype(&T::CheckSerialize)>>: std::true_type { };

    template<class T, class = void_t<>>
    struct HasAttributeSerializeProcessor: std::false_type { };

    template<class T>
    struct HasAttributeSerializeProcessor<T, void_t<decltype(&T::hasSerializeFieldProcessor)>>: std::true_type { };

    template<class T, class = void_t<>>
    struct HasAttributeDeserializeProcessor: std::false_type { };

    template<class T>
    struct HasAttributeDeserializeProcessor<T, void_t<decltype(&T::hasDeserializeFieldProcessor)>>: std::true_type { };

    class SerializeTypeProcessor
    {
    public:
        struct BaseFieldProcessor;

        template<typename _type>
        struct SerializeDefaultFieldProcessor;

    public:
        DataValue& node;

    public:
        SerializeTypeProcessor(DataValue& node):node(node) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name)
        {
            if constexpr (std::is_base_of<ISerializable, _base_type>::value && !std::is_same<ISerializable, _base_type>::value)
                object->_base_type::SerializeBasic(node);
        }

        BaseFieldProcessor StartField()
        {
            return BaseFieldProcessor(node);
        }

        struct BaseFieldProcessor
        {
            DataValue& node;

            BaseFieldProcessor(DataValue& node):node(node) {}
            
            template<typename _base, typename _attribute_type>
            struct AttributeWrapper: public _attribute_type::template SerializeFieldProcessor<_base>
            {
                template<typename ... _args>
                AttributeWrapper(const _base& base, _args ... args):_attribute_type::template SerializeFieldProcessor<_base>(base, args ...) {}
            };

            template<typename _base, typename _attribute_type, typename ... _args>
            auto AddAttributeImpl(const _base& base, _args ... args)
            {
                if constexpr (HasAttributeSerializeProcessor<_attribute_type>::value)
                {
                    return AttributeWrapper<_base, _attribute_type>(base, args ...);
                }
                else
                    return *this;
            }

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return AddAttributeImpl<BaseFieldProcessor, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            BaseFieldProcessor& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            BaseFieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                            _field_type& field)
            {
                return *this;
            }

            BaseFieldProcessor& SetProtectSection(ProtectSection section)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                return true;
            }
        };
    };

    class DeserializeTypeProcessor
    {
    public:
        struct BaseFieldProcessor;

        template<typename _type>
        struct DeserializeDefaultFieldProcessor;

    public:
        const DataValue& node;

    public:
        DeserializeTypeProcessor(const DataValue& node):node(node) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name)
        {
            if constexpr (std::is_base_of<ISerializable, _base_type>::value && !std::is_same<ISerializable, _base_type>::value)
                object->_base_type::DeserializeBasic(node);
        }

        BaseFieldProcessor StartField()
        {
            return BaseFieldProcessor(node);
        }

        struct BaseFieldProcessor
        {
            const DataValue& node;

            BaseFieldProcessor(const DataValue& node):node(node) {}
            
            template<typename _base, typename _attribute_type>
            struct AttributeWrapper: public _attribute_type::template DeserializeFieldProcessor<_base>
            {
                template<typename ... _args>
                AttributeWrapper(const _base& base, _args ... args):_attribute_type::template DeserializeFieldProcessor<_base>(base, args ...) {}
            };

            template<typename _base, typename _attribute_type, typename ... _args>
            auto AddAttributeImpl(const _base& base, _args ... args)
            {
                if constexpr (HasAttributeDeserializeProcessor<_attribute_type>::value)
                    return AttributeWrapper<_base, _attribute_type>(base, args ...);
                else
                    return (_base&)(*this);
            }

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return AddAttributeImpl<BaseFieldProcessor, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            BaseFieldProcessor& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            BaseFieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                            _field_type& field)
            {
                return *this;
            }

            BaseFieldProcessor& SetProtectSection(ProtectSection section)
            {
                return *this;
            }
        };
    };

    template<typename _origin_type>
    class SerializeDeltaTypeProcessor
    {
    public:
        struct BaseFieldProcessor;

    public:
        DataValue& node;
        const _origin_type& origin;
        const IObject& originObj;

    public:
        SerializeDeltaTypeProcessor(DataValue& node, const _origin_type& origin, const IObject& originObj):node(node), origin(origin), originObj(originObj) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name)
        {
            if constexpr (std::is_base_of<ISerializable, _base_type>::value && !std::is_same<ISerializable, _base_type>::value)
                object->_base_type::SerializeDeltaBasic(node, originObj);
        }

        BaseFieldProcessor StartField()
        {
            return BaseFieldProcessor(node, origin);
        }

        struct BaseFieldProcessor
        {
            DataValue& node;
            const _origin_type& origin;

            typedef _origin_type OriginType;

            BaseFieldProcessor(DataValue& node, const _origin_type& origin):node(node), origin(origin) {}
            
            template<typename _base, typename _attribute_type>
            struct AttributeWrapper: public _attribute_type::template SerializeDeltaFieldProcessor<_base>
            {
                template<typename ... _args>
                AttributeWrapper(const _base& base, _args ... args):_attribute_type::template SerializeDeltaFieldProcessor<_base>(base, args ...) {}
            };

            template<typename _base, typename _attribute_type, typename ... _args>
            auto AddAttributeImpl(const _base& base, _args ... args)
            {
                if constexpr (HasAttributeSerializeProcessor<_attribute_type>::value)
                    return AttributeWrapper<_base, _attribute_type>(base, args ...);
                else
                    return *this;
            }

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return AddAttributeImpl<BaseFieldProcessor, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            BaseFieldProcessor& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            BaseFieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                            _field_type& field)
            {
                return *this;
            }

            BaseFieldProcessor& SetProtectSection(ProtectSection section)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                return true;
            }
        };
    };

    template<typename _origin_type>
    class DeserializeDeltaTypeProcessor
    {
    public:
        struct BaseFieldProcessor;

        template<typename _type>
        struct DeserializeDefaultFieldProcessor;

    public:
        const DataValue& node;
        const _origin_type& origin;
        const IObject& originObj;

    public:
        DeserializeDeltaTypeProcessor(const DataValue& node, const _origin_type& origin, const IObject& originObj):node(node), origin(origin), originObj(originObj) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name)
        {
            if constexpr (std::is_base_of<ISerializable, _base_type>::value && !std::is_same<ISerializable, _base_type>::value)
                object->_base_type::DeserializeDeltaBasic(node, originObj);
        }

        BaseFieldProcessor StartField()
        {
            return BaseFieldProcessor(node, origin);
        }

        struct BaseFieldProcessor
        {
            const DataValue& node;
            const _origin_type& origin;

            typedef _origin_type OriginType;

            BaseFieldProcessor(const DataValue& node, const _origin_type& origin):node(node), origin(origin) {}

            template<typename _base, typename _attribute_type>
            struct AttributeWrapper: public _attribute_type::template DeserializeDeltaFieldProcessor<_base>
            {
                template<typename ... _args>
                AttributeWrapper(const _base& base, _args ... args):_attribute_type::template DeserializeDeltaFieldProcessor<_base>(base, args ...) {}
            };
            
            template<typename _base, typename _attribute_type, typename ... _args>
            auto AddAttributeImpl(const _base& base, _args ... args)
            {
                if constexpr (HasAttributeDeserializeProcessor<_attribute_type>::value)
                    return AttributeWrapper<_base, _attribute_type>(base, args ...);
                else
                    return base;
            }

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return AddAttributeImpl<BaseFieldProcessor, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            BaseFieldProcessor& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            BaseFieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                            _field_type& field)
            {
                return *this;
            }

            BaseFieldProcessor& SetProtectSection(ProtectSection section)
            {
                return *this;
            }
        };
    };
}
