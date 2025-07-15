#pragma once
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // ----------------------------------------------------------------
    // Asset holder, that can be edited from asset editor and previewed
    // ----------------------------------------------------------------
    class IAssetEditablePreview: public IRefCounterable
    {
    public:
        virtual ~IAssetEditablePreview() {}
        
        // Called when asset started to preview. 
        virtual void BeginPreview() = 0;

        // Called when asset finished preview
        virtual void EndPreview() = 0;
    };
}
