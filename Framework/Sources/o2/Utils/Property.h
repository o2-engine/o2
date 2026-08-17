#pragma once

#include "o2/Utils/Reflection/TypeTraits.h"

namespace o2
{
#define PROPERTIES(CLASSNAME) \
    typedef CLASSNAME _propertiesClassType

#define PROPERTY(TYPE, NAME, SETTER, GETTER)                                                                                                             \
    class NAME##_PROPERTY                                                                                                                                \
    {                                                                                                                                                    \
        _propertiesClassType* GetThis() const                                                                                                            \
        {                                                                                                                                                \
            return reinterpret_cast<_propertiesClassType*>(                                                                                              \
                const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) - offsetof(_propertiesClassType, NAME));                                \
        }                                                                                                                                                \
                                                                                                                                                         \
    public:                                                                                                                                              \
        typedef TYPE valueType;                                                                                                                          \
                                                                                                                                                         \
        NAME##_PROPERTY() {}                                                                                                                             \
                                                                                                                                                         \
        operator valueType() const { return GetThis()->GETTER(); }                                                                                       \
        NAME##_PROPERTY& operator=(const valueType& value) { GetThis()->SETTER(const_cast<valueType&>(value)); return *this; }                           \
                                                                                                                                                         \
        NAME##_PROPERTY& operator=(const NAME##_PROPERTY& value) { GetThis()->SETTER(value.Get()); return *this; }                                       \
                                                                                                                                                         \
        template<typename vt, typename X = typename std::enable_if<std::is_same<vt, valueType>::value && o2::SupportsEqualOperator<valueType>::value>::type> \
        bool operator==(const vt& value) const { return o2::Math::Equals(GetThis()->GETTER(), value); }                                                      \
                                                                                                                                                         \
        template<typename vt, typename X = typename std::enable_if<std::is_same<vt, valueType>::value && o2::SupportsEqualOperator<valueType>::value>::type> \
        bool operator!=(const vt& value) const { return !o2::Math::Equals(GetThis()->GETTER(), value); }                                                     \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsPlus<valueType>::value && std::is_same<T, valueType>::value>::type>        \
        valueType operator+(const T& value) { return GetThis()->GETTER() + value; }                                                                      \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsMinus<valueType>::value && std::is_same<T, valueType>::value>::type>       \
        valueType operator-(const T& value) { return GetThis()->GETTER() - value; }                                                                      \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsDivide<valueType>::value && std::is_same<T, valueType>::value>::type>      \
        valueType operator/(const T& value) { return GetThis()->GETTER() / value; }                                                                      \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsMultiply<valueType>::value && std::is_same<T, valueType>::value>::type>    \
        valueType operator*(const T& value) { return GetThis()->GETTER() * value; }                                                                      \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsPlus<valueType>::value && std::is_same<T, valueType>::value>::type>        \
        NAME##_PROPERTY& operator+=(const T& value) { auto _this = GetThis(); _this->SETTER(_this->GETTER() + value); return *this; }                    \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsMinus<valueType>::value && std::is_same<T, valueType>::value>::type>       \
        NAME##_PROPERTY& operator-=(const T& value) { auto _this = GetThis(); _this->SETTER(_this->GETTER() - value); return *this; }                    \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsDivide<valueType>::value && std::is_same<T, valueType>::value>::type>      \
        NAME##_PROPERTY& operator/=(const T& value) { auto _this = GetThis(); _this->SETTER(_this->GETTER() / value); return *this; }                    \
                                                                                                                                                         \
        template<typename T, typename X = typename std::enable_if<o2::SupportsMultiply<valueType>::value && std::is_same<T, valueType>::value>::type>    \
        NAME##_PROPERTY& operator*=(const T& value) { auto _this = GetThis(); _this->SETTER(_this->GETTER() * value); return *this; }                    \
                                                                                                                                                         \
        valueType Get() const { return GetThis()->GETTER(); }                                                                                            \
        void Set(const valueType& value) { GetThis()->SETTER(const_cast<valueType&>(value)); }                                                           \
                                                                                                                                                         \
        o2::PropertyValueProxy<valueType, NAME##_PROPERTY> GetValueProxy() { return o2::PropertyValueProxy<valueType, NAME##_PROPERTY>(this); }                  \
                                                                                                                                                         \
        bool IsProperty() const { return true; }                                                                                                         \
    };                                                                                                                                                   \
                                                                                                                                                         \
    NAME##_PROPERTY NAME;

#define GETTER(TYPE, NAME, GETTER)                                                                                         \
    class NAME##_GET_PROPERTY                                                                                              \
    {                                                                                                                      \
        _propertiesClassType* GetThis() const                                                                              \
        {                                                                                                                  \
            return reinterpret_cast<_propertiesClassType*>(                                                                \
                const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) - offsetof(_propertiesClassType, NAME));  \
        }                                                                                                                  \
                                                                                                                           \
    public:                                                                                                                \
        typedef TYPE valueType;                                                                                            \
                                                                                                                           \
        NAME##_GET_PROPERTY() {}                                                                                           \
                                                                                                                           \
        operator valueType() { return GetThis()->GETTER(); }                                                               \
        bool operator==(const valueType& value) const { return GetThis()->GETTER() == value; }                             \
        bool operator!=(const valueType& value) const { return GetThis()->GETTER() != value; }                             \
        TYPE Get() const { return GetThis()->GETTER(); }                                                                   \
    };                                                                                                                     \
                                                                                                                           \
    NAME##_GET_PROPERTY NAME;                                                

#define SETTER(TYPE, NAME, SETTER)                                                                                         \
    class NAME##_SET_PROPERTY                                                                                              \
    {                                                                                                                      \
        _propertiesClassType* GetThis() const                                                                              \
        {                                                                                                                  \
            return reinterpret_cast<_propertiesClassType*>(                                                                \
                const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) - offsetof(_propertiesClassType, NAME));  \
        }                                                                                                                  \
                                                                                                                           \
    public:                                                                                                                \
        typedef TYPE valueType;                                                                                            \
                                                                                                                           \
        NAME##_SET_PROPERTY() {}                                                                                           \
                                                                                                                           \
        NAME##_SET_PROPERTY& operator=(const valueType& value) { GetThis()->SETTER(value); return *this; }                 \
        void Set(const valueType& value) { GetThis()->SETTER(const_cast<valueType&>(value)); }                             \
    };                                                                                                                     \
                                                                                                                           \
    NAME##_SET_PROPERTY NAME;                                                

#define ACCESSOR(TYPE, NAME, KEY_TYPE, GETTER, GET_ALL)                                                                    \
    class NAME##_ACCESSOR                                                                                                  \
    {                                                                                                                      \
        _propertiesClassType* GetThis() const                                                                              \
        {                                                                                                                  \
            return reinterpret_cast<_propertiesClassType*>(                                                                \
                const_cast<std::byte*>(reinterpret_cast<const std::byte*>(this)) - offsetof(_propertiesClassType, NAME));  \
        }                                                                                                                  \
                                                                                                                           \
    public:                                                                                                                \
        typedef TYPE     valueType;                                                                                        \
        typedef KEY_TYPE keyType;                                                                                          \
        NAME##_ACCESSOR(_propertiesClassType* _this) {}                                                                    \
        valueType Get(const keyType& key) const { return GetThis()->GETTER(key); }                                         \
        Map<keyType, TYPE> GetAll() const { return GetThis()->GET_ALL(); }                                                 \
        valueType operator[](const keyType& key) const { return GetThis()->GETTER(key); }                                  \
                                                                                                                           \
        bool IsAccessor() const { return true; }                                                                           \
    };                                                                                                                     \
                                                                                                                           \
    NAME##_ACCESSOR NAME = NAME##_ACCESSOR(this);    

}
