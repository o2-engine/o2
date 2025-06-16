#include "TextSymbolComponent.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"

namespace o2
{
    TextSymbolComponent::TextSymbolComponent()
    {}

    TextSymbolComponent::TextSymbolComponent(const TextSymbolComponent& other):
        ImageComponent(other)
    {}

	void TextSymbolComponent::SetTextureAndRect(const TextureRef& texture, const RectF& rect)
	{
		Vec2F textureSize = texture->GetSize();
		SetTexture(texture);
		SetTextureSrcRect(RectF(rect.left* textureSize.x, rect.top*textureSize.y, 
								rect.right*textureSize.x, rect.bottom*textureSize.y));
	}

	String TextSymbolComponent::GetName()
    {
	    return "Text symbol";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::TextSymbolComponent>);
// --- META ---

DECLARE_CLASS(o2::TextSymbolComponent, o2__TextSymbolComponent);
// --- END META ---
