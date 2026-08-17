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
// --- META ---

CLASS_BASES_META(cocos2d::Node)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(cocos2d::Node)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(_tag);
}
END_META;
CLASS_METHODS_META(cocos2d::Node)
{
}
END_META;
// --- END META ---
