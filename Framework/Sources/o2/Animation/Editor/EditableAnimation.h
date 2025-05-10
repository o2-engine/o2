#pragma once
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // ---------------------------------------------------
    // Animation, that can be edited from animation editor
    // ---------------------------------------------------
    class IEditableAnimation: public IRefCounterable
    {
    public:
        virtual ~IEditableAnimation() {}
        
        // Called when animation started to edit. It means that animation must be deactivated
        virtual void BeginAnimationEdit() = 0;

        // Called when animation finished editing. ANimation must be reactivated
        virtual void EndAnimationEdit() = 0;
    };
}
