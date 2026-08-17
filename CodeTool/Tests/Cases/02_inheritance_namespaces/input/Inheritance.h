#pragma once
#include "o2/Utils/Basic/IObject.h"

namespace outer
{
    namespace inner
    {
        class Base: public o2::IObject
        {
        public:
            IOBJECT(Base);

            int baseField = 1;
        };
    }

    class Derived: public inner::Base
    {
    public:
        IOBJECT(Derived);

        float derivedField = 2.0f;
    };

    class MultiDerived: public inner::Base, public o2::ISerializable
    {
    public:
        SERIALIZABLE(MultiDerived);

        bool flag = true;
    };
}
