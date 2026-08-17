//@CODETOOL_NON_EXCLUDE
#pragma once
#include "CCProtectedNode.h"

namespace cocos2d
{
namespace ui
{
    class CC_GUI_DLL Widget : public ProtectedNode, public LayoutParameterProtocol
    {
    public:
        enum class BrightStyle
        {
            NONE = -1,
            NORMAL,
            HIGHLIGHT
        };

        bool _enabled = true;

        IOBJECT(Widget);
    };
}
}
