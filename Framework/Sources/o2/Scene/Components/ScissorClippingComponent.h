#pragma once

#include "o2/Scene/Component.h"

namespace o2
{
    // -----------------------------------------------------------------------------------------
    // Scissor clipping component. Clips children and inherited drawables by parent actor bounds
    // -----------------------------------------------------------------------------------------
    class ScissorClippingComponent : public Component
    {
    public:
        bool enableClipping = true; // Is clipping enabled @SERIALIZABLE

    public:
        // Default constructor
        ScissorClippingComponent();

        // Destructor
		~ScissorClippingComponent();

		// Returns name of component
		static String GetName();

		// Returns category of component
		static String GetCategory();

        SERIALIZABLE(ScissorClippingComponent);
        CLONEABLE_REF(ScissorClippingComponent);

    private:
        // Draws content of scene, enables scissor test and clips by actor bounds
        void OnDraw() override;

        // Draws component after all drawables, disables scissor test
        void OnPostDraw() override;
    };
}
// --- META ---

CLASS_BASES_META(o2::ScissorClippingComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::ScissorClippingComponent)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(enableClipping);
}
END_META;
CLASS_METHODS_META(o2::ScissorClippingComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDraw);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPostDraw);
}
END_META;
// --- END META ---
