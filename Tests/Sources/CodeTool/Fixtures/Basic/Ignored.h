#pragma once
// @CODETOOLIGNORE

#include "TestBase.h"

namespace game
{
    class NotProcessed: public o2::IObject
    {
    public:
        int field = 1;

        IOBJECT(NotProcessed);
    };
}
