#pragma once
#include "o2/Scene/Actor.h"
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

        // Returns actor that is being previewed
        virtual Ref<Actor> GetPreviewActor() const { return nullptr; }
    };

	// ----------------------------------------------------------------
	// Animation asset editable preview interface, that can be used to 
	// preview animation assets. Also can return animation player
	// ----------------------------------------------------------------
    class AnimationAssetEditablePreview : public IAssetEditablePreview
    {
    public:
		// Returns animation player
		virtual Ref<IAnimation> GetPreviewPlayer() const { return nullptr; }
    };
}
