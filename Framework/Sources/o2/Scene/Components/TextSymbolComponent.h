#pragma once

#include "ImageComponent.h"

namespace o2
{
    class TextSymbolComponent: public ImageComponent
    {
    public:
		// Default constructor
        TextSymbolComponent();

		// Copy constructor
        TextSymbolComponent(const TextSymbolComponent& other);

        // Sets texture coordinates for symbol
        void SetTextureAndRect(const TextureRef& texture, const RectF& rect);

        // Returns name of component
        static String GetName();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(TextSymbolComponent);
        CLONEABLE_REF(TextSymbolComponent);
    };
}
// --- META ---

CLASS_BASES_META(o2::TextSymbolComponent)
{
    BASE_CLASS(o2::ImageComponent);
}
END_META;
CLASS_FIELDS_META(o2::TextSymbolComponent)
{
}
END_META;
CLASS_METHODS_META(o2::TextSymbolComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const TextSymbolComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTextureAndRect, const TextureRef&, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
}
END_META;
// --- END META ---
