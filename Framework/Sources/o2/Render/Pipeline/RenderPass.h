#pragma once

#include "o2/Render/Camera.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class CameraActor;
    class RenderPipeline;

    FORWARD_CLASS_REF(SceneLayer);

    // -----------------------------------------------------------------------
    // Render pass execution context: rendering camera, drawing layers list,
    // background fill, optional source camera actor and the executing pipeline
    // -----------------------------------------------------------------------
    struct RenderPassContext
    {
        Camera camera; // Rendering camera

        Vector<Ref<SceneLayer>> layers; // Scene layers to draw

        bool   fillBackground = false; // Is background filled with solid color
        Color4 fillColor;              // Background fill color

        CameraActor*    cameraActor = nullptr; // Source camera actor; null when rendering an editor preview
        RenderPipeline* pipeline = nullptr;    // Currently executing pipeline
    };

    // -----------------------------------------------------------------------
    // Render pass interface. Passes are ordered inside a RenderPipeline and
    // draw a filtered part of the scene or perform a screen-space operation
    // -----------------------------------------------------------------------
    class RenderPass: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // Virtual destructor
        virtual ~RenderPass() {}

        // Returns pass name, type name by default
        virtual String GetName() const;

        // Enables or disables pass execution
        void SetEnabled(bool enabled);

        // Returns true when pass is enabled
        bool IsEnabled() const;

        // Executes pass
        virtual void Execute(RenderPassContext& context) {}

        SERIALIZABLE(RenderPass);
        CLONEABLE_REF(RenderPass);

    protected:
        bool mEnabled = true; // Is pass enabled @SERIALIZABLE @EDITOR_PROPERTY
    };
}
// --- META ---

CLASS_BASES_META(o2::RenderPass)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::RenderPass)
{
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mEnabled);
}
END_META;
CLASS_METHODS_META(o2::RenderPass)
{

    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, Execute, RenderPassContext&);
}
END_META;
// --- END META ---
