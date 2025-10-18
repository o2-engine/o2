#include "o2Editor/stdafx.h"
#include "DefaultActorTransformViewer.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/UI/SpoilerWithHead.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    DefaultActorTransformViewer::DefaultActorTransformViewer()
    {
        PushEditorScopeOnStack scope;

        // Position
        auto positionPropertyContainer = mmake<Widget>();
        positionPropertyContainer->name = "position";
        positionPropertyContainer->layout->minHeight = 20;
        mSpoiler->AddChild(positionPropertyContainer);

        auto positionIcon = o2UI.CreateImage("ui/UI4_position_icon.png");
        *positionIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        positionPropertyContainer->AddChild(positionIcon);

        mPositionProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mPositionProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mPositionProperty->GetXProperty()->SetValuePath("transform/positionX");
        mPositionProperty->GetYProperty()->SetValuePath("transform/positionY");
        mPositionProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mPositionProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mPositionProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mPositionProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        positionPropertyContainer->AddChild(mPositionProperty);

        // Pivot
        auto pivotPropertyContainer = mmake<Widget>();
        pivotPropertyContainer->name = "pivot";
        pivotPropertyContainer->layout->minHeight = 20;
        mSpoiler->AddChild(pivotPropertyContainer);

        auto pivotIcon = o2UI.CreateImage("ui/UI4_pivot_icon.png");
        *pivotIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        pivotPropertyContainer->AddChild(pivotIcon);

        mPivotProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mPivotProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mPivotProperty->GetXProperty()->SetValuePath("transform/pivotX");
        mPivotProperty->GetYProperty()->SetValuePath("transform/pivotY");
        mPivotProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mPivotProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mPivotProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mPivotProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        pivotPropertyContainer->AddChild(mPivotProperty);

        // Size
        auto sizePropertyContainer = mmake<Widget>();
        sizePropertyContainer->name = "size";
        sizePropertyContainer->layout->minHeight = 20;
        mSpoiler->AddChild(sizePropertyContainer);

        auto sizeIcon = o2UI.CreateImage("ui/UI4_icon_size.png");
        *sizeIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 0));
        sizePropertyContainer->AddChild(sizeIcon);

        mSizeProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mSizeProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mSizeProperty->GetXProperty()->SetValuePath("transform/width");
        mSizeProperty->GetYProperty()->SetValuePath("transform/height");
        mSizeProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mSizeProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mSizeProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mSizeProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        sizePropertyContainer->AddChild(mSizeProperty);

        // Scale
        auto scalePropertyContainer = mmake<Widget>();
        scalePropertyContainer->name = "scale";
        scalePropertyContainer->layout->minHeight = 20;
        mSpoiler->AddChild(scalePropertyContainer);

        auto scaleIcon = o2UI.CreateImage("ui/UI4_scale_icon.png");
        *scaleIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        scalePropertyContainer->AddChild(scaleIcon);

        mScaleProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mScaleProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mScaleProperty->GetXProperty()->SetValuePath("transform/scaleX");
        mScaleProperty->GetYProperty()->SetValuePath("transform/scaleY");
        mScaleProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mScaleProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mScaleProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mScaleProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        scalePropertyContainer->AddChild(mScaleProperty);

        // Rotation
        auto rotationAndShearPropertyContainer = mmake<Widget>();
        rotationAndShearPropertyContainer->name = "rotation and depth";
        rotationAndShearPropertyContainer->layout->minHeight = 20;
        mSpoiler->AddChild(rotationAndShearPropertyContainer);

        auto rotateIcon = o2UI.CreateImage("ui/UI4_rotate_icon.png");
        *rotateIcon->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.0f, 1.0f), Vec2F(0, 0), Vec2F(20, 0));
        rotationAndShearPropertyContainer->AddChild(rotateIcon);

        mRotationProperty = o2UI.CreateWidget<FloatProperty>();
        *mRotationProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.5f, 1.0f), Vec2F(40, 0), Vec2F(10, 0));
        mRotationProperty->SetValuePath("transform/angleDegree");
        mRotationProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mRotationProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        rotationAndShearPropertyContainer->AddChild(mRotationProperty);

        // Shear
        auto shearIcon = o2UI.CreateImage("ui/UI4_shear_icon.png");
        *shearIcon->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 1.0f), Vec2F(10, 0), Vec2F(30, 0));
        rotationAndShearPropertyContainer->AddChild(shearIcon);

        mShearProperty = o2UI.CreateWidget<FloatProperty>();
        *mShearProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(1, 1.0f), Vec2F(30, 0), Vec2F());
        mShearProperty->SetValuePath("drawDepth");
        mShearProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mShearProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        rotationAndShearPropertyContainer->AddChild(mShearProperty);

        // Layout
        mLayoutSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
        mLayoutSpoiler->name = "Layout";
        mLayoutSpoiler->caption = "Layout";
        mLayoutSpoiler->spacing = 5;
        mLayoutSpoiler->fitByChildren = true;
        mLayoutSpoiler->expandWidth = true;
        mLayoutSpoiler->expandHeight = false;
        mSpoiler->AddChild(mLayoutSpoiler);

        // Anchors
        // Right top
        auto rightTopAnchorPropertyContainer = mmake<Widget>();
        rightTopAnchorPropertyContainer->name = "right top anchor";
        rightTopAnchorPropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(rightTopAnchorPropertyContainer);

        auto anchorIcon = o2UI.CreateImage("ui/UI4_icon_anchor.png");
        *anchorIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        rightTopAnchorPropertyContainer->AddChild(anchorIcon);

        mAnchorRightTopProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mAnchorRightTopProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mAnchorRightTopProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "R";
        mAnchorRightTopProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "T";
        mAnchorRightTopProperty->GetXProperty()->SetValuePath("layout/anchorRight");
        mAnchorRightTopProperty->GetYProperty()->SetValuePath("layout/anchorTop");
        mAnchorRightTopProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mAnchorRightTopProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mAnchorRightTopProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mAnchorRightTopProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        rightTopAnchorPropertyContainer->AddChild(mAnchorRightTopProperty);

        // Left bottom
        auto leftBottomAnchorPropertyContainer = mmake<Widget>();
        leftBottomAnchorPropertyContainer->name = "left bottom anchor";
        leftBottomAnchorPropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(leftBottomAnchorPropertyContainer);

        mAnchorLeftBottomProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mAnchorLeftBottomProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mAnchorLeftBottomProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "L";
        mAnchorLeftBottomProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "B";
        mAnchorLeftBottomProperty->GetXProperty()->SetValuePath("layout/anchorLeft");
        mAnchorLeftBottomProperty->GetYProperty()->SetValuePath("layout/anchorBottom");
        mAnchorLeftBottomProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mAnchorLeftBottomProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mAnchorLeftBottomProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mAnchorLeftBottomProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        leftBottomAnchorPropertyContainer->AddChild(mAnchorLeftBottomProperty);

        // Offsets
        // Right top
        auto rightTopOffsetPropertyContainer = mmake<Widget>();
        rightTopOffsetPropertyContainer->name = "right top offset";
        rightTopOffsetPropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(rightTopOffsetPropertyContainer);

        auto offsetIcon = o2UI.CreateImage("ui/UI4_icon_offsets.png");
        *offsetIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        rightTopOffsetPropertyContainer->AddChild(offsetIcon);

        mOffsetRightTopProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mOffsetRightTopProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mOffsetRightTopProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "R";
        mOffsetRightTopProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "T";
        mOffsetRightTopProperty->GetXProperty()->SetValuePath("layout/offsetRight");
        mOffsetRightTopProperty->GetYProperty()->SetValuePath("layout/offsetTop");
        mOffsetRightTopProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mOffsetRightTopProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mOffsetRightTopProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mOffsetRightTopProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        rightTopOffsetPropertyContainer->AddChild(mOffsetRightTopProperty);

        // Left bottom
        auto leftBottomOffsetPropertyContainer = mmake<Widget>();
        leftBottomOffsetPropertyContainer->name = "left bottom offset";
        leftBottomOffsetPropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(leftBottomOffsetPropertyContainer);

        mOffsetLeftBottomProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mOffsetLeftBottomProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mOffsetLeftBottomProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "L";
        mOffsetLeftBottomProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "B";
        mOffsetLeftBottomProperty->GetXProperty()->SetValuePath("layout/offsetLeft");
        mOffsetLeftBottomProperty->GetYProperty()->SetValuePath("layout/offsetBottom");
        mOffsetLeftBottomProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mOffsetLeftBottomProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mOffsetLeftBottomProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mOffsetLeftBottomProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        leftBottomOffsetPropertyContainer->AddChild(mOffsetLeftBottomProperty);

        // Min size
        auto minSizePropertyContainer = mmake<Widget>();
        minSizePropertyContainer->name = "right top anchor";
        minSizePropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(minSizePropertyContainer);

        auto minSizeIcon = o2UI.CreateImage("ui/UI4_icon_min_size.png");
        *minSizeIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        minSizePropertyContainer->AddChild(minSizeIcon);

        mMinSizeProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mMinSizeProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mMinSizeProperty->GetXProperty()->SetValuePath("layout/minWidth");
        mMinSizeProperty->GetYProperty()->SetValuePath("layout/minHeight");
        mMinSizeProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mMinSizeProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mMinSizeProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mMinSizeProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        minSizePropertyContainer->AddChild(mMinSizeProperty);

        // Max size
        auto maxSizePropertyContainer = mmake<Widget>();
        maxSizePropertyContainer->name = "right top anchor";
        maxSizePropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(maxSizePropertyContainer);

        auto maxSizeIcon = o2UI.CreateImage("ui/UI4_icon_max_size.png");
        *maxSizeIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        maxSizePropertyContainer->AddChild(maxSizeIcon);

        mMaxSizeProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mMaxSizeProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mMaxSizeProperty->GetXProperty()->SetValuePath("layout/maxWidth");
        mMaxSizeProperty->GetYProperty()->SetValuePath("layout/maxHeight");
        mMaxSizeProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mMaxSizeProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mMaxSizeProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mMaxSizeProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        maxSizePropertyContainer->AddChild(mMaxSizeProperty);

        // Weight
        auto weightPropertyContainer = mmake<Widget>();
        weightPropertyContainer->name = "right top anchor";
        weightPropertyContainer->layout->minHeight = 20;
        mLayoutSpoiler->AddChild(weightPropertyContainer);

        auto weightIcon = o2UI.CreateImage("ui/UI4_icon_weight.png");
        *weightIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
        weightPropertyContainer->AddChild(weightIcon);

        mWeightProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
        *mWeightProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
        mWeightProperty->GetXProperty()->SetValuePath("layout/widthWeight");
        mWeightProperty->GetYProperty()->SetValuePath("layout/heightWeight");
        mWeightProperty->GetXProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mWeightProperty->GetYProperty()->onChanged = THIS_FUNC(OnPropertyChanged);
        mWeightProperty->GetXProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mWeightProperty->GetYProperty()->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        weightPropertyContainer->AddChild(mWeightProperty);
    }

    DefaultActorTransformViewer::~DefaultActorTransformViewer()
    {}

    void DefaultActorTransformViewer::SetTargetActors(const Vector<Actor*>& actors)
    {
        mTargetActors = actors;

        auto prototypes = actors.Convert<Actor*>([](Actor* x) { return x->GetPrototypeLink().Get(); });


        mPositionProperty->GetXProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::positionX)>(
            actors, prototypes, [](Actor* x) { return &x->transform->positionX; });

        mPositionProperty->GetYProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::positionY)>(
            actors, prototypes, [](Actor* x) { return &x->transform->positionY; });

        mPivotProperty->GetXProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::pivotX)>(
            actors, prototypes, [](Actor* x) { return &x->transform->pivotX; });

        mPivotProperty->GetYProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::pivotY)>(
            actors, prototypes, [](Actor* x) { return &x->transform->pivotY; });

        mScaleProperty->GetXProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::scaleX)>(
            actors, prototypes, [](Actor* x) { return &x->transform->scaleX; });

        mScaleProperty->GetYProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::scaleY)>(
            actors, prototypes, [](Actor* x) { return &x->transform->scaleY; });

        mSizeProperty->GetXProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::width)>(
            actors, prototypes, [](Actor* x) { return &x->transform->width; });

        mSizeProperty->GetYProperty()->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::height)>(
            actors, prototypes, [](Actor* x) { return &x->transform->height; });

        mRotationProperty->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::angleDegree)>(
            actors, prototypes, [](Actor* x) { return &x->transform->angleDegree; });

        mShearProperty->SelectValueAndPrototypeProperties<Actor, decltype(ActorTransform::shear)>(
            actors, prototypes, [](Actor* x) { return &x->transform->shear; });

        Vector<Widget*> targetWidgets = mTargetActors
            .FindAll([](Actor* x) { return dynamic_cast<Widget*>(x) != nullptr; })
            .Convert<Widget*>([](Actor* x) { return dynamic_cast<Widget*>(x); });

        mLayoutEnabled = !targetWidgets.IsEmpty();
        mLayoutSpoiler->enabled = mLayoutEnabled;

        if (mLayoutEnabled)
        {
            auto widgetPrototypes = targetWidgets.Convert<Widget*>(
                [](Actor* x) { return dynamic_cast<Widget*>(x->GetPrototypeLink().Get()); });

            mAnchorRightTopProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::anchorRight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->anchorRight; });

            mAnchorRightTopProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::anchorTop)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->anchorTop; });

            mAnchorLeftBottomProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::anchorLeft)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->anchorLeft; });

            mAnchorLeftBottomProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::anchorBottom)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->anchorBottom; });

            mOffsetRightTopProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::offsetRight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->offsetRight; });

            mOffsetRightTopProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::offsetTop)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->offsetTop; });

            mOffsetLeftBottomProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::offsetLeft)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->offsetLeft; });

            mOffsetLeftBottomProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::offsetBottom)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->offsetBottom; });

            mMinSizeProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::minWidth)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->minWidth; });

            mMinSizeProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::minHeight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->minHeight; });

            mMaxSizeProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::maxWidth)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->maxWidth; });

            mMaxSizeProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::maxHeight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->maxHeight; });

            mWeightProperty->GetXProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::widthWeight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->widthWeight; });

            mWeightProperty->GetYProperty()->SelectValueAndPrototypeProperties<Widget, decltype(WidgetLayout::heightWeight)>(
                targetWidgets, widgetPrototypes, [](Widget* x) { return &x->layout->heightWeight; });
        }
    }

    void DefaultActorTransformViewer::Refresh()
    {
        mPositionProperty->Refresh();
        mPivotProperty->Refresh();
        mScaleProperty->Refresh();
        mSizeProperty->Refresh();
        mRotationProperty->Refresh();
        mShearProperty->Refresh();

        if (mLayoutEnabled)
        {
            mAnchorRightTopProperty->Refresh();
            mAnchorLeftBottomProperty->Refresh();
            mOffsetRightTopProperty->Refresh();
            mOffsetLeftBottomProperty->Refresh();
            mMinSizeProperty->Refresh();
            mMaxSizeProperty->Refresh();
            mWeightProperty->Refresh();
        }
    }

    void DefaultActorTransformViewer::OnPropertiesEnabled()
    {
        mPositionProperty->SetPropertyEnabled(true);
        mPivotProperty->SetPropertyEnabled(true);
        mScaleProperty->SetPropertyEnabled(true);
        mSizeProperty->SetPropertyEnabled(true);
        mRotationProperty->SetPropertyEnabled(true);
        mShearProperty->SetPropertyEnabled(true);
        mAnchorRightTopProperty->SetPropertyEnabled(true);
        mAnchorLeftBottomProperty->SetPropertyEnabled(true);
        mOffsetRightTopProperty->SetPropertyEnabled(true);
        mOffsetLeftBottomProperty->SetPropertyEnabled(true);
        mMinSizeProperty->SetPropertyEnabled(true);
        mMaxSizeProperty->SetPropertyEnabled(true);
        mWeightProperty->SetPropertyEnabled(true);
    }

    void DefaultActorTransformViewer::OnPropertiesDisabled()
    {
        mPositionProperty->SetPropertyEnabled(false);
        mPivotProperty->SetPropertyEnabled(false);
        mScaleProperty->SetPropertyEnabled(false);
        mSizeProperty->SetPropertyEnabled(false);
        mRotationProperty->SetPropertyEnabled(false);
        mShearProperty->SetPropertyEnabled(false);
        mAnchorRightTopProperty->SetPropertyEnabled(false);
        mAnchorLeftBottomProperty->SetPropertyEnabled(false);
        mOffsetRightTopProperty->SetPropertyEnabled(false);
        mOffsetLeftBottomProperty->SetPropertyEnabled(false);
        mMinSizeProperty->SetPropertyEnabled(false);
        mMaxSizeProperty->SetPropertyEnabled(false);
        mWeightProperty->SetPropertyEnabled(false);
    }

    void DefaultActorTransformViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        for (auto& actor : mTargetActors)
            actor->UpdateSelfTransform();

        IActorTransformViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultActorTransformViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IActorTransformViewer::OnPropertyChangeCompleted(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::DefaultActorTransformViewer, Editor__DefaultActorTransformViewer);
// --- END META ---
