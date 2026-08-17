// Deliberately not marked with CODETOOL_NON_EXCLUDE: in exclude mode this file
// is invisible to the tool, so the base chain Widget -> ProtectedNode -> Node
// cannot be followed
#pragma once
#include "CCNode.h"

namespace cocos2d
{
    class CC_DLL ProtectedNode : public Node
    {
    public:
        int _protectedTag = 0;
    };
}
