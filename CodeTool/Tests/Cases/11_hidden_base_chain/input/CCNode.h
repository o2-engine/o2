//@CODETOOL_NON_EXCLUDE
#pragma once
#include "o2/Utils/Basic/IObject.h"

namespace cocos2d
{
    class CC_DLL Node : public o2::IObject
    {
    public:
        int _tag = 0;

        IOBJECT(Node);
    };
}
