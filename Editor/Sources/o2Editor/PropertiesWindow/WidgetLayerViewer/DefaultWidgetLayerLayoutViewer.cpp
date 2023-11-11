#include "o2Editor/stdafx.h"
#include "DefaultWidgetLayerLayoutViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Core/Actions/PropertyChange.h"
#include "o2Editor/Core/EditorApplication.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/Core/UI/SpoilerWithHead.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{

	DefaultWidgetLayerLayoutViewer::DefaultWidgetLayerLayoutViewer()
	{
		PushEditorScopeOnStack scope;

		// Position
		auto positionPropertyContainer = mnew Widget();
		positionPropertyContainer->name = "position";
		positionPropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(positionPropertyContainer);

		auto positionIcon = o2UI.CreateImage("ui/UI4_position_icon.png");
		*positionIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
		positionPropertyContainer->AddChild(positionIcon);

		mPositionProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*mPositionProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		mPositionProperty->SetValuePath("transform/position");
		mPositionProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		positionPropertyContainer->AddChild(mPositionProperty);

		// Size
		auto sizePropertyContainer = mnew Widget();
		sizePropertyContainer->name = "size";
		sizePropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(sizePropertyContainer);

		auto sizeIcon = o2UI.CreateImage("ui/UI4_icon_size.png");
		*sizeIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 0));
		sizePropertyContainer->AddChild(sizeIcon);

		mSizeProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*mSizeProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		mSizeProperty->SetValuePath("transform/size");
		mSizeProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		sizePropertyContainer->AddChild(mSizeProperty);

		// Anchors
		// Right top
		auto rightTopAnchorPropertyContainer = mnew Widget();
		rightTopAnchorPropertyContainer->name = "right top anchor";
		rightTopAnchorPropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(rightTopAnchorPropertyContainer);

		auto anchorIcon = o2UI.CreateImage("ui/UI4_icon_anchor.png");
		*anchorIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
		rightTopAnchorPropertyContainer->AddChild(anchorIcon);

		mAnchorRightTopProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*mAnchorRightTopProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		mAnchorRightTopProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "R";
		mAnchorRightTopProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "T";
		mAnchorRightTopProperty->SetValuePath("layout/anchorMax");
		mAnchorRightTopProperty->onChanged = THIS_FUNC(OnPropertyChanged);
		mAnchorRightTopProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		rightTopAnchorPropertyContainer->AddChild(mAnchorRightTopProperty);

		// Left bottom
		auto leftBottomAnchorPropertyContainer = mnew Widget();
		leftBottomAnchorPropertyContainer->name = "left bottom anchor";
		leftBottomAnchorPropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(leftBottomAnchorPropertyContainer);

		mAnchorLeftBottomProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*mAnchorLeftBottomProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		mAnchorLeftBottomProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "L";
		mAnchorLeftBottomProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "B";
		mAnchorLeftBottomProperty->SetValuePath("layout/anchorMin");
		mAnchorLeftBottomProperty->onChanged = THIS_FUNC(OnPropertyChanged);
		mAnchorLeftBottomProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		leftBottomAnchorPropertyContainer->AddChild(mAnchorLeftBottomProperty);

		// Offsets
		// Right top
		auto rightTopOffsetPropertyContainer = mnew Widget();
		rightTopOffsetPropertyContainer->name = "right top offset";
		rightTopOffsetPropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(rightTopOffsetPropertyContainer);

		auto offsetIcon = o2UI.CreateImage("ui/UI4_icon_offsets.png");
		*offsetIcon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 0));
		rightTopOffsetPropertyContainer->AddChild(offsetIcon);

		moffsetRightTopProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*moffsetRightTopProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		moffsetRightTopProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "R";
		moffsetRightTopProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "T";
		moffsetRightTopProperty->SetValuePath("layout/offsetMax");
		moffsetRightTopProperty->onChanged = THIS_FUNC(OnPropertyChanged);
		moffsetRightTopProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		rightTopOffsetPropertyContainer->AddChild(moffsetRightTopProperty);

		// Left bottom
		auto leftBottomOffsetPropertyContainer = mnew Widget();
		leftBottomOffsetPropertyContainer->name = "left bottom offset";
		leftBottomOffsetPropertyContainer->layout->minHeight = 20;
		mSpoiler->AddChild(leftBottomOffsetPropertyContainer);

		mOffsetLeftBottomProperty = o2UI.CreateWidget<Vec2FProperty>("colored");
		*mOffsetLeftBottomProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20, 0);
		mOffsetLeftBottomProperty->GetChildByType<Label>("container/layout/properties/x label")->text = "L";
		mOffsetLeftBottomProperty->GetChildByType<Label>("container/layout/properties/y label")->text = "B";
		mOffsetLeftBottomProperty->SetValuePath("layout/offsetMin");
		mOffsetLeftBottomProperty->onChanged = THIS_FUNC(OnPropertyChanged);
		mOffsetLeftBottomProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
		leftBottomOffsetPropertyContainer->AddChild(mOffsetLeftBottomProperty);
	}

	DefaultWidgetLayerLayoutViewer::~DefaultWidgetLayerLayoutViewer()
	{
		delete mSpoiler;
	}

	void DefaultWidgetLayerLayoutViewer::SetTargetLayers(const Vector<WidgetLayer*>& layers)
	{
		mLayers = layers;

		auto prototypes = layers.Convert<WidgetLayer*>([](WidgetLayer* x) { return nullptr; });

		mPositionProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::position)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.position; });

		mSizeProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::size)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.size; });

		mAnchorRightTopProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::anchorMax)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.anchorMax; });

		mAnchorLeftBottomProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::anchorMin)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.anchorMin; });

		moffsetRightTopProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::offsetMax)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.offsetMax; });

		mOffsetLeftBottomProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayerLayout::offsetMin)>(
			layers, prototypes, [](WidgetLayer* x) { return &x->layout.offsetMin; });
	}

	void DefaultWidgetLayerLayoutViewer::Refresh()
	{
		mAnchorRightTopProperty->Refresh();
		mAnchorLeftBottomProperty->Refresh();
		moffsetRightTopProperty->Refresh();
		mOffsetLeftBottomProperty->Refresh();
	}

	void DefaultWidgetLayerLayoutViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& prevValue,
														   const Vector<DataDocument>& newValue)
	{
		PropertyChangeAction* action = mnew PropertyChangeAction(
			o2EditorSceneScreen.GetSelectedObjects(), path, prevValue, newValue);

		o2EditorApplication.DoneAction(action);
	}

	void DefaultWidgetLayerLayoutViewer::OnPropertyChanged(IPropertyField* field)
	{
		for (auto layer : mLayers) {
			layer->GetOwnerWidget()->UpdateTransform();
			layer->GetOwnerWidget()->OnChanged();
		}
	}

}
// --- META ---

DECLARE_CLASS(Editor::DefaultWidgetLayerLayoutViewer, Editor__DefaultWidgetLayerLayoutViewer);
// --- END META ---
