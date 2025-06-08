#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Render/TextureRef.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class Label;
}

namespace Editor
{
    // ----------------------
    // Texture preview widget
    // ----------------------
    class TexturePreview: public Widget
    {
    public:
        // Default constructor. Initializes image
        TexturePreview(RefCounter* refCounter);

        // Default copy-constructor
        TexturePreview(RefCounter* refCounter, const TexturePreview& other);

        // Copy operator
        TexturePreview& operator=(const TexturePreview& other);

        // Sets viewing curve
        void SetTexture(const TextureRef& texture, const RectI& srcRect = RectI());

        SERIALIZABLE(TexturePreview);
        CLONEABLE_REF(TexturePreview);

    protected:
        const float mLabelHeight = 20.0f; // Height of the label with texture info

        Ref<Sprite> mBackgroundSprite; // Background sprite (chessmate pattern)
        Ref<Sprite> mTextureSprite;    // Texture sprite
        Ref<Label>  mTextureInfoLabel; // Texture info label

    protected:
        // Updates layers layouts, calls after updating widget layout; calculates sprite size
        void UpdateLayersLayouts() override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::TexturePreview)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::TexturePreview)
{
    FIELD().PROTECTED().DEFAULT_VALUE(20.0f).NAME(mLabelHeight);
    FIELD().PROTECTED().NAME(mBackgroundSprite);
    FIELD().PROTECTED().NAME(mTextureSprite);
    FIELD().PROTECTED().NAME(mTextureInfoLabel);
}
END_META;
CLASS_METHODS_META(Editor::TexturePreview)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TexturePreview&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTexture, const TextureRef&, const RectI&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
}
END_META;
// --- END META ---
