#include "o2Editor/stdafx.h"
#include "SpriteViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BorderIntProperty.h"
#include "o2Editor/Properties/Basic/ColorProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/ImageSlicesEditorWidget.h"

namespace Editor
{
    void SpriteViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        const Type& spriteType = TypeOf(Sprite);

        auto commonFieldsLayout = mmake<VerticalLayout>();
        commonFieldsLayout->spacing = 5;
        commonFieldsLayout->expandWidth = true;
        commonFieldsLayout->expandHeight = false;
        commonFieldsLayout->fitByChildren = true;
        mSpoiler->AddChild(commonFieldsLayout);

        mImageProperty = o2EditorProperties.BuildFieldType<AssetProperty>(commonFieldsLayout, spriteType, "image", "",
                                                                          mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mSizePivotProperty = o2EditorProperties.BuildFieldType<Vec2FProperty>(commonFieldsLayout, spriteType, "szPivot", "",
                                                                              mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mPivotProperty = o2EditorProperties.BuildFieldType<Vec2FProperty>(commonFieldsLayout, spriteType, "pivot", "",
                                                                          mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mColorProperty = o2EditorProperties.BuildFieldType<ColorProperty>(commonFieldsLayout, spriteType, "color", "",
                                                                          mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mAlphaProperty = o2EditorProperties.BuildFieldType<FloatProperty>(commonFieldsLayout, spriteType, "transparency", "",
                                                                          mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mModeProperty = o2EditorProperties.BuildFieldType<EnumProperty>(commonFieldsLayout, spriteType, "mode", "",
                                                                        mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mModeProperty->onChanged += [&](auto& x) { OnModeSelected(); };

        mHiddenProperties = o2UI.CreateWidget<VerticalLayout>();
        mHiddenProperties->expandWidth = true;
        mHiddenProperties->expandHeight = false;
        mHiddenProperties->fitByChildren = true;
        mSpoiler->AddChild(mHiddenProperties);

        // Fill properties
        mFillPropertiesSpoiler = o2UI.CreateWidget<Spoiler>();
        mHiddenProperties->AddChild(mFillPropertiesSpoiler);

        auto fillSpace = mmake<Widget>();
        fillSpace->layout->minHeight = 5;
        mFillPropertiesSpoiler->AddChildWidget(fillSpace);

        mFillProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mFillPropertiesSpoiler, spriteType, "fill", "",
                                                                         mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        // Slice properties
        mSlicedPropertiesSpoiler = o2UI.CreateWidget<Spoiler>();
        mHiddenProperties->AddChild(mSlicedPropertiesSpoiler);

        auto sliceSpace = mmake<Widget>();
        sliceSpace->layout->minHeight = 5;
        mSlicedPropertiesSpoiler->AddChildWidget(sliceSpace);

        auto slicesEditorSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");

        slicesEditorSpoiler->SetCaption("Slices");
        mSliceBorderProperty = o2EditorProperties.BuildFieldType<BorderIProperty>(slicesEditorSpoiler, spriteType, "sliceBorder", "",
                                                                                  mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);

        mSlicesEditor = mmake<ImageSlicesEditorWidget>();
        slicesEditorSpoiler->AddChildWidget(mSlicesEditor);
        mSlicedPropertiesSpoiler->AddChild(slicesEditorSpoiler);

        // Slice properties
        mTiledPropertiesSpoiler = o2UI.CreateWidget<Spoiler>();
        mHiddenProperties->AddChild(mTiledPropertiesSpoiler);

        auto tiledSpace = mmake<Widget>();
        tiledSpace->layout->minHeight = 5;
        mTiledPropertiesSpoiler->AddChildWidget(tiledSpace);

        mTileScaleProperty = o2EditorProperties.BuildFieldType<FloatProperty>(mTiledPropertiesSpoiler, spriteType, "tileScale", "",
                                                                              mPropertiesContext, mOnChildFieldChangeCompleted, onChanged);
    }

    void SpriteViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (mSlicesEditor && !targetObjets.IsEmpty())
            mSlicesEditor->Setup(dynamic_cast<Sprite*>(targetObjets[0].first)->GetImageAsset(), mSliceBorderProperty);
    }

    void SpriteViewer::OnModeSelected()
    {
        SpriteMode mode = (SpriteMode)(mModeProperty->GetCommonValue());

        mFillPropertiesSpoiler->SetExpanded(mode == SpriteMode::Fill360CCW ||
                                            mode == SpriteMode::Fill360CW ||
                                            mode == SpriteMode::FillDownToUp ||
                                            mode == SpriteMode::FillLeftToRight ||
                                            mode == SpriteMode::FillRightToLeft ||
                                            mode == SpriteMode::FillUpToDown ||
                                            mode == SpriteMode::Sliced);

        mSlicedPropertiesSpoiler->SetExpanded(mode == SpriteMode::Sliced);

        mTiledPropertiesSpoiler->SetExpanded(mode == SpriteMode::Tiled);
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<o2::Sprite>);
// --- META ---

DECLARE_CLASS(Editor::SpriteViewer, Editor__SpriteViewer);
// --- END META ---
