
#pragma once

#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/ValueProxy.h"

namespace o2
{
    template<typename _type, typename _enable>
    struct CheckSerializeBasicOverridden;

    template<typename _type, typename _enable>
    struct CheckDeserializeBasicOverridden;

    // -----------------------------
    // Serializable object interface
    // -----------------------------
    class ISerializable: public IObject
    {
    public:
        // Serializing object into data node
        virtual void Serialize(DataValue& node) const {}

        // Deserializing object from data node
        virtual void Deserialize(const DataValue& node) {};

        // Serializing object delta from origin into data node
        virtual void SerializeDelta(DataValue& node, const IObject& origin) const {}

        // Deserializing object delta from origin from data node
        virtual void DeserializeDelta(const DataValue& node, const IObject& origin) {};

        // Serializes data to string
        String SerializeToString() const;

        // Deserializes data from string
        void DeserializeFromString(const String& str);

        // Assign operator from data node
        ISerializable& operator=(const DataValue& node) { return *this; };

        // DataDocument converting operator
        explicit operator DataDocument() const { return DataDocument(); }

        // Internal wrappers used by serialization processors to avoid friending every helper type
        void SerializeBasicInternal(DataValue& node) const { SerializeBasic(node); }
        void DeserializeBasicInternal(const DataValue& node) { DeserializeBasic(node); }
        void SerializeDeltaBasicInternal(DataValue& node, const IObject& origin) const { SerializeDeltaBasic(node, origin); }
        void DeserializeDeltaBasicInternal(const DataValue& node, const IObject& origin) { DeserializeDeltaBasic(node, origin); }
        void OnSerializeInternal(DataValue& node) const { OnSerialize(node); }
        void OnDeserializedInternal(const DataValue& node) { OnDeserialized(node); }
        void OnSerializeDeltaInternal(DataValue& node, const IObject& origin) const { OnSerializeDelta(node, origin); }
        void OnDeserializedDeltaInternal(const DataValue& node, const IObject& origin) { OnDeserializedDelta(node, origin); }

        IOBJECT(ISerializable);

    protected:
        // Serializing object into data node
        virtual void SerializeBasic(DataValue& node) const {}

        // Deserializing object from data node
        virtual void DeserializeBasic(const DataValue& node) {}

        // Serializing object into data node
        virtual void SerializeDeltaBasic(DataValue& node, const IObject& origin) const {}

        // Deserializing object from data node
        virtual void DeserializeDeltaBasic(const DataValue& node, const IObject& origin) {}

        // Beginning serialization callback
        virtual void OnSerialize(DataValue& node) const {}

        // Completion deserialization callback
        virtual void OnDeserialized(const DataValue& node) {}

        // Beginning serialization delta callback
        virtual void OnSerializeDelta(DataValue& node, const IObject& origin) const {}

        // Completion deserialization delta callback
        virtual void OnDeserializedDelta(const DataValue& node, const IObject& origin) {}

        friend class SerializeTypeProcessor;
        friend class DeserializeTypeProcessor;

        template<typename _origin_type>
        friend class SerializeDeltaTypeProcessor;

        template<typename _origin_type>
        friend class DeserializeDeltaTypeProcessor;

        template<typename _type, typename _enable>
        friend struct CheckSerializeBasicOverridden;

        template<typename _type, typename _enable>
        friend struct CheckDeserializeBasicOverridden;
    };

    template<typename T>
    struct DataValue::Converter<T, typename std::enable_if<std::is_base_of<ISerializable, T>::value>::type>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, DataValue& data)
        {
            data.SetObject();
            value.Serialize(data);
        }

        static void Read(T& value, const DataValue& data)
        {
            value.Deserialize(data);
        }
    };

    template<typename T>
    struct DataValue::DeltaConverter<T, typename std::enable_if<std::is_base_of<ISerializable, T>::value>::type>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, const T& origin, DataValue& data)
        {
            value.SerializeDelta(data, dynamic_cast<const IObject&>(origin));
        }

        static void Read(T& value, const T& origin, const DataValue& data)
        {
            value.DeserializeDelta(data, dynamic_cast<const IObject&>(origin));
        }
    };

    template<typename _type, typename _enable = void>
    struct CheckSerializeBasicOverridden
    {
        // Default serialization way
        static void Process(_type* object, DataValue& node)
        {
            object->SerializeBasicInternal(node);
            object->OnSerializeInternal(node);
        }
    };

    template<typename T>
    struct CheckSerializeBasicOverridden<T, typename std::void_t<decltype(&T::SerializeBasicOverride)>>
    {
        // Using overridden function
        static void Process(T* object, DataValue& node)
        {
            object->SerializeBasicOverride(node);
        }
    };

    template<typename _type, typename _enable = void>
    struct CheckDeserializeBasicOverridden
    {
        // Default serialization way
        static void Process(_type* object, const DataValue& node)
        {
            object->DeserializeBasicInternal(node);
            object->OnDeserializedInternal(node);
        }
    };

    template<typename T>
    struct CheckDeserializeBasicOverridden<T, typename std::void_t<decltype(&T::DeserializeBasicOverride)>>
    {
        // Using overridden function
        static void Process(T* object, const DataValue& node)
        {
            object->DeserializeBasicOverride(node);
        }
    };

    template<typename _type, typename _enable = void>
    struct CheckSerializeDeltaBasicOverridden
    {
        // Default serialization way
        static void Process(_type* object, const _type* origin, const IObject& originObj, DataValue& node)
        {
            object->SerializeDeltaBasicInternal(node, originObj);
            object->OnSerializeDeltaInternal(node, originObj);
        }
    };

    template<typename T>
    struct CheckSerializeDeltaBasicOverridden<T, typename std::void_t<decltype(&T::SerializeDeltaBasicOverride)>>
    {
        // Using overridden function
        static void Process(T* object, const T* origin, const IObject& originObj, DataValue& node)
        {
            object->SerializeDeltaBasicOverride(node, *origin);
        }
    };

    template<typename _type, typename _enable = void>
    struct CheckDeserializeDeltaBasicOverridden
    {
        // Default serialization way
        static void Process(_type* object, const _type* origin, const IObject& originObj, const DataValue& node)
        {
            object->DeserializeDeltaBasicInternal(node, originObj);
            object->OnDeserializedDeltaInternal(node, originObj);
        }
    };

    template<typename T>
    struct CheckDeserializeDeltaBasicOverridden<T, typename std::void_t<decltype(&T::DeserializeDeltaBasicOverride)>>
    {
        // Using overridden function
        static void Process(T* object, const T* origin, const IObject& originObj, const DataValue& node)
        {
            object->DeserializeDeltaBasicOverride(node, *origin);
        }
    };

    // ----------------------------
    // Serializable field attribute
    // ----------------------------
    class SerializableAttribute: public IAttribute
    {
    public:
        ATTRIBUTE_COMMENT_DEFINITION("SERIALIZABLE");
        ATTRIBUTE_SHORT_DEFINITION("SERIALIZABLE_ATTRIBUTE");

    public:
        static constexpr bool hasSerializeFieldProcessor = true;
        static constexpr bool hasDeserializeFieldProcessor = true;

        template<typename _base, typename _type>
        struct SerializeDefaultProcessorMixin: public _base
        {
            _type defaultValue;

            SerializeDefaultProcessorMixin(const _base& base, const _type& defaultValue):
                _base(base), defaultValue(defaultValue) {}

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                return *fieldPtr != defaultValue;
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                              _field_type& field)
            {
                if (!CheckSerialize(object, type, name, pointerGetter, field))
                    return *this;

                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _base::node.AddMember(name).Set(*fieldPtr);

                return *this;
            }
        };

        template<typename _base>
        struct SerializeFieldProcessor: public _base
        {
            SerializeFieldProcessor(const _base& base): _base(base) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<SerializeFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto SetDefaultValue(const _type& value)
            {
                return SerializeDefaultProcessorMixin<SerializeFieldProcessor<_base>, _type>(*this, value);
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                if (!_base::CheckSerialize(object, type, name, pointerGetter, field))
                    return false;

                if constexpr (std::is_default_constructible<_field_type>::value && SupportsEqualOperator<_field_type>::value)
                {
                    _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                    if (Math::Equals(*fieldPtr, _field_type()))
                        return false;
                }

                return true;
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                              _field_type& field)
            {
                if (!CheckSerialize(object, type, name, pointerGetter, field))
                    return *this;

                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _base::node.AddMember(name).Set(*fieldPtr);

                return *this;
            }
        };

        template<typename _base>
        struct DeserializeFieldProcessor: public _base
        {
            DeserializeFieldProcessor(const _base& base): _base(base) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<DeserializeFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                              _field_type& field)
            {
                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));

                if (auto m = _base::node.FindMember(name))
                    m->Get(*fieldPtr);

                return *this;
            }
        };

        template<typename _base>
        struct SerializeDeltaFieldProcessor: public _base
        {
            SerializeDeltaFieldProcessor(const _base& base):_base(base) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<SerializeDeltaFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                if (!_base::CheckSerialize(object, type, name, pointerGetter, field))
                    return false;

                if constexpr (SupportsEqualOperator<_field_type>::value)
                {
                    _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                    _field_type* originFieldPtr = (_field_type*)((*pointerGetter)(&const_cast<_object_type&>(_base::origin)));

                    if (EqualsForDeltaSerialize(*fieldPtr, *originFieldPtr))
                        return false;
                }

                return true;
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                                 _field_type& field)
            {
                if (!CheckSerialize(object, type, name, pointerGetter, field))
                    return *this;

                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _field_type* originFieldPtr = (_field_type*)((*pointerGetter)(&const_cast<_object_type&>(_base::origin)));

                DataValue& member = _base::node.AddMember(name);
                member.SetDelta(*fieldPtr, *originFieldPtr);
                if (member.IsNull())
                    _base::node.RemoveMember(name);

                return *this;
            }
        };

        template<typename _base>
        struct DeserializeDeltaFieldProcessor: public _base
        {
            DeserializeDeltaFieldProcessor(const _base& base):_base(base) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<DeserializeDeltaFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                                   _field_type& field)
            {
                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _field_type* originFieldPtr = (_field_type*)((*pointerGetter)(&const_cast<_object_type&>(_base::origin)));

                if (auto m = _base::node.FindMember(name); m && !m->IsNull())
                    m->GetDelta(*fieldPtr, *originFieldPtr);
                else
                {
                    _field_type* originFieldPtr = (_field_type*)((*pointerGetter)(&const_cast<_object_type&>(_base::origin)));
                    if constexpr (std::is_pointer<_field_type>::value &&
                                  std::is_copy_constructible<typename std::remove_pointer<_field_type>::type>::value)
                    {
                        if (*originFieldPtr != nullptr)
                        {
                            typedef typename std::remove_pointer<_field_type>::type _field_type_unp;
                            if constexpr (IsCloneable<_field_type_unp>::value)
                            {
                                if constexpr (std::is_same<typename std::invoke_result<decltype(&_field_type_unp::Clone)()>::type, _field_type>::value)
                                    *fieldPtr = (*originFieldPtr)->Clone();
                                else
                                    *fieldPtr = dynamic_cast<_field_type>((*originFieldPtr)->Clone());
                            }
                            else
                                *fieldPtr = mnew typename std::remove_pointer<_field_type>::type(**originFieldPtr);
                        }
                    }
                    else
                    {
                        *fieldPtr = *originFieldPtr;
                    }
                }

                return *this;
            }
        };
    };

    // ----------------------------------
    // Serialize with condition attribute
    // ----------------------------------
    class ISerializeIfAttribute: public IAttribute
    {
    public:
        virtual bool Invoke(const void* object) const = 0;
    };


    template<typename _class>
    class SerializeIfAttribute: public ISerializeIfAttribute
    {
        ATTRIBUTE_COMMENT_DEFINITION("SERIALIZE_IF");
        ATTRIBUTE_SHORT_DEFINITION("SERIALIZE_IF_ATTRIBUTE");

    public:
        bool(_class::*functionPtr)() const;

        SerializeIfAttribute(bool(_class::*functionPtr)() const): functionPtr(functionPtr) {}

        bool Invoke(const _class* object) const { return (object->*functionPtr)(); }
        bool Invoke(const void* object) const override { return Invoke((_class*)object); }

    public:
        static constexpr bool hasSerializeFieldProcessor = true;

        template<typename _base>
        struct SerializeFieldProcessor: public _base
        {
            bool(_class::*functionPtr)() const;

            SerializeFieldProcessor(const _base& base, bool(_class::*functionPtr)() const):
                _base(base), functionPtr(functionPtr) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<SerializeFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                if (!(object->*functionPtr)())
                    return false;

                return _base::CheckSerialize(object, type, name, pointerGetter, field);
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                              _field_type& field)
            {
                if (!CheckSerialize(object, type, name, pointerGetter, field))
                    return *this;

                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _base::node.AddMember(name).Set(*fieldPtr);

                return *this;
            }
        };

        template<typename _base>
        struct SerializeDeltaFieldProcessor: public _base
        {
            bool(_class::*functionPtr)() const;

            SerializeDeltaFieldProcessor(const _base& base, bool(_class::*functionPtr)() const):
                _base(base), functionPtr(functionPtr) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args)
            {
                return _base::template AddAttributeImpl<SerializeDeltaFieldProcessor<_base>, _attribute_type, _args ...>(*this, args ...);
            }

            template<typename _type>
            auto& SetDefaultValue(const _type& value)
            {
                return *this;
            }

            template<typename _object_type, typename _field_type>
            bool CheckSerialize(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                                _field_type& field)
            {
                if (!(object->*functionPtr)())
                    return false;

                return _base::CheckSerialize(object, type, name, pointerGetter, field);
            }

            template<typename _object_type, typename _field_type>
            auto& FieldBasics(_object_type* object, Type* type, const char* name, void*(*pointerGetter)(void*),
                              _field_type& field)
            {
                if (!CheckSerialize(object, type, name, pointerGetter, field))
                    return *this;

                _field_type* fieldPtr = (_field_type*)((*pointerGetter)(object));
                _field_type* originFieldPtr = (_field_type*)((*pointerGetter)(&const_cast<typename _base::OriginType&>(_base::origin)));

                DataValue& member = _base::node.AddMember(name);
                member.SetDelta(*fieldPtr, *originFieldPtr);
                if (member.IsNull())
                    _base::node.RemoveMember(name);

                return *this;
            }
        };
    };

#define SERIALIZABLE_ATTRIBUTE() \
    template AddAttribute<o2::SerializableAttribute>()

#define SERIALIZE_IF_ATTRIBUTE(FUNC) \
    template AddAttribute<o2::SerializeIfAttribute<thisclass>>(&thisclass::FUNC)

    // Serialization implementation macros
#define SERIALIZABLE_MAIN(CLASS)                                                                                                                \
    IOBJECT_MAIN(CLASS)                                                                                                                         \
                                                                                                                                                \
    template<typename __type, typename _enable>                                                                                                 \
    friend struct o2::CheckSerializeBasicOverridden;                                                                                            \
                                                                                                                                                \
    template<typename __type, typename _enable>                                                                                                 \
    friend struct o2::CheckDeserializeBasicOverridden;                                                                                          \
                                                                                                                                                \
    template<typename __type, typename _enable>                                                                                                 \
    friend struct o2::CheckSerializeDeltaBasicOverridden;                                                                                       \
                                                                                                                                                \
    template<typename __type, typename _enable>                                                                                                 \
    friend struct o2::CheckDeserializeDeltaBasicOverridden;                                                                                     \
                                                                                                                                                \
    void SerializeBasic(o2::DataValue& node) const override                                                                                     \
    {                                                                                                                                           \
        o2::SerializeTypeProcessor processor(node);                                                                                             \
        ProcessBaseTypes(const_cast<CLASS*>(this), processor);                                                                                  \
        ProcessFields(const_cast<CLASS*>(this), processor);                                                                                     \
    }                                                                                                                                           \
    void Serialize(o2::DataValue& node) const override                                                                                          \
    {                                                                                                                                           \
        o2::CheckSerializeBasicOverridden<CLASS>::Process(const_cast<CLASS*>(this), node);                                                      \
    }                                                                                                                                           \
    void DeserializeBasic(const o2::DataValue& node) override                                                                                   \
    {                                                                                                                                           \
        o2::DeserializeTypeProcessor processor(node);                                                                                           \
        ProcessBaseTypes(const_cast<CLASS*>(this), processor);                                                                                  \
        ProcessFields(const_cast<CLASS*>(this), processor);                                                                                     \
    }                                                                                                                                           \
    void Deserialize(const o2::DataValue& node) override                                                                                        \
    {                                                                                                                                           \
        o2::CheckDeserializeBasicOverridden<CLASS>::Process(const_cast<CLASS*>(this), node);                                                    \
    }                                                                                                                                           \
    void SerializeDeltaBasic(o2::DataValue& node, const o2::IObject& origin) const override                                                     \
    {                                                                                                                                           \
        o2::SerializeDeltaTypeProcessor<CLASS> processor(node, dynamic_cast<const CLASS&>(origin), origin);                                     \
        ProcessBaseTypes(const_cast<CLASS*>(this), processor);                                                                                  \
        ProcessFields(const_cast<CLASS*>(this), processor);                                                                                     \
    }                                                                                                                                           \
    void SerializeDelta(o2::DataValue& node, const o2::IObject& origin) const override                                                          \
    {                                                                                                                                           \
        o2::CheckSerializeDeltaBasicOverridden<CLASS>::Process(const_cast<CLASS*>(this), dynamic_cast<const CLASS*>(&origin), origin, node);    \
    }                                                                                                                                           \
    void DeserializeDeltaBasic(const o2::DataValue& node, const o2::IObject& origin) override                                                   \
    {                                                                                                                                           \
        o2::DeserializeDeltaTypeProcessor<CLASS> processor(node, dynamic_cast<const CLASS&>(origin), origin);                                   \
        ProcessBaseTypes(const_cast<CLASS*>(this), processor);                                                                                  \
        ProcessFields(const_cast<CLASS*>(this), processor);                                                                                     \
    }                                                                                                                                           \
    void DeserializeDelta(const o2::DataValue& node, const o2::IObject& origin) override                                                        \
    {                                                                                                                                           \
        o2::CheckDeserializeDeltaBasicOverridden<CLASS>::Process(const_cast<CLASS*>(this), dynamic_cast<const CLASS*>(&origin), origin, node);  \
    }                                                                                                                                           \
    CLASS& operator=(const o2::DataValue& node)                                                                                                 \
    {                                                                                                                                           \
        Deserialize(node); return *this;                                                                                                        \
    }                                                                                                                                           \
    explicit operator o2::DataDocument() const                                                                                                  \
    {                                                                                                                                           \
        o2::DataDocument doc;                                                                                                                   \
        Serialize(doc);                                                                                                                         \
        return doc;                                                                                                                             \
    }

#define SERIALIZABLE(CLASS)                                                                                     \
    SERIALIZABLE_MAIN(CLASS)                                                                                    \
    IOBJECT_SCRIPTING();                                                                                        \
                                                                                                                \
    template<typename _type_processor> static void ProcessBaseTypes(CLASS* object, _type_processor& processor); \
    template<typename _type_processor> static void ProcessFields(CLASS* object, _type_processor& processor);    \
    template<typename _type_processor> static void ProcessMethods(CLASS* object, _type_processor& processor);   

#define SERIALIZABLE_MANUAL(CLASS) \
    SERIALIZABLE(CLASS)
}

#include "o2/Utils/Serialization/SerializeFieldProcessors.h"
// --- META ---

CLASS_BASES_META(o2::ISerializable)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(o2::ISerializable)
{
}
END_META;
CLASS_METHODS_META(o2::ISerializable)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Serialize, DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, Deserialize, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeDelta, DataValue&, const IObject&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeserializeDelta, const DataValue&, const IObject&);
    FUNCTION().PUBLIC().SIGNATURE(String, SerializeToString);
    FUNCTION().PUBLIC().SIGNATURE(void, DeserializeFromString, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeBasicInternal, DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeserializeBasicInternal, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, SerializeDeltaBasicInternal, DataValue&, const IObject&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeserializeDeltaBasicInternal, const DataValue&, const IObject&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSerializeInternal, DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserializedInternal, const DataValue&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSerializeDeltaInternal, DataValue&, const IObject&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDeserializedDeltaInternal, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeBasic, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeBasic, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, SerializeDeltaBasic, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, DeserializeDeltaBasic, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
