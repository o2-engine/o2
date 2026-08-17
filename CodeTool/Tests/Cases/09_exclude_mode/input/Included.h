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
