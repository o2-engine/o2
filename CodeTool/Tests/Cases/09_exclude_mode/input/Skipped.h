#pragma once
#include "o2/Utils/Basic/IObject.h"

namespace test
{
    class Skipped: public o2::IObject
    {
    public:
        IOBJECT(Skipped);

        int field = 1;
    };
}
