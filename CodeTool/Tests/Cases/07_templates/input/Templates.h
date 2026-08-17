#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    template<typename T>
    class TemplateClass: public o2::ISerializable
    {
    public:
        T value;  // @SERIALIZABLE

        SERIALIZABLE(TemplateClass<T>);
    };

    class UsesTemplates: public o2::ISerializable
    {
    public:
        o2::Map<o2::String, int> mapField;      // @SERIALIZABLE
        o2::Vector<o2::Vector<float>> nested;   // @SERIALIZABLE

        SERIALIZABLE(UsesTemplates);
    };
}
