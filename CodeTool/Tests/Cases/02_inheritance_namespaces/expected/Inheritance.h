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
// --- META ---

CLASS_BASES_META(outer::Derived)
{
    BASE_CLASS(outer::inner::Base);
}
END_META;
CLASS_FIELDS_META(outer::Derived)
{
    FIELD().PUBLIC().DEFAULT_VALUE(2.0f).NAME(derivedField);
}
END_META;
CLASS_METHODS_META(outer::Derived)
{
}
END_META;

CLASS_BASES_META(outer::MultiDerived)
{
    BASE_CLASS(outer::inner::Base);
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(outer::MultiDerived)
{
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(flag);
}
END_META;
CLASS_METHODS_META(outer::MultiDerived)
{
}
END_META;

CLASS_BASES_META(outer::inner::Base)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(outer::inner::Base)
{
    FIELD().PUBLIC().DEFAULT_VALUE(1).NAME(baseField);
}
END_META;
CLASS_METHODS_META(outer::inner::Base)
{
}
END_META;
// --- END META ---
