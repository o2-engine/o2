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
