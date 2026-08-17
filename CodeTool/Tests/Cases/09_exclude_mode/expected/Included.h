//@CODETOOL_NON_EXCLUDE
#pragma once
#include "o2/Utils/Basic/IObject.h"

namespace test
{
    class Included: public o2::IObject
    {
    public:
        IOBJECT(Included);

        int field = 1;
    };
}
// --- META ---

CLASS_BASES_META(test::Included)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(test::Included)
{
    FIELD().PUBLIC().DEFAULT_VALUE(1).NAME(field);
}
END_META;
CLASS_METHODS_META(test::Included)
{
}
END_META;
// --- END META ---
