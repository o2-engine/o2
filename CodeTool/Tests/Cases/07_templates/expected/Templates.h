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
// --- META ---

META_TEMPLATES(typename T)
CLASS_BASES_META(test::TemplateClass<T>)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
META_TEMPLATES(typename T)
CLASS_FIELDS_META(test::TemplateClass<T>)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(value);
}
END_META;
META_TEMPLATES(typename T)
CLASS_METHODS_META(test::TemplateClass<T>)
{
}
END_META;

CLASS_BASES_META(test::UsesTemplates)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::UsesTemplates)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mapField);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(nested);
}
END_META;
CLASS_METHODS_META(test::UsesTemplates)
{
}
END_META;
// --- END META ---
