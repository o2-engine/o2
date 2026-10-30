#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Scene/UI/Widget.h"

using namespace o2;

namespace o2
{
    class SoundPlayer;
    class Sprite;
    class VectorSprite;
}

namespace Editor
{
    FORWARD_CLASS_REF(ModelView);

    // -------------------------------------------------------------------------------------------------------
    // Small preview of an asset: an image, a vector image or an atlas page on the checker, a font sample, a
    // model; a sound plays and stops by a click, the others open the Preview window; other assets show the
    // icon of their type
    // -------------------------------------------------------------------------------------------------------
    class AssetPreviewIcon: public Widget, public CursorAreaEventsListener
    {
    public:
        enum class Kind { None, Image, VectorImage, Atlas, Font, Model, Sound, Video, Icon };

    public:
        // Default constructor
        AssetPreviewIcon(RefCounter* refCounter);

        // Copy-constructor
        AssetPreviewIcon(RefCounter* refCounter, const AssetPreviewIcon& other);

        // Destructor, stops the sound
        ~AssetPreviewIcon();

        // Copy-operator
        AssetPreviewIcon& operator=(const AssetPreviewIcon& other);

        // Sets the asset to preview, empty for none
        void SetAsset(const AssetRef<Asset>& asset);

        // Returns the previewed asset
        const AssetRef<Asset>& GetAsset() const;

        // Returns how the asset is previewed
        Kind GetKind() const;

        // Returns the drawable of the preview, null for a model or none
        const Ref<IRectDrawable>& GetPreviewDrawable() const;

        // Returns the view of the model, null for the other assets
        const Ref<ModelView>& GetModelView() const;

        // Returns is the sound playing
        bool IsSoundPlaying() const;

        // Returns is a click opening the Preview window
        bool OpensPreview() const;

        // Does what a click does: plays or stops the sound, or opens the Preview window
        void Activate();

        // Draws the preview
        void Draw() override;

        // Updates the sound playback
        void Update(float dt) override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AssetPreviewIcon);
        CLONEABLE_REF(AssetPreviewIcon);

    protected:
        AssetRef<Asset> mAsset;            // Previewed asset
        Kind            mKind = Kind::None; // How the asset is previewed

        Ref<Sprite>        mChecker;     // Transparency checker under the images
        Ref<IRectDrawable> mPreview;     // Image, vector image, atlas page, font sample or type icon
        Vec2F              mPreviewSize; // Size the preview keeps the aspect of, zero to fill the rect
        Ref<ModelView>     mModelView;   // View of the model

        Ref<SoundPlayer>   mSoundPlayer; // Player of the sound
        Ref<IRectDrawable> mPlayCircle;  // Accent circle of the play button of a sound
        Ref<VectorSprite>  mPlayGlyph;   // Play or pause glyph of a sound

    protected:
        // Called when cursor released (only when cursor pressed this at previous time): plays the sound or opens the preview
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Removes the preview of the previous asset
        void Clear();

        // Returns the rect of the preview in the rect of the widget
        RectF GetPreviewRect() const;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

PRE_ENUM_META(Editor::AssetPreviewIcon::Kind);

CLASS_BASES_META(Editor::AssetPreviewIcon)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AssetPreviewIcon)
{
    FIELD().PROTECTED().NAME(mAsset);
    FIELD().PROTECTED().DEFAULT_VALUE(Kind::None).NAME(mKind);
    FIELD().PROTECTED().NAME(mChecker);
    FIELD().PROTECTED().NAME(mPreview);
    FIELD().PROTECTED().NAME(mPreviewSize);
    FIELD().PROTECTED().NAME(mModelView);
    FIELD().PROTECTED().NAME(mSoundPlayer);
    FIELD().PROTECTED().NAME(mPlayCircle);
    FIELD().PROTECTED().NAME(mPlayGlyph);
}
END_META;
CLASS_METHODS_META(Editor::AssetPreviewIcon)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetPreviewIcon&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAsset, const AssetRef<Asset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<Asset>&, GetAsset);
    FUNCTION().PUBLIC().SIGNATURE(Kind, GetKind);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetPreviewDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ModelView>&, GetModelView);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSoundPlaying);
    FUNCTION().PUBLIC().SIGNATURE(bool, OpensPreview);
    FUNCTION().PUBLIC().SIGNATURE(void, Activate);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, Clear);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetPreviewRect);
}
END_META;
// --- END META ---
