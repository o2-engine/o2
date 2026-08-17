#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    enum class Color { Red, Green, Blue };

    enum class Flags { None = 0, First = 1, Second = 2 };

    class WithEnums: public o2::ISerializable
    {
    public:
        enum class Inner { A, B };

        Color color = Color::Red;  // @SERIALIZABLE
        Inner inner = Inner::A;    // @SERIALIZABLE

        SERIALIZABLE(WithEnums);
    };
}
// --- META ---

PRE_ENUM_META(test::Color);

PRE_ENUM_META(test::Flags);

PRE_ENUM_META(test::WithEnums::Inner);

CLASS_BASES_META(test::WithEnums)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::WithEnums)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color::Red).NAME(color);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Inner::A).NAME(inner);
}
END_META;
CLASS_METHODS_META(test::WithEnums)
{
}
END_META;
// --- END META ---
