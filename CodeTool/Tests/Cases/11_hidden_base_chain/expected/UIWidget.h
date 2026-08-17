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
// --- META ---

PRE_ENUM_META(cocos2d::ui::Widget::BrightStyle);

CLASS_BASES_META(cocos2d::ui::Widget)
{
    BASE_CLASS(ProtectedNode);
    BASE_CLASS(LayoutParameterProtocol);
}
END_META;
CLASS_FIELDS_META(cocos2d::ui::Widget)
{
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(_enabled);
}
END_META;
CLASS_METHODS_META(cocos2d::ui::Widget)
{
}
END_META;
// --- END META ---
