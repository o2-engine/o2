#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Sprite.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class Spoiler;
}

namespace Editor
{
    FORWARD_CLASS_REF(AssetProperty);
    FORWARD_CLASS_REF(BorderIProperty);
    FORWARD_CLASS_REF(ColorProperty);
    FORWARD_CLASS_REF(EnumProperty);
    FORWARD_CLASS_REF(FloatProperty);
    FORWARD_CLASS_REF(ImageSlicesEditorWidget);
    FORWARD_CLASS_REF(Vec2FProperty);

    // --------------------
    // Editor sprite viewer
    // --------------------
    class SpriteViewer : public TObjectPropertiesViewer<Sprite>
    {
    public:
        IOBJECT(SpriteViewer);

    protected:
        Ref<VerticalLayout> mHiddenProperties; // Hidden properties container

        Ref<AssetProperty> mImageProperty;     // Image asset property
        Ref<AssetProperty> mMaterialProperty;  // Material asset property
        Ref<ColorProperty> mColorProperty;     // Color property
        Ref<FloatProperty> mAlphaProperty;     // Alpha property
        Ref<Vec2FProperty> mSizePivotProperty; // Size pivot property
        Ref<Vec2FProperty> mPivotProperty;     // Pivot property
        Ref<EnumProperty>  mModeProperty;      // Mode property selector. Shows required property spoiler

        Ref<Spoiler>       mFillPropertiesSpoiler; // Fill properties spoiler
        Ref<FloatProperty> mFillProperty;          // Fill property

        Ref<Spoiler>                 mSlicedPropertiesSpoiler; // Sliced properties spoiler
        Ref<BorderIProperty>         mSliceBorderProperty;     // Slice border property
        Ref<ImageSlicesEditorWidget> mSlicesEditor;            // Image slices editor widget

        Ref<Spoiler>       mTiledPropertiesSpoiler; // Tiled properties spoiler
        Ref<FloatProperty> mTileScaleProperty;      // Tile scale property

    protected:
        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

        // Called when type enum selected, shows required property spoiler
        void OnModeSelected();
    };
}
// --- META ---

CLASS_BASES_META(Editor::SpriteViewer)
{
    BASE_CLASS(Editor::TObjectPropertiesViewer<Sprite>);
}
END_META;
CLASS_FIELDS_META(Editor::SpriteViewer)
{
    FIELD().PROTECTED().NAME(mHiddenProperties);
    FIELD().PROTECTED().NAME(mImageProperty);
    FIELD().PROTECTED().NAME(mMaterialProperty);
    FIELD().PROTECTED().NAME(mColorProperty);
    FIELD().PROTECTED().NAME(mAlphaProperty);
    FIELD().PROTECTED().NAME(mSizePivotProperty);
    FIELD().PROTECTED().NAME(mPivotProperty);
    FIELD().PROTECTED().NAME(mModeProperty);
    FIELD().PROTECTED().NAME(mFillPropertiesSpoiler);
    FIELD().PROTECTED().NAME(mFillProperty);
    FIELD().PROTECTED().NAME(mSlicedPropertiesSpoiler);
    FIELD().PROTECTED().NAME(mSliceBorderProperty);
    FIELD().PROTECTED().NAME(mSlicesEditor);
    FIELD().PROTECTED().NAME(mTiledPropertiesSpoiler);
    FIELD().PROTECTED().NAME(mTileScaleProperty);
}
END_META;
CLASS_METHODS_META(Editor::SpriteViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;

    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, OnModeSelected);
}
END_META;
// --- END META ---
