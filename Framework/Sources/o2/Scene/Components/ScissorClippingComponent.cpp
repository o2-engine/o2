#include "o2/stdafx.h"
#include "ScissorClippingComponent.h"

namespace o2
{

    ScissorClippingComponent::ScissorClippingComponent()
    {}

    ScissorClippingComponent::~ScissorClippingComponent()
    {}

	String ScissorClippingComponent::GetName()
	{
        return "Scissor clipping";
	}

	String ScissorClippingComponent::GetCategory()
	{
        return "Render";
	}

	void ScissorClippingComponent::OnDraw()
    {
        if (enableClipping)
            o2Render.EnableScissorTest(mOwner.Lock()->transform->GetWorldAxisAlignedRect());
    }

    void ScissorClippingComponent::OnPostDraw()
    {
        if (enableClipping)
            o2Render.DisableScissorTest();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ScissorClippingComponent>);
// --- META ---

DECLARE_CLASS(o2::ScissorClippingComponent, o2__ScissorClippingComponent);
// --- END META ---
