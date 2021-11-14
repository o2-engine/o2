#include "o2Editor/stdafx.h"
#include "EditorUIStyle.h"

#include "o2/Animation/Animate.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Animation/Tracks/AnimationColor4Track.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2Editor/AnimationWindow/PropertiesListDlg.h"
#include "o2Editor/AnimationWindow/Tree.h"
#include "o2Editor/AssetsWindow/AssetIcon.h"
#include "o2Editor/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Core/Properties/Basic/ActorProperty.h"
#include "o2Editor/Core/Properties/Basic/AssetProperty.h"
#include "o2Editor/Core/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Core/Properties/Basic/BorderFloatProperty.h"
#include "o2Editor/Core/Properties/Basic/BorderIntProperty.h"
#include "o2Editor/Core/Properties/Basic/ColorProperty.h"
#include "o2Editor/Core/Properties/Basic/ComponentProperty.h"
#include "o2Editor/Core/Properties/Basic/CurveProperty.h"
#include "o2Editor/Core/Properties/Basic/EnumProperty.h"
#include "o2Editor/Core/Properties/Basic/FloatProperty.h"
#include "o2Editor/Core/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Core/Properties/Basic/LayerProperty.h"
#include "o2Editor/Core/Properties/Basic/RectangleFloatProperty.h"
#include "o2Editor/Core/Properties/Basic/RectangleIntProperty.h"
#include "o2Editor/Core/Properties/Basic/SceneLayersListProperty.h"
#include "o2Editor/Core/Properties/Basic/StringProperty.h"
#include "o2Editor/Core/Properties/Basic/TagProperty.h"
#include "o2Editor/Core/Properties/Basic/Vector2FloatProperty.h"
#include "o2Editor/Core/Properties/Basic/Vector2IntProperty.h"
#include "o2Editor/Core/Properties/Basic/WStringProperty.h"
#include "o2Editor/Core/UI/SpoilerWithHead.h"
#include "o2Editor/Core/UIStyle/BasicUIStyle.h"
#include "o2Editor/Core/WindowsSystem/DockableWindow.h"
#include "o2Editor/PropertiesWindow/ActorsViewer/AddComponentPanel.h"
#include "o2Editor/TreeWindow/SceneTree.h"

using namespace o2;

namespace Editor
{
	void EditorUIStyleBuilder::RebuildDockableWndStyle()
	{
		DockableWindow* sample = mnew DockableWindow();
		sample->layout->minSize = Vec2F(100, 50);
		sample->SetClippingLayout(Layout::BothStretch(-1, 0, -2, 17));
		sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 20));
		sample->SetEnableScrollsHiding(true);

		*sample->GetDockingFrameSample() = Sprite("ui/UI4_Window_place.png");

		// not tab back
		auto backLayer = sample->AddLayer("back", nullptr);

		auto regularBackLayer = backLayer->AddChildLayer("regularBack", mnew Sprite("ui/UI4_window_frame_regular.png"),
														 Layout::BothStretch(-13, -13, -13, -11));

		auto dockedBackLayer = backLayer->AddChildLayer("dockedBack", mnew Sprite("ui/UI4_window_frame_docked.png"),
														Layout::BothStretch(-13, -13, -13, -11));

		auto iconLayer = backLayer->AddChildLayer("icon", mnew Sprite("ui/UI4_o2_sign.png"),
												  Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, -1)));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "Window";
		captionText->horAlign = HorAlign::Left;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		auto textLayer = backLayer->AddChildLayer("caption", captionText,
												  Layout::HorStretch(VerAlign::Top, 20, 35, 20, -2));

		// tab head
		auto tabBackLayer = sample->AddLayer("tab", nullptr);

		auto tabBackDown = tabBackLayer->AddChildLayer("back", mnew Sprite("ui/UI4_window_frame_docked_tab.png"),
													   Layout::BothStretch(-13, -13, -13, -11));

		auto tabBack = tabBackLayer->AddChildLayer("tabBack", mnew Sprite("ui/UI4_window_frame_docked_tab_back.png"),
												   Layout::HorStretch(VerAlign::Top, -13, -13, 40, -11));

		auto tabMain = tabBackLayer->AddChildLayer("main", nullptr,
												   Layout::HorStretch(VerAlign::Top, 0, 0, 19));

		auto tabMainInactiveBack = tabMain->AddChildLayer("inactive", mnew Sprite("ui/UI4_window_frame_docked_tab_inactive.png"),
														  Layout::HorStretch(VerAlign::Top, -13, -11, 40, -11));

		auto tabMainActiveBack = tabMain->AddChildLayer("active", mnew Sprite("ui/UI4_window_frame_docked_tab_active_back copy.png"),
														Layout::HorStretch(VerAlign::Top, -13, -11, 40, -11));

		auto tabIconLayer = tabMain->AddChildLayer("icon", mnew Sprite("ui/UI4_o2_sign.png"),
												   Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, -1)));

		Text* tabCaptionText = mnew Text("stdFont.ttf");
		tabCaptionText->text = "Window";
		tabCaptionText->horAlign = HorAlign::Left;
		tabCaptionText->verAlign = VerAlign::Middle;
		tabCaptionText->dotsEngings = true;
		auto tabTextLayer = tabMain->AddChildLayer("caption", tabCaptionText,
												   Layout::HorStretch(VerAlign::Top, 20, 10, 20, -2));


		// scroll bars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -20);
		sample->SetVerticalScrollBar(verScrollBar);

		//states
		AnimationClip dockedStateAnim;
		*dockedStateAnim.AddTrack<float>("layer/back/child/dockedBack/transparency") = AnimationTrack<float>::EaseInOut(0, 1, 0.2f);
		*dockedStateAnim.AddTrack<float>("layer/back/child/regularBack/transparency") = AnimationTrack<float>::EaseInOut(1, 0, 0.2f);

		auto dockedState = sample->AddState("docked", dockedStateAnim);


		AnimationClip tabStateAnim;
		*tabStateAnim.AddTrack<float>("layer/tab/transparency") = AnimationTrack<float>::EaseInOut(0, 1, 0.2f);
		*tabStateAnim.AddTrack<float>("layer/back/transparency") = AnimationTrack<float>::EaseInOut(1, 0, 0.2f);

		auto tabbedState = sample->AddState("tab", tabStateAnim);


		AnimationClip tabActiveStateAnim;
		*tabActiveStateAnim.AddTrack<float>("layer/tab/child/main/child/active/transparency") = AnimationTrack<float>::EaseInOut(0, 1, 0.2f);
		*tabActiveStateAnim.AddTrack<float>("layer/tab/child/main/child/inactive/transparency") = AnimationTrack<float>::EaseInOut(1, 0, 0.2f);
		*tabActiveStateAnim.AddTrack<float>("layer/tab/child/back/transparency") = AnimationTrack<float>::EaseInOut(0, 1, 0.2f);

		auto tabActiveState = sample->AddState("tabActive", tabActiveStateAnim);


		sample->AddState("tabFirst", AnimationClip::EaseInOut("layer/tab/child/tabBack/transparency", 0.0f, 1.0f, 0.2f));


		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 2.0f;

		// additional elements
		Button* closeBtn = o2UI.CreateWidget<Button>("close");
		closeBtn->name = "closeButton";
		*closeBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(0, 2));
		sample->AddInternalWidget(closeBtn);

		Button* optionsBtn = o2UI.CreateWidget<Button>("arrow");
		optionsBtn->name = "optionsButton";
		*optionsBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-16, 2));
		sample->AddInternalWidget(optionsBtn);

		// drag handles
		sample->SetDragAreaLayouts(Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(5, -15), Vec2F(-5, -2)),    // head
								   Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(5, -2), Vec2F(-5, 5)),      // top
								   Layout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(5, -5), Vec2F(-5, 5)),      // bottom
								   Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(-5, 5), Vec2F(5, -5)),      // left
								   Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-5, 5), Vec2F(5, -5)),      // right
								   Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(-5, -5), Vec2F(5, 5)),      // left top
								   Layout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-5, -5), Vec2F(5, 5)),      // right top
								   Layout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(-5, -5), Vec2F(5, 5)),      // left bottom 
								   Layout(Vec2F(1, 0), Vec2F(1, 0), Vec2F(-5, -5), Vec2F(5, 5)));     // right bottom

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildPlayStopButtonStyle()
	{
		Toggle* sample = mnew Toggle();
		auto playRootIconLayer = sample->AddLayer("playRootIcon", nullptr);
		auto stopRootIconLayer = sample->AddLayer("stopRootIcon", nullptr);

		auto playIconLayer = playRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_play_btn_regular.png"),
															  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto playSelectIconLayer = playRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_play_btn_select.png"),
																	Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto playPressedIconLayer = playRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_play_btn_pressed.png"),
																	 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopIconLayer = stopRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_stop_btn_regular.png"),
															  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopSelectIconLayer = stopRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_stop_btn_select.png"),
																	Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopPressedIconLayer = stopRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_stop_btn_pressed.png"),
																	 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		AnimationClip playBtnSelectAnim = AnimationClip::EaseInOut("layer/playRootIcon/child/hover/transparency", 0.0f, 1.0f, 0.1f);
		*playBtnSelectAnim.AddTrack<float>("layer/stopRootIcon/child/hover/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", playBtnSelectAnim)->offStateAnimationSpeed = 0.25f;

		AnimationClip playBtnPressAnim = AnimationClip::EaseInOut("layer/playRootIcon/child/pressed/transparency", 0.0f, 1.0f, 0.1f);
		*playBtnPressAnim.AddTrack<float>("layer/stopRootIcon/child/pressed/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);

		sample->AddState("pressed", playBtnPressAnim)->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnPressAnim = AnimationClip::EaseInOut("layer/playRootIcon/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnPressAnim.AddTrack<float>("layer/stopRootIcon/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnPressAnim);

		o2UI.AddWidgetStyle(sample, "play-stop");
	}

	void EditorUIStyleBuilder::RebuildPauseButtonStyle()
	{
		Toggle* sample = mnew Toggle();
		auto pauseIconRootLayer = sample->AddLayer("regular", nullptr);
		auto pauseIconLayer = pauseIconRootLayer->AddChildLayer("icon", mnew Sprite("ui/UI4_pause_btn_regular.png"),
																Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
																Vec2F(-10, 10), Vec2F(10, -10)));

		auto pauseSelectIconLayer = pauseIconRootLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_pause_btn_select.png"),
																	  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
																	  Vec2F(-10, 10), Vec2F(10, -10)));

		auto pausePressedIconLayer = pauseIconRootLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_pause_btn_pressed.png"),
																	   Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
																	   Vec2F(-10, 10), Vec2F(10, -10)));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/regular/child/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 0.25f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/regular/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

		sample->AddState("value", AnimationClip::EaseInOut("layer/regular/transparency", 0.3f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "pause");
	}

	void EditorUIStyleBuilder::RebuildStepButtonStyle()
	{
		Button* sample = mnew Button();
		sample->name = "step button";
		auto stepIconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_step_btn_regular.png"),
											  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
											  Vec2F(-10, 10), Vec2F(10, -10)));

		auto stepSelectIconLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_step_btn_select.png"),
													Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
													Vec2F(-10, 10), Vec2F(10, -10)));

		auto stepPressedIconLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_step_btn_pressed.png"),
													 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f),
													 Vec2F(-10, 10), Vec2F(10, -10)));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 0.25f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "step");
	}

	void EditorUIStyleBuilder::RebuildRoundDropDown()
	{
		DropDown* sample = mnew DropDown();
		sample->layout->minSize = Vec2F(20, 20);
		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_panel_subpanel_bk.png"),
										  Layout::BothStretch(-7, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_subpanel_select.png"),
											Layout::BothStretch(-7, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_subpanel_pressed.png"),
											 Layout::BothStretch(-7, -5, -5, -5));

		auto arrowLayer = sample->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn_white.png"),
										   Layout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-5, 0)));

		sample->SetClippingLayout(Layout::BothStretch(10, 3, 20, 1));

		auto list = sample->GetListView();
		*list = *o2UI.GetWidgetStyle<CustomList>("standard");

		list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
		list->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));

		list->RemoveLayer("back");
		list->AddLayer("back", mnew Sprite("ui/UI4_Context_menu.png"), Layout::BothStretch(-21, -19, -19, -19));

		list->layout->pivot = Vec2F(0.5f, 1.0f);
		list->layout->anchorMin = Vec2F(0, 0);
		list->layout->anchorMax = Vec2F(1, 0);
		list->layout->offsetMin = Vec2F(0, -60);
		list->layout->offsetMax = Vec2F(0, 0);

		Label* itemSample = o2UI.CreateLabel("empty");
		itemSample->horAlign = HorAlign::Left;
		sample->SetItemSample(itemSample);


		// Selected text
		Text* captionText = mnew Text("stdFont.ttf");
		captionText->horAlign = HorAlign::Left;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->wordWrap = false;
		captionText->color = Color4(235, 255, 253);
		sample->AddLayer("selectedText", captionText, Layout::BothStretch(7, 0, 0, 0));

		// States
		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "round");
	}

	void EditorUIStyleBuilder::RebuildMenuPanelDropDown()
	{
		DropDown* sample = mnew DropDown();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -5, -5, -5));

		auto arrowLayer = sample->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn.png"),
										   Layout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0)));

		sample->SetClippingLayout(Layout::BothStretch(10, 3, 20, 1));

		auto list = sample->GetListView();
		*list = *o2UI.GetWidgetStyle<CustomList>("standard");

		list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
		list->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));

		list->RemoveLayer("back");
		list->AddLayer("back", mnew Sprite("ui/UI4_Context_menu.png"), Layout::BothStretch(-21, -19, -19, -19));

		list->layout->pivot = Vec2F(0.5f, 1.0f);
		list->layout->anchorMin = Vec2F(0, 0);
		list->layout->anchorMax = Vec2F(1, 0);
		list->layout->offsetMin = Vec2F(0, -60);
		list->layout->offsetMax = Vec2F(0, 0);

		Label* itemSample = o2UI.CreateLabel("empty");
		itemSample->horAlign = HorAlign::Left;
		sample->SetItemSample(itemSample);


		// Selected text
		Text* captionText = mnew Text("stdFont.ttf");
		captionText->horAlign = HorAlign::Left;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->wordWrap = false;
		captionText->color = Color4(96, 125, 139);
		sample->AddLayer("selectedText", captionText, Layout::BothStretch(7, 0, 0, 0));

		// States
		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu panel");
	}

	void EditorUIStyleBuilder::RebuildSearchButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_search_regular.png"),
											 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_search_select.png"),
											Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_search_pressed.png"),
											 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "search");
	}

	void EditorUIStyleBuilder::RebuildListTreeToggle()
	{
		Toggle* sample = mnew Toggle();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto listLayer = sample->AddLayer("listIcon", mnew Sprite("ui/UI4_list_icon.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto treeLayer = sample->AddLayer("treeIcon", mnew Sprite("ui/UI4_tree_icon.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/treeIcon/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/listIcon/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "list-tree");
	}

	void EditorUIStyleBuilder::RebuildRevertBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);
		auto rootLayer = sample->AddLayer("root", nullptr);
		auto selectLayer = rootLayer->AddChildLayer("hover", nullptr);
		auto iconLayer = selectLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_revert.png"),
													Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedIconLayer = selectLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_revert.png"),
														   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/root/child/hover/transparency", 1.0f, 0.5f, 0.1f))
			->offStateAnimationSpeed = 0.25f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/root/child/hover/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "revert");
	}

	void EditorUIStyleBuilder::RebuildActorsTreeNodeEnableToggle()
	{
		Toggle* sample = mnew Toggle();

		auto halfHideLayer = sample->AddLayer("halfHide", nullptr);

		auto back = halfHideLayer->AddChildLayer("back", mnew Sprite("ui/UI4_off_dot.png"),
												 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto dot = halfHideLayer->AddChildLayer("dot", mnew Sprite("ui/UI4_on_dot.png"),
												Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

		AnimationClip valueAnim = AnimationClip::EaseInOut("layer/halfHide/child/dot/transparency", 0.0f, 1.0f, 0.1f);
		*valueAnim.AddTrack<float>("layer/halfHide/child/back/transparency") = AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.1f);
		sample->AddState("value", valueAnim);

		sample->AddState("halfHide", AnimationClip::EaseInOut("layer/halfHide/transparency", 1.0f, 0.5f, 0.1f));

		o2UI.AddWidgetStyle(sample, "actor node enable");
	}

	void EditorUIStyleBuilder::RebuildActorsTreeLockToggle()
	{
		Toggle* sample = mnew Toggle();

		auto halfHideLayer = sample->AddLayer("halfHide", nullptr);

		auto locked = halfHideLayer->AddChildLayer("locked", mnew Sprite("ui/UI4_lock_icon.png"),
												   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto unlocked = halfHideLayer->AddChildLayer("unlocked", mnew Sprite("ui/UI4_unlock_icon.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

		AnimationClip valueStateAnim = AnimationClip::EaseInOut("layer/halfHide/child/locked/transparency", 0.0f, 1.0f, 0.1f);
		*valueStateAnim.AddTrack<float>("layer/halfHide/child/unlocked/transparency") = AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.1f);
		sample->AddState("locked", valueStateAnim);

		sample->AddState("halfHide", AnimationClip::EaseInOut("layer/halfHide/transparency", 1.0f, 0.5f, 0.1f));

		o2UI.AddWidgetStyle(sample, "actor node lock");
	}

	void EditorUIStyleBuilder::RebuildActorsTreeLinkBtn()
	{
		Button* sample = mnew Button();

		auto halfHideLayer = sample->AddLayer("halfHide", nullptr);
		auto selectLayer = halfHideLayer->AddChildLayer("hover", nullptr);
		auto pressedLayer = selectLayer->AddChildLayer("pressed", nullptr);
		auto back = pressedLayer->AddChildLayer("back", mnew Sprite("ui/UI4_link_icon.png"),
												Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));


		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));
		sample->AddState("halfHide", AnimationClip::EaseInOut("layer/halfHide/transparency", 1.0f, 0.5f, 0.1f));
		sample->AddState("hover", AnimationClip::EaseInOut("layer/halfHide/child/hover/transparency", 1.0f, 0.5f, 0.1f));
		sample->AddState("pressed", AnimationClip::EaseInOut("layer/halfHide/child/hover/child/pressed/transparency", 0.8f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "actor node link");
	}

	void EditorUIStyleBuilder::RebuildActorsTree()
	{
		// basics
		SceneTree* sample = mnew SceneTree();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 0, 1));
		sample->SetViewLayout(Layout::BothStretch(0, 0, 1, 0));
		sample->SetEnableScrollsHiding(true);
		sample->SetChildsNodesOffset(10);

		// hover
		*sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");
		sample->SetHoverLayout(Layout::BothStretch(-10, -16, -10, -16));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());

		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// node sample
		TreeNode* itemSample = sample->GetNodeSample();

		auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		Text* captionLayerText = mnew Text("stdFont.ttf");
		captionLayerText->horAlign = HorAlign::Left;
		captionLayerText->verAlign = VerAlign::Middle;
		captionLayerText->color = Color4(96, 125, 139);
		itemSample->AddLayer("name", captionLayerText, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(14, -20), Vec2F(0, 0)));

		auto nameLayer = itemSample->layer["name"];
		nameLayer->layout = Layout::BothStretch(15, 0, 55, 0);
		((Text*)nameLayer->GetDrawable())->dotsEngings = true;

		auto actorNodeEnableToggle = o2UI.CreateWidget<Toggle>("actor node enable");
		actorNodeEnableToggle->name = "enableToggle";
		*actorNodeEnableToggle->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-5, 0));
		itemSample->AddChild(actorNodeEnableToggle);

		auto actorNodeLockToggle = o2UI.CreateWidget<Toggle>("actor node lock");
		actorNodeLockToggle->name = "lockToggle";
		*actorNodeLockToggle->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-20, 0));
		itemSample->AddChild(actorNodeLockToggle);

		auto actorNodeLinkButton = o2UI.CreateWidget<Button>("actor node link");
		actorNodeLinkButton->name = "linkBtn";
		*actorNodeLinkButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-35, 0));
		itemSample->AddChild(actorNodeLinkButton);

		auto actorNodeEditBox = o2UI.CreateWidget<EditBox>("backless");
		actorNodeEditBox->name = "nameEditBox";
		*actorNodeEditBox->layout = WidgetLayout::BothStretch(15, 0, 55, 0);
		actorNodeEditBox->Hide(true);
		itemSample->AddChild(actorNodeEditBox);

		AnimationClip itemEditStateAnim = AnimationClip::EaseInOut("layer/name/transparency", 1.0f, 0.0f, 0.15f);
		*itemEditStateAnim.AddTrack<bool>("child/nameEditBox/enabled") = AnimationTrack<bool>::Linear(false, true, 0.15f);
		itemSample->AddState("edit", itemEditStateAnim);

		// node sample button
		Button* itemSampleExpandBtn = mnew Button();
		itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
		itemSampleExpandBtn->name = "expandBtn";

		auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mnew Sprite("ui/UI4_Right_icn.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = itemSampleExpandBtn->AddLayer("hover", mnew Sprite("ui/UI4_Right_icn_select.png"),
														 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mnew Sprite("ui/UI4_Right_icn_pressed.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		itemSampleExpandBtn->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		itemSampleExpandBtn->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->layout->anchorMin = Vec2F(0, 1);
		itemSampleExpandBtn->layout->anchorMax = Vec2F(0, 1);
		itemSampleExpandBtn->layout->offsetMin = Vec2F(2, -20);
		itemSampleExpandBtn->layout->offsetMax = Vec2F(12, 0);

		itemSample->AddChild(itemSampleExpandBtn);

		AnimationClip expandedStateAnim;
		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

		itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		itemSample->AddState("focused", focusedItemAnim);

		// scrollbars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
						 5.0f, 15.0f, 0.2f));

		sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
						 -5.0f, -15.0f, 0.2f));

		sample->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetStateForcible("hover", false);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5;

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildPanelDownButton()
	{
		Button* sample = mnew Button();

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -5, -5, -5));

		auto arrowLayer = sample->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn.png"),
										   Layout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0)));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "Button";
		captionText->horAlign = HorAlign::Middle;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->color = Color4(96, 125, 139);
		sample->AddLayer("caption", captionText, Layout::BothStretch(0, 0, 15, 0));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "panel down");
	}

	void EditorUIStyleBuilder::RebuildTrashDownPanelButton()
	{
		auto sample = mnew Button();

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -5, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto trashIconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_small_trash_icon.png"),
											   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "down panel trash");
	}

	void EditorUIStyleBuilder::RebuildMessagesDownPanelToggle()
	{
		Toggle* sample = mnew Toggle();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -5, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto offLayer = sample->AddLayer("off", mnew Sprite("ui/UI4_log_info_inactive_icon.png"),
										 Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		auto onLayer = sample->AddLayer("on", mnew Sprite("ui/UI4_log_info_icon.png"),
										Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "999";
		captionText->horAlign = HorAlign::Middle;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->color = Color4(96, 125, 139);
		sample->AddLayer("caption", captionText, Layout::BothStretch(13, 0, 0, 0));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/off/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/on/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "log messages");
	}

	void EditorUIStyleBuilder::RebuildWarningsDownPanelToggle()
	{
		Toggle* sample = mnew Toggle();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -5, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto offLayer = sample->AddLayer("off", mnew Sprite("ui/UI4_log_warnings_inactive_icon.png"),
										 Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		auto onLayer = sample->AddLayer("on", mnew Sprite("ui/UI4_log_warnings_icon.png"),
										Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "999";
		captionText->horAlign = HorAlign::Middle;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->color = Color4(96, 125, 139);
		sample->AddLayer("caption", captionText, Layout::BothStretch(13, 0, 0, 0));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/off/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/on/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "log warnings");
	}

	void EditorUIStyleBuilder::RebuildErrorsDownPanelToggle()
	{
		Toggle* sample = mnew Toggle();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -5, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto offLayer = sample->AddLayer("off", mnew Sprite("ui/UI4_log_errors_inactive_icon.png"),
										 Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		auto onLayer = sample->AddLayer("on", mnew Sprite("ui/UI4_log_errors_icon.png"),
										Layout::Based(BaseCorner::Left, Vec2F(30, 30), Vec2F(-4, -2)));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "999";
		captionText->horAlign = HorAlign::Middle;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->color = Color4(96, 125, 139);
		sample->AddLayer("caption", captionText, Layout::BothStretch(13, 0, 0, 0));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/off/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/on/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "log errors");
	}

	void EditorUIStyleBuilder::RebuildFilterMenuButton()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_filter_icon.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(1, 0)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu filter");
	}

	void EditorUIStyleBuilder::RebuildTreeMenuButton()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -5, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -5, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -5, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_tree_icon.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu tree");
	}

	void EditorUIStyleBuilder::RebuildFoldersTree()
	{
		// basics
		Tree* sample = mnew Tree();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 0, 1));
		sample->SetViewLayout(Layout::BothStretch(0, 0, 1, 0));
		sample->SetEnableScrollsHiding(true);
		sample->SetChildsNodesOffset(10);

		// hover
		*sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");
		sample->SetHoverLayout(Layout::BothStretch(-10, -16, -10, -16));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());
		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// node sample
		TreeNode* itemSample = sample->GetNodeSample();

		auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		itemSample->AddLayer("icon", mnew Sprite("ui/UI4_folder_icon_dark.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(10, 0)));

		Text* captionLayerText = mnew Text("stdFont.ttf");
		captionLayerText->horAlign = HorAlign::Left;
		captionLayerText->verAlign = VerAlign::Middle;
		captionLayerText->color = Color4(96, 125, 139);
		itemSample->AddLayer("name", captionLayerText, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(12, -20), Vec2F(0, 0)));

		auto nameLayer = itemSample->layer["name"];
		nameLayer->layout = Layout::BothStretch(30, 0, 5, 0);
		((Text*)nameLayer->GetDrawable())->dotsEngings = true;

		auto actorNodeEditBox = o2UI.CreateWidget<EditBox>("backless");
		actorNodeEditBox->name = "nameEditBox";
		*actorNodeEditBox->layout = WidgetLayout::BothStretch(30, 0, 5, 0);
		actorNodeEditBox->Hide(true);
		itemSample->AddChild(actorNodeEditBox);

		AnimationClip itemEditStateAnim = AnimationClip::EaseInOut("layer/name/transparency", 1.0f, 0.0f, 0.15f);
		*itemEditStateAnim.AddTrack<bool>("child/nameEditBox/enabled") = AnimationTrack<bool>::Linear(false, true, 0.15f);
		itemSample->AddState("edit", itemEditStateAnim);

		// node sample button
		Button* itemSampleExpandBtn = mnew Button();
		itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
		itemSampleExpandBtn->name = "expandBtn";

		auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mnew Sprite("ui/UI4_Right_icn.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = itemSampleExpandBtn->AddLayer("hover", mnew Sprite("ui/UI4_Right_icn_select.png"),
														 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mnew Sprite("ui/UI4_Right_icn_pressed.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		itemSampleExpandBtn->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		itemSampleExpandBtn->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->layout->anchorMin = Vec2F(0, 1);
		itemSampleExpandBtn->layout->anchorMax = Vec2F(0, 1);
		itemSampleExpandBtn->layout->offsetMin = Vec2F(0, -20);
		itemSampleExpandBtn->layout->offsetMax = Vec2F(10, 0);

		itemSample->AddChild(itemSampleExpandBtn);

		AnimationClip expandedStateAnim;
		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

		itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		itemSample->AddState("focused", focusedItemAnim);

		// scrollbars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
						 5.0f, 15.0f, 0.2f));

		sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
						 -5.0f, -15.0f, 0.2f));

		sample->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetStateForcible("hover", false);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5;

		o2UI.AddWidgetStyle(sample, "folders");
	}

	void EditorUIStyleBuilder::RebuildRegularAssetIcon()
	{
		AssetIcon* sample = mnew AssetIcon();
		sample->layout->minSize = Vec2F(50, 60);

		// selection layer
		auto itemSelectionLayer = sample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		// icon
		sample->AddLayer("icon", mnew Sprite("ui/UI4_big_file_icon.png"),
						 Layout::Based(BaseCorner::Center, Vec2F(40, 40), Vec2F(0, 10)));

		// hover
		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_Context_menu_white.png"),
										   Layout::BothStretch(-10, -16, -10, -16));

		// name label
		Label* nameLabel = o2UI.CreateLabel("file name");
		nameLabel->name = "nameLabel";
		*nameLabel->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 25, 0);
		nameLabel->height = 8;
		nameLabel->linesDistanceCoef = 0.7f;
		nameLabel->horAlign = HorAlign::Middle;
		nameLabel->verAlign = VerAlign::Top;
		nameLabel->horOverflow = Label::HorOverflow::Wrap;
		nameLabel->verOverflow = Label::VerOverflow::Cut;
		sample->AddChild(nameLabel);

		// edit box
		EditBox* nameEditBox = mnew EditBox();
		nameEditBox->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
		nameEditBox->SetViewLayout(Layout::BothStretch(0, 0, 2, 0));
		nameEditBox->SetCaretBlinkingDelay(1.15f);
		nameEditBox->SetSelectionColor(Color4(0, 156, 141, 120));
		nameEditBox->SetMultiLine(false);

		Text* textDrawable = nameEditBox->GetTextDrawable();
		textDrawable->SetFontAsset(FontAssetRef("stdFont.ttf"));
		textDrawable->height = 8;
		textDrawable->color = Color4(96, 125, 139);

		Sprite* caretDrawable = nameEditBox->GetCaretDrawable();
		*caretDrawable = Sprite();
		caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetHeight()) * 1.7f);
		caretDrawable->pivot = Vec2F(0, 0.26f);
		caretDrawable->color = Color4::Black();

		nameEditBox->name = "nameEditBox";
		*nameEditBox->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 22, 0);
		sample->AddChild(nameEditBox);

		// edit state
		AnimationClip itemEditStateAnim = AnimationClip::EaseInOut("child/nameLabel/enabled", true, false, 0.15f);
		*itemEditStateAnim.AddTrack<bool>("child/nameEditBox/enabled") = AnimationTrack<bool>::Linear(false, true, 0.15f);
		sample->AddState("edit", itemEditStateAnim);

		// focused state
		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		sample->AddState("focused", focusedItemAnim);

		sample->AddState("halfHide", AnimationClip::EaseInOut("transparency", 1.0f, 0.5f, 0.1f));
		sample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));
		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildAssetsGridScroll()
	{
		AssetsIconsScrollArea* sample = mnew AssetsIconsScrollArea();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
		sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
		sample->SetEnableScrollsHiding(true);
		sample->SetItemSample(o2UI.CreateWidget<AssetIcon>());
		sample->SetItemsSpacing(Vec2F(5, 5));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());
		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// selection
		*sample->GetSelectingDrawable() = Sprite("ui/UI4_selection_frame.png");

		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildLinkBtn()
	{
		Button* sample = mnew Button();

		auto halfHideLayer = sample->AddLayer("halfHide", nullptr);
		auto selectLayer = halfHideLayer->AddChildLayer("hover", nullptr);
		auto pressedLayer = selectLayer->AddChildLayer("pressed", nullptr);
		auto back = pressedLayer->AddChildLayer("back", mnew Sprite("ui/UI4_link_icon.png"),
												Layout::Based(BaseCorner::Center, Vec2F(15, 15), Vec2F(0, 0)));


		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));
		sample->AddState("halfHide", AnimationClip::EaseInOut("layer/halfHide/transparency", 1.0f, 0.5f, 0.1f));
		sample->AddState("hover", AnimationClip::EaseInOut("layer/halfHide/child/hover/transparency", 1.0f, 0.5f, 0.1f));
		sample->AddState("pressed", AnimationClip::EaseInOut("layer/halfHide/child/hover/child/pressed/transparency", 0.8f, 1.0f, 0.1f));

		o2UI.AddWidgetStyle(sample, "asset link");
	}

	void EditorUIStyleBuilder::RebuildSinglelineEditboxProperty()
	{
		EditBox* sample = o2UI.CreateWidget<EditBox>("singleline");
		sample->SetClippingLayout(Layout::BothStretch(0, 0, 10, 0));
		sample->SetViewLayout(Layout::BothStretch(3, 1, 8, -1));

		sample->AddLayer("arrows", mnew Sprite("ui/UI4_ch_arrows.png"),
						 Layout::Based(BaseCorner::Right, Vec2F(10, 20), Vec2F(0, 0)));

		o2UI.AddWidgetStyle(sample, "singleline with arrows");
	}

	void EditorUIStyleBuilder::RebuildEditorDropdown()
	{
		auto sample = mnew Widget();

		DropDown* dropdown = mnew DropDown();
		*dropdown->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		dropdown->name = "dropdown";
		auto backLayer = dropdown->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
		auto selectLayer = dropdown->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
		auto pressedLayer = dropdown->AddLayer("pressed", mnew Sprite("ui/UI4_Editbox_pressed.png"), Layout::BothStretch(-9, -9, -9, -9));
		auto arrowLayer = dropdown->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn.png"),
											 Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));

		dropdown->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

		auto list = dropdown->GetListView();
		*list = *o2UI.GetWidgetStyle<CustomList>("standard");
		list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
		delete list->layer["back"]->GetDrawable();
		list->layer["back"]->SetDrawable(mnew Sprite("ui/UI4_Box_regular.png"));
		list->layout->pivot = Vec2F(0.5f, 1.0f);
		list->layout->anchorMin = Vec2F(0, 0);
		list->layout->anchorMax = Vec2F(1, 0);
		list->layout->offsetMin = Vec2F(0, -60);
		list->layout->offsetMax = Vec2F(0, 1);

		Text* undefinedText = mnew Text("stdFont.ttf");
		undefinedText->text = "--";
		undefinedText->horAlign = HorAlign::Left;
		undefinedText->verAlign = VerAlign::Middle;
		undefinedText->dotsEngings = true;
		undefinedText->color = Color4(96, 125, 139);
		auto undefinedLayer = dropdown->AddLayer("undefined", undefinedText, Layout::BothStretch(3, 0, 3, 0));

		Label* itemSample = o2UI.CreateLabel("empty");
		itemSample->horAlign = HorAlign::Left;
		dropdown->SetItemSample(itemSample);

		dropdown->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		dropdown->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		dropdown->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

		dropdown->AddState("undefined", AnimationClip::EaseInOut("layer/undefined/transparency", 0.0f, 1.0f, 0.05f));

		dropdown->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		Button* revertBtn = o2UI.CreateWidget<Button>("revert");
		*revertBtn->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F());

		sample->AddChild(dropdown);
		sample->AddChild(revertBtn);

		AnimationClip revertStateAnim = AnimationClip::EaseInOut("child/dropdown/layout/offsetRight", 0.0f, -20.0f, 0.15f);
		*revertStateAnim.AddTrack<bool>("child/revert/enabled") = AnimationTrack<bool>::EaseInOut(false, true, 0.15f);
		sample->AddState("revert", revertStateAnim);

		o2UI.AddWidgetStyle(sample, "enum property");
	}

	void EditorUIStyleBuilder::RebuildActorHeadEnableToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);
		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_enable_toggle_big.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("backSelect", mnew Sprite("ui/UI4_enable_toggle_big_select.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_enable_toggle_big_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_enable_toggle_big_focused.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto checkLayer = sample->AddLayer("check", mnew Sprite("ui/UI4_enable_dot_big.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(10, 10)));

		auto unknownLayer = sample->AddLayer("unknown", mnew Sprite("ui/UI4_enable_dot_big_unknown.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(10, 10)));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/backSelect/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("value", AnimationClip::EaseInOut("layer/check/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("unknown", AnimationClip::EaseInOut("layer/unknown/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "actor head enable");
	}

	void EditorUIStyleBuilder::RebuildActorHeadEnableProperty()
	{
		auto sample = mnew BooleanProperty();
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		Toggle* toggle = o2UI.CreateToggle("empty", "actor head enable");
		toggle->name = "toggle";
		*toggle->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		layout->AddChild(toggle);

		o2UI.AddWidgetStyle(sample, "actor head enable");
	}

	void EditorUIStyleBuilder::RebuildActorHeadName()
	{
		EditBox* sample = mnew EditBox();
		sample->SetClippingLayout(Layout::BothStretch(5, 0, 5, 0));
		sample->SetViewLayout(Layout::BothStretch(7, 0, 7, 0));
		sample->SetCaretBlinkingDelay(1.15f);
		sample->SetMultiLine(false);
		sample->SetSelectionColor(Color4(0, 156, 141, 120));
		sample->layout->minSize = Vec2F(50, 17);

		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_round_field.png"),
										  Layout::BothStretch(-4, -4, -5, -4));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										   Layout::BothStretch(-4, -4, -5, -4));

		auto focusLayer = sample->AddLayer("focus", mnew Sprite("ui/UI4_round_field_select.png"),
										   Layout::BothStretch(-4, -4, -5, -4));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip focusAnim = AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f);
		*focusAnim.AddTrack<float>("layer/hover/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);
		sample->AddState("focused", focusAnim)
			->offStateAnimationSpeed = 0.5f;

		Text* textDrawable = sample->GetTextDrawable();
		textDrawable->verAlign = VerAlign::Middle;
		textDrawable->horAlign = HorAlign::Left;
		textDrawable->color = Color4(96, 125, 139);
		textDrawable->SetFontAsset(FontAssetRef("stdFont.ttf"));

		Sprite* caretDrawable = sample->GetCaretDrawable();
		*caretDrawable = Sprite();
		caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetHeight()) * 1.7f);
		caretDrawable->pivot = Vec2F(0, 0.26f);
		caretDrawable->color = Color4::Black();

		o2UI.AddWidgetStyle(sample, "actor head name");
	}

	void EditorUIStyleBuilder::RebuildActorHeadNameProperty()
	{
		auto sample = mnew StringProperty();
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		EditBox* editBox = o2UI.CreateEditBox("actor head name");
		*editBox->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		editBox->name = "editBox";
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "actor head name");
	}

	void EditorUIStyleBuilder::RebuildActorHeadLockToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto rootLayer = sample->AddLayer("root", nullptr);
		auto valueLayer = rootLayer->AddChildLayer("value", nullptr);

		auto unlockLayer = valueLayer->AddChildLayer("unlock", mnew Sprite("ui/UI4_big_lock_open.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto lockLayer = valueLayer->AddChildLayer("lock", mnew Sprite("ui/UI4_big_lock_close.png"),
												   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto unknownLayer = rootLayer->AddChildLayer("unknown", mnew Sprite("ui/UI4_big_lock_unknown.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/root/transparency", 1.0f, 0.7f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		auto valueAnim = AnimationClip::EaseInOut("layer/root/child/value/child/unlock/transparency", 1.0f, 0.0f, 0.1f);
		*valueAnim.AddTrack<float>("layer/root/child/value/child/lock/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueAnim)->offStateAnimationSpeed = 0.5f;

		auto unknownAnim = AnimationClip::EaseInOut("layer/root/child/value/transparency", 1.0f, 0.0f, 0.1f);
		*unknownAnim.AddTrack<float>("layer/root/child/unknown/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("unknown", unknownAnim)->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "actor head lock");
	}

	void EditorUIStyleBuilder::RebuildActorHeadLockProperty()
	{
		auto sample = mnew BooleanProperty();
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		Toggle* toggle = o2UI.CreateToggle("empty", "actor head lock");
		toggle->name = "toggle";
		*toggle->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		layout->AddChild(toggle);

		o2UI.AddWidgetStyle(sample, "actor head lock");
	}

	void EditorUIStyleBuilder::RebuildActorHeadActorAssetProperty()
	{
		auto sample = mnew AssetProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = true;

		auto spoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
		spoiler->name = "spoiler";
		spoiler->RemoveLayer("caption");
		*spoiler->layout = WidgetLayout::BothStretch();
		sample->AddChild(spoiler);

		auto mainLayout = mnew HorizontalLayout();
		mainLayout->name = "mainLayout";
		*mainLayout->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 19, 0);
		mainLayout->expandHeight = true;
		spoiler->AddInternalWidget(mainLayout);

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		mainLayout->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 19, 0);
		layout->baseCorner = BaseCorner::Right;
		layout->expandHeight = false;
		layout->spacing = 2;
		layoutContainer->AddChild(layout);

		auto box = mnew Widget();
		box->name = "box";
		box->SetFocusable(true);
		box->layout->minHeight = 19;
		layout->AddChild(box);

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_round_field_gray.png"),
									   Layout::BothStretch(-4, -4, -5, -4));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_round_field_gray_select.png"),
										 Layout::BothStretch(-4, -4, -5, -4));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_round_field_focused.png"),
										Layout::BothStretch(-4, -4, -5, -4));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		auto nameText = mnew Text("stdFont.ttf");
		nameText->text = "--";
		nameText->horAlign = HorAlign::Left;
		nameText->verAlign = VerAlign::Middle;
		nameText->dotsEngings = true;
		nameText->color = Color4(96, 125, 139);;
		box->AddLayer("caption", nameText, Layout::BothStretch(2, 2, 2, 2));

		box->SetFocusable(true);

		AnimationClip instanceAnim;

		*instanceAnim.AddTrack<bool>("child/spoiler/internalWidget/expand/enabled") =
			AnimationTrack<bool>::EaseInOut(false, true, 0.2f);

		sample->AddState("instance", instanceAnim);

		// 		auto linkBtn = o2UI.CreateWidget<Button>("asset link");
		// 		*linkBtn->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(15, 15), Vec2F());
		// 		box->AddChild(linkBtn);

		o2UI.AddWidgetStyle(sample, "actor head asset property");
	}

	void EditorUIStyleBuilder::RebuildActorHeadTags()
	{
		EditBox* sample = mnew EditBox();
		sample->SetClippingLayout(Layout::BothStretch(5, 0, 5, 0));
		sample->SetViewLayout(Layout::BothStretch(7, 0, 7, 0));
		sample->SetCaretBlinkingDelay(1.15f);
		sample->SetMultiLine(false);
		sample->SetSelectionColor(Color4(0, 156, 141, 120));
		sample->layout->minSize = Vec2F(50, 17);

		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_round_field_gray.png"),
										  Layout::BothStretch(-4, -4, -5, -4));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_round_field_gray_select.png"),
										   Layout::BothStretch(-4, -4, -5, -4));

		auto focusLayer = sample->AddLayer("focus", mnew Sprite("ui/UI4_round_field_select.png"),
										   Layout::BothStretch(-4, -4, -5, -4));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip focusAnim = AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f);
		*focusAnim.AddTrack<float>("layer/hover/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);
		sample->AddState("focused", focusAnim)
			->offStateAnimationSpeed = 0.5f;

		Text* textDrawable = sample->GetTextDrawable();
		textDrawable->verAlign = VerAlign::Middle;
		textDrawable->horAlign = HorAlign::Left;
		textDrawable->color = Color4(96, 125, 139);
		textDrawable->SetFontAsset(FontAssetRef("stdFont.ttf"));

		Sprite* caretDrawable = sample->GetCaretDrawable();
		*caretDrawable = Sprite();
		caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetHeight()) * 1.7f);
		caretDrawable->pivot = Vec2F(0, 0.26f);
		caretDrawable->color = Color4::Black();

		o2UI.AddWidgetStyle(sample, "actor head tags");
	}

	void EditorUIStyleBuilder::RebuildActorHeadTagsProperty()
	{
		auto sample = mnew TagsProperty();
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		EditBox* editBox = o2UI.CreateEditBox("actor head tags");
		editBox->name = "editBox";
		*editBox->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "actor head tags");
	}

	void EditorUIStyleBuilder::RebuildActorHeadLayer()
	{
		DropDown* sample = mnew DropDown();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_round_field_gray.png"),
										  Layout::BothStretch(-4, -4, -5, -4));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_round_field_gray_select.png"),
										   Layout::BothStretch(-4, -4, -5, -4));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_round_field_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -4));

		auto arrowLayer = sample->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn.png"),
										   Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));

		sample->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

		auto list = sample->GetListView();
		*list = *o2UI.GetWidgetStyle<CustomList>("standard");
		list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
		delete list->layer["back"]->GetDrawable();
		list->layer["back"]->SetDrawable(mnew Sprite("ui/UI4_Box_regular.png"));
		list->layout->pivot = Vec2F(0.5f, 1.0f);
		list->layout->anchorMin = Vec2F(0, 0);
		list->layout->anchorMax = Vec2F(1, 0);
		list->layout->offsetMin = Vec2F(-1, -60);
		list->layout->offsetMax = Vec2F(0, 3);

		Text* undefinedText = mnew Text("stdFont.ttf");
		undefinedText->text = "--";
		undefinedText->horAlign = HorAlign::Left;
		undefinedText->verAlign = VerAlign::Middle;
		undefinedText->dotsEngings = true;
		undefinedText->color = Color4(96, 125, 139);
		auto undefinedLayer = sample->AddLayer("undefined", undefinedText, Layout::BothStretch(3, 0, 3, 0));

		Label* itemSample = o2UI.CreateLabel("empty");
		itemSample->horAlign = HorAlign::Left;
		sample->SetItemSample(itemSample);

		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

		sample->AddState("undefined", AnimationClip::EaseInOut("layer/undefined/transparency", 0.0f, 1.0f, 0.05f));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "actor head layer");
	}

	void EditorUIStyleBuilder::RebuildActorHeadLayerProperty()
	{
		auto sample = mnew LayerProperty();
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		DropDown* dropDown = o2UI.CreateDropdown("actor head layer");
		dropDown->name = "dropDown";
		*dropDown->layout = WidgetLayout::BothStretch(0, 0, 20, 0);
		layout->AddChild(dropDown);

		o2UI.AddWidgetStyle(sample, "actor head layer");
	}

	void EditorUIStyleBuilder::RebuildAcceptPrototypeBtn()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_accept_prefab.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_accept_prefab_select.png"),
											Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_accept_prefab_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(25, 25)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "accept prototype");
	}

	void EditorUIStyleBuilder::RebuildRevertPrototypeBtn()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_revert_prefab.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_revert_prefab_select.png"),
											Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_revert_prefab_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(25, 25)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "revert prototype");
	}

	void EditorUIStyleBuilder::RebuildBreakPrototypeBtn()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_break_prefab.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_break_prefab_select.png"),
											Layout::Based(BaseCorner::Center, Vec2F(25, 25)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_break_prefab_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(25, 25)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "break prototype");
	}

	void EditorUIStyleBuilder::RebuildComponentOptionsBtn()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_gray_options.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_gray_options_select.png"),
											Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_gray_options_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "component options");
	}

	void EditorUIStyleBuilder::RebuildComponentSaveBtn()
	{
		Button* sample = mnew Button();
		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_save_gray.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_save_gray copy.png"),
											Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_save_gray.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "component save");
	}

	void EditorUIStyleBuilder::RebuildHorWideScrollbar()
	{
		HorizontalScrollBar* sample = mnew HorizontalScrollBar();
		sample->layout->minSize = Vec2F(5, 5);
		sample->SetScrollSense(0.25f);
		sample->SetMinimalScrollHandleSize(10);

		auto backLayer = sample->AddLayer("back", nullptr);
		auto backLayerImg = backLayer->AddChildLayer("image", mnew Sprite("ui/UI4_Editbox_regular.png"),
													 Layout::BothStretch(-9, -9, -9, -9));

		auto handleLayer = sample->AddLayer("handle", nullptr);
		handleLayer->interactableLayout = Layout::Based(BaseCorner::Center, Vec2F(8, 30));

		auto barRegularSprite = handleLayer->AddChildLayer("regular", mnew Sprite("ui/ver_hole_handle.png"),
														   Layout::Based(BaseCorner::Center, Vec2F(8, 30)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "wide");
	}

	void EditorUIStyleBuilder::RebuildVerWideScrollbar()
	{
		VerticalScrollBar* sample = mnew VerticalScrollBar();
		sample->layout->minSize = Vec2F(5, 5);
		sample->SetScrollSense(0.25f);
		sample->SetMinimalScrollHandleSize(10);

		auto backLayer = sample->AddLayer("back", nullptr);
		auto backLayerImg = backLayer->AddChildLayer("image", mnew Sprite("ui/UI4_Editbox_regular.png"),
													 Layout::BothStretch(-9, -9, -9, -9));

		auto handleLayer = sample->AddLayer("handle", nullptr);
		handleLayer->interactableLayout = Layout::Based(BaseCorner::Center, Vec2F(30, 8));

		auto barRegularSprite = handleLayer->AddChildLayer("regular", mnew Sprite("ui/hor_hole_handle.png"),
														   Layout::Based(BaseCorner::Center, Vec2F(30, 8)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "wide");
	}

	void EditorUIStyleBuilder::RebuildHorWideProgressbar()
	{
		HorizontalProgress* sample = mnew HorizontalProgress();
		sample->layout->minSize = Vec2F(5, 5);
		sample->SetScrollSense(0.25f);

		auto backLayer = sample->AddLayer("back", nullptr);
		auto backLayerImg = backLayer->AddChildLayer("image", mnew Sprite("ui/UI4_Editbox_regular.png"),
													 Layout::BothStretch(-9, -9, -9, -9));

		auto handleLayer = sample->AddLayer("bar", nullptr);
		auto barRegularSprite = handleLayer->AddChildLayer("regular", mnew Sprite("ui/ver_hole_handle.png"),
														   Layout::Based(BaseCorner::Right, Vec2F(8, 30), Vec2F(4, 0)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "wide");
	}

	void EditorUIStyleBuilder::RebuildVerWideProgressbar()
	{
		VerticalProgress* sample = mnew VerticalProgress();
		sample->layout->minSize = Vec2F(5, 5);
		sample->SetScrollSense(0.25f);

		auto backLayer = sample->AddLayer("back", nullptr);
		auto backLayerImg = backLayer->AddChildLayer("image", mnew Sprite("ui/UI4_Editbox_regular.png"),
													 Layout::BothStretch(-9, -9, -9, -9));

		auto handleLayer = sample->AddLayer("bar", nullptr);
		auto barRegularSprite = handleLayer->AddChildLayer("regular", mnew Sprite("ui/hor_hole_handle.png"),
														   Layout::Based(BaseCorner::Bottom, Vec2F(30, 8), Vec2F(0, -4)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "wide");
	}

	void EditorUIStyleBuilder::RebuildAnimationTree()
	{
		// basics
		AnimationTree* sample = mnew AnimationTree();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 0, 1));
		sample->SetViewLayout(Layout::BothStretch(0, 0, 1, 0));
		sample->SetEnableScrollsHiding(true);
		sample->SetChildsNodesOffset(10);

		// hover
		*sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");
		sample->SetHoverLayout(Layout::BothStretch(-10, -16, -10, -16));

		// zebra back
		sample->SetZebraBackLine(new Sprite(Color4(0, 0, 0, 13)));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());
		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// node sample
		AnimationTreeNode* itemSample = mnew AnimationTreeNode();
		sample->SetNodeSample(itemSample);
		itemSample->layout->minHeight = 25;

		auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		Text* captionLayerText = mnew Text("stdFont.ttf");
		captionLayerText->color = Color4(96, 125, 139);
		captionLayerText->horAlign = HorAlign::Left;
		captionLayerText->verAlign = VerAlign::Middle;
		itemSample->AddLayer("name", captionLayerText, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -20), Vec2F(0, 0)));

		auto nameLayer = itemSample->layer["name"];
		nameLayer->layout = Layout::BothStretch(15, 0, 5, 0);
		((Text*)nameLayer->GetDrawable())->dotsEngings = true;

		auto actorNodeEditBox = o2UI.CreateWidget<EditBox>("backless");
		actorNodeEditBox->name = "nameEditBox";
		*actorNodeEditBox->layout = WidgetLayout::BothStretch(15, 0, 5, 0);
		actorNodeEditBox->Hide(true);
		itemSample->AddChild(actorNodeEditBox);

		AnimationClip itemEditStateAnim = AnimationClip::EaseInOut("layer/name/transparency", 1.0f, 0.0f, 0.15f);
		*itemEditStateAnim.AddTrack<bool>("child/nameEditBox/enabled") = AnimationTrack<bool>::Linear(false, true, 0.15f);
		itemSample->AddState("edit", itemEditStateAnim);

		// node sample button
		Button* itemSampleExpandBtn = mnew Button();
		itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
		itemSampleExpandBtn->name = "expandBtn";

		auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mnew Sprite("ui/UI4_Right_icn.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = itemSampleExpandBtn->AddLayer("hover", mnew Sprite("ui/UI4_Right_icn_select.png"),
														 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mnew Sprite("ui/UI4_Right_icn_pressed.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		itemSampleExpandBtn->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		itemSampleExpandBtn->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->layout->anchorMin = Vec2F(0, 1);
		itemSampleExpandBtn->layout->anchorMax = Vec2F(0, 1);
		itemSampleExpandBtn->layout->offsetMin = Vec2F(0, -25);
		itemSampleExpandBtn->layout->offsetMax = Vec2F(10, 0);

		itemSample->AddChild(itemSampleExpandBtn);

		AnimationClip expandedStateAnim;
		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

		itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		itemSample->AddState("focused", focusedItemAnim);

		// scrollbars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
						 5.0f, 15.0f, 0.2f));

		sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
						 -5.0f, -15.0f, 0.2f));

		sample->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetStateForcible("hover", false);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5;

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildAnimationPropertiesTree()
	{
		// basics
		AnimationPropertiesTree* sample = mnew AnimationPropertiesTree();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 0, 1));
		sample->SetViewLayout(Layout::BothStretch(0, 0, 1, 0));
		sample->SetEnableScrollsHiding(true);
		sample->SetChildsNodesOffset(10);

		// hover
		*sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");
		sample->SetHoverLayout(Layout::BothStretch(-10, -16, -10, -16));

		// zebra back
		sample->SetZebraBackLine(mnew Sprite(Color4(0, 0, 0, 13)));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());
		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// node sample
		AnimationPropertiesTreeNode* itemSample = mnew AnimationPropertiesTreeNode();
		sample->SetNodeSample(itemSample);
		itemSample->layout->minHeight = 20;

		auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		itemSample->AddLayer("icon", mnew Sprite("ui/UI4_folder_icon.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(15, -1)));

		Text* captionLayerText = mnew Text("stdFont.ttf");
		captionLayerText->color = Color4(96, 125, 139);
		captionLayerText->horAlign = HorAlign::Left;
		captionLayerText->verAlign = VerAlign::Middle;
		itemSample->AddLayer("name", captionLayerText);

		auto nameLayer = itemSample->layer["name"];
		nameLayer->layout = Layout::BothStretch(33, 0, 5, 0);
		((Text*)nameLayer->GetDrawable())->dotsEngings = true;

		auto addButton = o2UI.CreateWidget<Button>("add small");
		addButton->name = "addButton";
		*addButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-10, 0));
		itemSample->AddChild(addButton);

		auto removeButton = o2UI.CreateWidget<Button>("remove small");
		removeButton->name = "removeButton";
		*removeButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-10, 0));
		itemSample->AddChild(removeButton);

		// node sample button
		Button* itemSampleExpandBtn = mnew Button();
		*itemSampleExpandBtn->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(15, 15), Vec2F(2, -1));
		itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
		itemSampleExpandBtn->name = "expandBtn";

		auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mnew Sprite("ui/UI4_Right_icn.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = itemSampleExpandBtn->AddLayer("hover", mnew Sprite("ui/UI4_Right_icn_select.png"),
														 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mnew Sprite("ui/UI4_Right_icn_pressed.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		itemSampleExpandBtn->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		itemSampleExpandBtn->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		itemSample->AddChild(itemSampleExpandBtn);

		AnimationClip expandedStateAnim;
		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

		itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		itemSample->AddState("focused", focusedItemAnim);

		// scrollbars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
						 5.0f, 15.0f, 0.2f));

		sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
						 -5.0f, -15.0f, 0.2f));

		sample->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetStateForcible("hover", false);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5;

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildRedEditBoxStyle()
	{
		EditBox* sample = mnew EditBox();
		sample->SetClippingLayout(Layout::BothStretch(0, 0, 10, 0));
		sample->SetViewLayout(Layout::BothStretch(3, 1, 8, -1));
		sample->SetCaretBlinkingDelay(1.15f);
		sample->SetMultiLine(false);
		sample->SetSelectionColor(Color4(0, 156, 141, 120));
		sample->layout->minSize = Vec2F(10, 10);

		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_red_edit_box.png"), Layout::BothStretch(-4, -4, -4, -4));
		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
		auto focusLayer = sample->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"), Layout::BothStretch(-9, -9, -9, -9));
		sample->AddLayer("arrows", mnew Sprite("ui/UI4_ch_arrows.png"),
						 Layout::Based(BaseCorner::Right, Vec2F(10, 20), Vec2F(0, 0)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip focusAnim = AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f);
		*focusAnim.AddTrack<float>("layer/hover/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);
		sample->AddState("focused", focusAnim)
			->offStateAnimationSpeed = 0.5f;

		Text* textDrawable = sample->GetTextDrawable();
		textDrawable->verAlign = VerAlign::Middle;
		textDrawable->horAlign = HorAlign::Left;
		textDrawable->color = Color4(96, 125, 139);
		textDrawable->SetFontAsset(FontAssetRef("stdFont.ttf"));

		Sprite* caretDrawable = sample->GetCaretDrawable();
		*caretDrawable = Sprite();
		caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetHeight()) * 1.7f);
		caretDrawable->pivot = Vec2F(0, 0.26f);
		caretDrawable->color = Color4::Black();

		o2UI.AddWidgetStyle(sample, "red singleline");
	}

	void EditorUIStyleBuilder::RebuildGreenEditBoxStyle()
	{
		EditBox* sample = mnew EditBox();
		sample->SetClippingLayout(Layout::BothStretch(0, 0, 10, 0));
		sample->SetViewLayout(Layout::BothStretch(3, 1, 8, -1));
		sample->SetCaretBlinkingDelay(1.15f);
		sample->SetMultiLine(false);
		sample->SetSelectionColor(Color4(0, 156, 141, 120));
		sample->layout->minSize = Vec2F(10, 10);

		auto backLayer = sample->AddLayer("back", mnew Sprite("ui/UI4_green_edit_box.png"), Layout::BothStretch(-4, -4, -4, -4));
		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
		auto focusLayer = sample->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"), Layout::BothStretch(-9, -9, -9, -9));
		sample->AddLayer("arrows", mnew Sprite("ui/UI4_ch_arrows.png"),
						 Layout::Based(BaseCorner::Right, Vec2F(10, 20), Vec2F(0, 0)));

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip focusAnim = AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f);
		*focusAnim.AddTrack<float>("layer/hover/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);
		sample->AddState("focused", focusAnim)
			->offStateAnimationSpeed = 0.5f;

		Text* textDrawable = sample->GetTextDrawable();
		textDrawable->verAlign = VerAlign::Middle;
		textDrawable->horAlign = HorAlign::Left;
		textDrawable->color = Color4(96, 125, 139);
		textDrawable->SetFontAsset(FontAssetRef("stdFont.ttf"));

		Sprite* caretDrawable = sample->GetCaretDrawable();
		*caretDrawable = Sprite();
		caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetHeight()) * 1.7f);
		caretDrawable->pivot = Vec2F(0, 0.26f);
		caretDrawable->color = Color4::Black();

		o2UI.AddWidgetStyle(sample, "green singleline");
	}

	void EditorUIStyleBuilder::RebuildPreviewMenuToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto regular = sample->AddLayer("regular", nullptr);

		auto regularBackLayer = regular->AddChildLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto regularSelectLayer = regular->AddChildLayer("selectBack", mnew Sprite("ui/UI4_panel_button_select.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto regularPressedLayer = regular->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_panel_button_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto toggled = sample->AddLayer("toggled", nullptr);

		auto toggledBackLayer = toggled->AddChildLayer("regularBack", mnew Sprite("ui/UI4_menu_toggled.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto toggledSelectLayer = toggled->AddChildLayer("selectBack", mnew Sprite("ui/UI4_menu_toggled_hover.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto toggledPressedLayer = toggled->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_menu_toggled_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto recordIcon = sample->AddLayer("playIcon", mnew Sprite("ui/UI4_pencil_icon_green.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 1)));

		// hover
		AnimationClip hoverAnim = AnimationClip::EaseInOut("layer/regular/child/selectBack/transparency", 0.0f, 1.0f, 0.1f);
		*hoverAnim.AddTrack<float>("layer/toggled/child/selectBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", hoverAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// pressed
		AnimationClip pressedAnim = AnimationClip::EaseInOut("layer/regular/child/pressedBack/transparency", 0.0f, 1.0f, 0.1f);
		*pressedAnim.AddTrack<float>("layer/toggled/child/pressedBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("pressed", pressedAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// focused
		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		// value
		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/regular/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/toggled/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "menu preview");
	}

	void EditorUIStyleBuilder::RebuildRecordMenuToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto regular = sample->AddLayer("regular", nullptr);

		auto regularBackLayer = regular->AddChildLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto regularSelectLayer = regular->AddChildLayer("selectBack", mnew Sprite("ui/UI4_panel_button_select.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto regularPressedLayer = regular->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_panel_button_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto toggled = sample->AddLayer("toggled", nullptr);

		auto toggledBackLayer = toggled->AddChildLayer("regularBack", mnew Sprite("ui/UI4_menu_toggled.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto toggledSelectLayer = toggled->AddChildLayer("selectBack", mnew Sprite("ui/UI4_menu_toggled_hover.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto toggledPressedLayer = toggled->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_menu_toggled_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto recordIcon = sample->AddLayer("playIcon", mnew Sprite("ui/UI4_Record_icon.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 1)));

		// hover
		AnimationClip hoverAnim = AnimationClip::EaseInOut("layer/regular/child/selectBack/transparency", 0.0f, 1.0f, 0.1f);
		*hoverAnim.AddTrack<float>("layer/toggled/child/selectBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", hoverAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// pressed
		AnimationClip pressedAnim = AnimationClip::EaseInOut("layer/regular/child/pressedBack/transparency", 0.0f, 1.0f, 0.1f);
		*pressedAnim.AddTrack<float>("layer/toggled/child/pressedBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("pressed", pressedAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// focused
		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		// value
		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/regular/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/toggled/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "menu record");
	}

	void EditorUIStyleBuilder::RebuildRewindLeftMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_rewind_left.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu rewind left");
	}

	void EditorUIStyleBuilder::RebuildMoveLeftMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_move_left.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu move left");
	}

	void EditorUIStyleBuilder::RebuildPlayStopMenuToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto playLayer = sample->AddLayer("playIcon", mnew Sprite("ui/UI4_anim_play.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 1)));

		auto pauseLayer = sample->AddLayer("pauseIcon", mnew Sprite("ui/UI4_anim_pause.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/pauseIcon/transparency", 0.0f, 1.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/playIcon/transparency") =
			AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "menu play-stop");
	}

	void EditorUIStyleBuilder::RebuildRewindRightMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_rewind_right.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu rewind right");
	}

	void EditorUIStyleBuilder::RebuildMoveRightMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_move_right.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu move right");
	}

	void EditorUIStyleBuilder::RebuildLoopMenuToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto loopLayer = sample->AddLayer("loop", mnew Sprite("ui/UI4_anim_loop.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto nonLoopLayer = sample->AddLayer("nonLoop", mnew Sprite("ui/UI4_anim_not_loop.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/nonLoop/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/loop/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "menu loop-nonloop");
	}

	void EditorUIStyleBuilder::RebuildAddKeyMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_add_key.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu add key");
	}

	void EditorUIStyleBuilder::RebuildCurvesMenuToggle()
	{
		Toggle* sample = mnew Toggle();
		sample->layout->minSize = Vec2F(20, 20);

		auto regular = sample->AddLayer("regular", nullptr);

		auto regularBackLayer = regular->AddChildLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto regularSelectLayer = regular->AddChildLayer("selectBack", mnew Sprite("ui/UI4_panel_button_select.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto regularPressedLayer = regular->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_panel_button_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto toggled = sample->AddLayer("toggled", nullptr);

		auto toggledBackLayer = toggled->AddChildLayer("regularBack", mnew Sprite("ui/UI4_menu_toggled.png"),
													   Layout::BothStretch(-4, -4, -5, -5));

		auto toggledSelectLayer = toggled->AddChildLayer("selectBack", mnew Sprite("ui/UI4_menu_toggled_hover.png"),
														 Layout::BothStretch(-4, -4, -5, -5));

		auto toggledPressedLayer = toggled->AddChildLayer("pressedBack", mnew Sprite("ui/UI4_menu_toggled_pressed.png"),
														  Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto icon = sample->AddLayer("playIcon", mnew Sprite("ui/UI4_curves_mode.png"),
									 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 1)));

		// hover
		AnimationClip hoverAnim = AnimationClip::EaseInOut("layer/regular/child/selectBack/transparency", 0.0f, 1.0f, 0.1f);
		*hoverAnim.AddTrack<float>("layer/toggled/child/selectBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", hoverAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// pressed
		AnimationClip pressedAnim = AnimationClip::EaseInOut("layer/regular/child/pressedBack/transparency", 0.0f, 1.0f, 0.1f);
		*pressedAnim.AddTrack<float>("layer/toggled/child/pressedBack/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("pressed", pressedAnim)->offStateAnimationSpeed = 1.0f / 4.0f;

		// focused
		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		// value
		AnimationClip valueBtnAnim = AnimationClip::EaseInOut("layer/regular/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnAnim.AddTrack<float>("layer/toggled/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnAnim);

		o2UI.AddWidgetStyle(sample, "menu curves");
	}

	void EditorUIStyleBuilder::RebuildPropertiesMenuButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto backLayer = sample->AddLayer("regularBack", mnew Sprite("ui/UI4_panel_button.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto selectLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_panel_button_select.png"),
											Layout::BothStretch(-4, -4, -5, -5));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_panel_button_pressed.png"),
											 Layout::BothStretch(-4, -4, -5, -5));

		auto focusLayer = sample->AddLayer("focused", mnew Sprite("ui/UI4_panel_button_focus.png"),
										   Layout::BothStretch(-4, -4, -5, -5));

		auto iconLayer = sample->AddLayer("icon", mnew Sprite("ui/UI4_anim_properties_icon.png"),
										  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 1)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "menu properties");
	}

	void EditorUIStyleBuilder::RebuildAddKeyButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(20, 20);

		auto basicLayer = sample->AddLayer("basic", nullptr);

		auto backLayer = basicLayer->AddChildLayer("regularBack", mnew Sprite("ui/UI4_add_key.png"),
												   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 0)));

		auto selectLayer = basicLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_add_key_hover.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 0)));

		auto pressedLayer = basicLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_add_key_pressed.png"),
													  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(-1, 0)));

		sample->AddState("inactive", AnimationClip::EaseInOut("layer/basic/transparency", 1.0f, 0.5f, 0.01f));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/basic/child/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/basic/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "add key");
	}

	void EditorUIStyleBuilder::RebuildAddKeyDotButton()
	{
		Button* sample = mnew Button();

		auto basicLayer = sample->AddLayer("basic", nullptr);

		auto backLayer = basicLayer->AddChildLayer("regularBack", mnew Sprite("ui/UI4_curve_color.png"),
												   Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto selectLayer = basicLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_curve_color.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		auto pressedLayer = basicLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_curve_color.png"),
													  Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, 0)));

		sample->AddState("inactive", AnimationClip::EaseInOut("layer/basic/transparency", 1.0f, 0.5f, 0.01f));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/basic/child/hover/transparency", 0.0f, 0.5f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/basic/child/pressed/transparency", 0.0f, 0.2f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "add dot key");
	}

	void EditorUIStyleBuilder::RebuildAddSmallBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_plus_small_regular.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_plus_small_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_plus_small_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "add small");
	}

	void EditorUIStyleBuilder::RebuildRemoveSmallBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_minus_small_regular.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_minus_small_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_minus_small_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "remove small");
	}

	void EditorUIStyleBuilder::RebuildCreateAssetInstanceBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_create_asset_instance.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_create_asset_instance_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_create_asset_instance_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.05f));

		o2UI.AddWidgetStyle(sample, "create asset instance");
	}

	void EditorUIStyleBuilder::RebuildRemoveAssetInstanceBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_remove_asset_instance.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_remove_asset_instance_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_remove_asset_instance_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.05f));

		o2UI.AddWidgetStyle(sample, "remove asset instance");
	}

	void EditorUIStyleBuilder::RebuildSaveAssetInstanceBtn()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_save_asset_instance.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_save_asset_instance_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_save_asset_instance_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.05f));

		o2UI.AddWidgetStyle(sample, "save asset instance");
	}

	void EditorUIStyleBuilder::RebuildSpoilerWithHead()
	{
		auto sample = mnew SpoilerWithHead();
		sample->SetHeadHeight(18);
		sample->spacing = 5.0f;
		sample->borderLeft = 10;
		sample->borderTop = 5.0f;
		sample->expandHeight = false;
		sample->expandWidth = true;
		sample->fitByChildren = true;
		sample->baseCorner = BaseCorner::RightTop;

		sample->AddLayer("back", mnew Sprite("ui/UI4_component_head.png"),
						 Layout::HorStretch(VerAlign::Top, -1, -3, 25, -2));

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "Button";
		captionText->horAlign = HorAlign::Left;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->wordWrap = true;
		captionText->color = Color4(235, 255, 253);
		sample->AddLayer("caption", captionText, Layout::HorStretch(VerAlign::Top, 41, 0, 20, 0));

		auto expandBtn = o2UI.CreateWidget<Button>("expand white");
		expandBtn->name = "expand";
		*expandBtn->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(1, 0));
		sample->AddInternalWidget(expandBtn);

		auto icon = o2UI.CreateImage("ui/UI4_transform_icon.png");
		icon->name = "icon";
		*icon->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(21, 0));
		sample->AddInternalWidget(icon);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildAddComponentButton()
	{
		Button* sample = mnew Button();

		auto basicLayer = sample->AddLayer("basic", nullptr);

		auto backLayer = basicLayer->AddChildLayer("regularBack", mnew Sprite("ui/UI4_add_btn.png"),
												   Layout::Based(BaseCorner::Center, Vec2F(40, 40), Vec2F(0, 0)));

		auto selectLayer = basicLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_add_btn_hover.png"),
													 Layout::Based(BaseCorner::Center, Vec2F(40, 40), Vec2F(-1, 0)));

		auto pressedLayer = basicLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_add_btn_pressed.png"),
													  Layout::Based(BaseCorner::Center, Vec2F(40, 40), Vec2F(0, 0)));

		sample->AddState("inactive", AnimationClip::EaseInOut("layer/basic/transparency", 1.0f, 0.5f, 0.01f));

		sample->AddState("hover", AnimationClip::EaseInOut("layer/basic/child/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/basic/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "add component");
	}

	void EditorUIStyleBuilder::RebuildAddComponentsTree()
	{
		// basics
		ComponentsTree* sample = mnew ComponentsTree();
		sample->layout->minSize = Vec2F(20, 20);
		sample->SetClippingLayout(Layout::BothStretch(1, 2, 0, 1));
		sample->SetViewLayout(Layout::BothStretch(0, 0, 1, 0));
		sample->SetEnableScrollsHiding(true);
		sample->SetChildsNodesOffset(10);

		// Back
		sample->AddLayer("back", mnew Sprite(Color4(0, 0, 0, 50)), Layout::BothStretch())->transparency = 0.05f;

		// hover
		*sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");
		sample->SetHoverLayout(Layout::BothStretch(-10, -16, -10, -16));

		// zebra back
		sample->SetZebraBackLine(mnew Sprite(Color4(0, 0, 0, 13)));

		// highlight
		*sample->GetHighlightDrawable() = Sprite("ui/UI4_selection_frame.png");
		sample->GetHighlightDrawable()->pivot = Vec2F(0.5f, 0.5f);
		sample->SetHighlightLayout(Layout::BothStretch());
		sample->SetHighlightAnimation(Animate(*sample->GetHighlightDrawable()).
									  Hide().Scale(1.5f).Then().
									  Wait(0.3f).Then().
									  Show().Scale(1.0f).For(0.2f).Then().
									  Wait(1.0f).Then().
									  Hide().For(0.2f));

		// node sample
		ComponentsTreeNode* itemSample = mnew ComponentsTreeNode();
		sample->SetNodeSample(itemSample);
		itemSample->layout->minHeight = 30;

		auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

		auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mnew Sprite("ui/UI4_Context_menu_select.png"),
																  Layout::BothStretch(-10, -16, -10, -16));

		auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mnew Sprite("ui/UI4_Context_menu_white.png"),
																	Layout::BothStretch(-10, -16, -10, -16));

		itemSample->AddLayer("icon", mnew Sprite("ui/UI4_folder_icon.png"), Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(15, -1)));

		Text* captionLayerText = mnew Text("stdFont.ttf");
		captionLayerText->color = Color4(96, 125, 139);
		captionLayerText->horAlign = HorAlign::Left;
		captionLayerText->verAlign = VerAlign::Middle;
		itemSample->AddLayer("name", captionLayerText);

		auto nameLayer = itemSample->layer["name"];
		nameLayer->layout = Layout::BothStretch(35, 0, 5, 0);
		((Text*)nameLayer->GetDrawable())->dotsEngings = true;

		// node sample button
		Button* itemSampleExpandBtn = mnew Button();
		*itemSampleExpandBtn->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(15, 15), Vec2F(2, -1));
		itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
		itemSampleExpandBtn->name = "expandBtn";

		auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mnew Sprite("ui/UI4_Right_icn.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto selectLayer = itemSampleExpandBtn->AddLayer("hover", mnew Sprite("ui/UI4_Right_icn_select.png"),
														 Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

		auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mnew Sprite("ui/UI4_Right_icn_pressed.png"),
														  Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


		itemSampleExpandBtn->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		itemSampleExpandBtn->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		itemSampleExpandBtn->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		itemSample->AddChild(itemSampleExpandBtn);

		AnimationClip expandedStateAnim;
		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		*expandedStateAnim.AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") =
			AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

		itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

		itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

		AnimationClip focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
		*focusedItemAnim.AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(0.3f, 0.0f, 0.2f);
		itemSample->AddState("focused", focusedItemAnim);

		// scrollbars
		HorizontalScrollBar* horScrollBar = o2UI.CreateHorScrollBar();
		horScrollBar->layout->anchorMin = Vec2F(0, 0);
		horScrollBar->layout->anchorMax = Vec2F(1, 0);
		horScrollBar->layout->offsetMin = Vec2F(5, 0);
		horScrollBar->layout->offsetMax = Vec2F(-15, 15);
		sample->SetHorizontalScrollBar(horScrollBar);

		VerticalScrollBar* verScrollBar = o2UI.CreateVerScrollBar();
		verScrollBar->layout->anchorMin = Vec2F(1, 0);
		verScrollBar->layout->anchorMax = Vec2F(1, 1);
		verScrollBar->layout->offsetMin = Vec2F(-15, 15);
		verScrollBar->layout->offsetMax = Vec2F(0, -5);
		sample->SetVerticalScrollBar(verScrollBar);

		sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
						 5.0f, 15.0f, 0.2f));

		sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
						 -5.0f, -15.0f, 0.2f));

		sample->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetStateForcible("hover", false);

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5;

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildAnimationStateViewerEditButton()
	{
		Button* sample = mnew Button();
		sample->layout->minSize = Vec2F(5, 5);

		auto regularLayer = sample->AddLayer("regular", mnew Sprite("ui/UI4_edit_anim_regular.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto hoverLayer = sample->AddLayer("hover", mnew Sprite("ui/UI4_edit_anim_hover.png"),
										   Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto pressedLayer = sample->AddLayer("pressed", mnew Sprite("ui/UI4_edit_anim_pressed.png"),
											 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


		sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		o2UI.AddWidgetStyle(sample, "edit animation state");
	}

	void EditorUIStyleBuilder::RebuildAnimationSpoiler()
	{
		auto sample = mnew SpoilerWithHead();
		sample->SetHeadHeight(30);
		sample->spacing = 5.0f;
		sample->borderLeft = 10;
		sample->borderTop = 5.0f;
		sample->expandHeight = false;
		sample->expandWidth = true;
		sample->fitByChildren = true;
		sample->baseCorner = BaseCorner::RightTop;

		Text* captionText = mnew Text("stdFont.ttf");
		captionText->text = "Button";
		captionText->horAlign = HorAlign::Left;
		captionText->verAlign = VerAlign::Middle;
		captionText->dotsEngings = true;
		captionText->wordWrap = true;
		captionText->color = Color4(235, 255, 253);
		sample->AddLayer("caption", captionText, Layout::HorStretch(VerAlign::Top, 41, 0, 20, 0));

		auto expandBtn = o2UI.CreateWidget<Button>("expand");
		expandBtn->name = "expand";
		*expandBtn->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(1, 0));
		sample->AddInternalWidget(expandBtn);

		auto playStopBtn = o2UI.CreateWidget<Toggle>("animation component play-stop");
		playStopBtn->name = "play-stop";
		*playStopBtn->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(20, 0));
		sample->AddInternalWidget(playStopBtn);

		auto removeBtn = o2UI.CreateWidget<Button>("remove small");
		removeBtn->name = "expand";
		*removeBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(0, 0));
		sample->AddInternalWidget(removeBtn);

		auto loopBtn = o2UI.CreateWidget<Toggle>("animation component loop");
		loopBtn->name = "loop";
		*loopBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-20, 0));
		sample->AddInternalWidget(loopBtn);

		auto bar = o2UI.CreateWidget<HorizontalProgress>("animation component bar");
		bar->name = "bar";
		*bar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 2, 20);
		sample->AddInternalWidget(bar);

		o2UI.AddWidgetStyle(sample, "animation state");
	}

	void EditorUIStyleBuilder::RebuildAnimationStateViewerPlayToggle()
	{
		Toggle* sample = mnew Toggle();
		auto playRootIconLayer = sample->AddLayer("playRootIcon", nullptr);
		auto stopRootIconLayer = sample->AddLayer("stopRootIcon", nullptr);

		auto playIconLayer = playRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_play_anim_regular.png"),
															  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto playSelectIconLayer = playRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_play_anim_hover.png"),
																	Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto playPressedIconLayer = playRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_play_anim_pressed.png"),
																	 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopIconLayer = stopRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_stop_anim_regular.png"),
															  Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopSelectIconLayer = stopRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_stop_anim_hover.png"),
																	Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		auto stopPressedIconLayer = stopRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_stop_anim_pressed.png"),
																	 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		AnimationClip playBtnSelectAnim = AnimationClip::EaseInOut("layer/playRootIcon/child/hover/transparency", 0.0f, 1.0f, 0.1f);
		*playBtnSelectAnim.AddTrack<float>("layer/stopRootIcon/child/hover/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", playBtnSelectAnim)->offStateAnimationSpeed = 0.25f;

		AnimationClip playBtnPressAnim = AnimationClip::EaseInOut("layer/playRootIcon/child/pressed/transparency", 0.0f, 1.0f, 0.1f);
		*playBtnPressAnim.AddTrack<float>("layer/stopRootIcon/child/pressed/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);

		sample->AddState("pressed", playBtnPressAnim)->offStateAnimationSpeed = 0.5f;

		AnimationClip valueBtnPressAnim = AnimationClip::EaseInOut("layer/playRootIcon/transparency", 1.0f, 0.0f, 0.1f);
		*valueBtnPressAnim.AddTrack<float>("layer/stopRootIcon/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("value", valueBtnPressAnim);

		o2UI.AddWidgetStyle(sample, "animation state play-stop");
	}

	void EditorUIStyleBuilder::RebuildAnimationStateViewerLoopToggle()
	{
		Toggle* sample = mnew Toggle();
		auto loopRootIconLayer = sample->AddLayer("loopRootIcon", nullptr);
		auto nonLoopRootIconLayer = sample->AddLayer("nonLoopRootIcon", nullptr);

		loopRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_anim_loop_regular.png"),
										 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		loopRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_anim_loop_hover.png"),
										 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		loopRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_anim_loop_pressed.png"),
										 Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

		nonLoopRootIconLayer->AddChildLayer("regular", mnew Sprite("ui/UI4_non_loop_anim_regular.png"),
											Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, -1)));

		nonLoopRootIconLayer->AddChildLayer("hover", mnew Sprite("ui/UI4_non_loop_anim_hover.png"),
											Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, -1)));

		nonLoopRootIconLayer->AddChildLayer("pressed", mnew Sprite("ui/UI4_non_loop_anim_pressed.png"),
											Layout::Based(BaseCorner::Center, Vec2F(20, 20), Vec2F(0, -1)));

		AnimationClip hoverAnim = AnimationClip::EaseInOut("layer/loopRootIcon/child/hover/transparency", 0.0f, 1.0f, 0.1f);
		*hoverAnim.AddTrack<float>("layer/nonLoopRootIcon/child/hover/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.1f);

		sample->AddState("hover", hoverAnim)->offStateAnimationSpeed = 0.25f;

		AnimationClip pressedAnim = AnimationClip::EaseInOut("layer/loopRootIcon/child/pressed/transparency", 0.0f, 1.0f, 0.1f);
		*pressedAnim.AddTrack<float>("layer/nonLoopRootIcon/child/pressed/transparency") =
			AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);

		sample->AddState("pressed", pressedAnim)->offStateAnimationSpeed = 0.5f;

		AnimationClip valueAnim = AnimationClip::EaseInOut("layer/loopRootIcon/transparency", 0.0f, 1.0f, 0.1f);
		*valueAnim.AddTrack<float>("layer/nonLoopRootIcon/transparency") =
			AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.1f);

		sample->AddState("value", valueAnim);

		o2UI.AddWidgetStyle(sample, "animation state loop");
	}

	void EditorUIStyleBuilder::RebuildAnimationStateViewerProgressBar()
	{
		HorizontalProgress* sample = mnew HorizontalProgress();
		sample->layout->minSize = Vec2F(5, 5);
		auto backLayer = sample->AddLayer("back", nullptr);
		auto spriteBackLayer = backLayer->AddChildLayer("sprite", mnew Sprite(Color4(181, 203, 204)), Layout::BothStretch());

		backLayer->interactableLayout = Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(0, -4), Vec2F(0, 4));

		auto barLayer = sample->AddLayer("bar", nullptr);
		auto barRegularSprite = barLayer->AddChildLayer("regular", mnew Sprite(Color4(248, 93, 72)), Layout::BothStretch());

		auto barSelectSprite = barLayer->AddChildLayer("hover", mnew Sprite(Color4(248, 93, 72).ChangeLightness(0.1f)),
													   Layout::BothStretch());

		auto barPressedSprite = barLayer->AddChildLayer("pressed", mnew Sprite(Color4(248, 93, 72).ChangeLightness(-0.1f)),
														Layout::BothStretch());

		sample->AddState("hover", AnimationClip::EaseInOut("layer/bar/child/hover/transparency", 0.0f, 1.0f, 0.1f))
			->offStateAnimationSpeed = 1.0f / 4.0f;

		sample->AddState("pressed", AnimationClip::EaseInOut("layer/bar/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
			->offStateAnimationSpeed = 0.5f;

		sample->SetOrientation(HorizontalProgress::Orientation::Right);

		o2UI.AddWidgetStyle(sample, "animation state bar");
	}

	void EditorUIStyleBuilder::RebuildFloatProperty()
	{
		auto sample = mnew FloatProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto editBox = o2UI.CreateEditBox("singleline with arrows");
		editBox->name = "editBox";
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildRedFloatProperty()
	{
		auto sample = mnew FloatProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto editBox = o2UI.CreateEditBox("red singleline");
		editBox->name = "editBox";
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "red");
	}

	void EditorUIStyleBuilder::RebuildGreenFloatProperty()
	{
		auto sample = mnew FloatProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto editBox = o2UI.CreateEditBox("green singleline");
		editBox->name = "editBox";
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "green");
	}

	void EditorUIStyleBuilder::RebuildIntegerProperty()
	{
		auto sample = mnew IntegerProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto editBox = o2UI.CreateEditBox("singleline with arrows");
		editBox->name = "editBox";
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildActorProperty()
	{
		auto sample = mnew ActorProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto box = mnew Widget();
		box->name = "box";
		box->SetFocusable(true);
		*box->layout = WidgetLayout::BothStretch();

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
									   Layout::BothStretch(-9, -9, -9, -9));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										 Layout::BothStretch(-9, -9, -9, -9));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"),
										Layout::BothStretch(-9, -9, -9, -9));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		auto nameText = mnew Text("stdFont.ttf");
		nameText->text = "--";
		nameText->horAlign = HorAlign::Left;
		nameText->verAlign = VerAlign::Middle;
		nameText->dotsEngings = true;
		nameText->color = Color4(96, 125, 139);
		box->AddLayer("caption", nameText, Layout::BothStretch(2, 2, 2, 2));

		box->SetFocusable(true);

		auto linkBtn = o2UI.CreateWidget<Button>("asset link");
		*linkBtn->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(15, 15), Vec2F());
		box->AddChild(linkBtn);
		layout->AddChild(box);

		o2UI.AddWidgetStyle(sample, "standard");
	}


	void EditorUIStyleBuilder::RebuildAssetPropety()
	{
		auto sample = mnew AssetProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = true;

		auto spoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
		spoiler->name = "spoiler";
		spoiler->RemoveLayer("caption");
		*spoiler->layout = WidgetLayout::BothStretch();
		sample->AddChild(spoiler);

		auto mainLayout = mnew HorizontalLayout();
		mainLayout->name = "mainLayout";
		*mainLayout->layout = WidgetLayout::HorStretch(VerAlign::Top, 10, 0, 19, 0);
		mainLayout->expandHeight = true;
		spoiler->AddInternalWidget(mainLayout);

		auto label = o2UI.CreateWidget<Label>();
		label->name = "propertyName";
		label->horAlign = HorAlign::Left;
		label->layout->widthWeight = 0.6f;
		label->horOverflow = Label::HorOverflow::Dots;
		mainLayout->AddChild(label);

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		mainLayout->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 19, 0);
		layout->baseCorner = BaseCorner::Right;
		layout->expandHeight = false;
		layout->spacing = 2;
		layoutContainer->AddChild(layout);

		auto instanceCaption = o2UI.CreateLabel("instance");
		*instanceCaption->layout = WidgetLayout::BothStretch(0, 5, 0, 0);
		instanceCaption->name = "instanceCaption";
		instanceCaption->horOverflow = Label::HorOverflow::Dots;
		instanceCaption->horAlign = HorAlign::Left;
		instanceCaption->verAlign = VerAlign::Bottom;
		instanceCaption->height = 8;
		instanceCaption->transparency = 0.6f;
		layout->AddInternalWidget(instanceCaption);

		auto createInstanceBtn = o2UI.CreateWidget<Button>("create asset instance");
		createInstanceBtn->name = "create";
		createInstanceBtn->layout->minHeight = 20;
		createInstanceBtn->layout->maxWidth = 20;
		layout->AddChild(createInstanceBtn);

		auto removeInstanceBtn = o2UI.CreateWidget<Button>("remove asset instance");
		removeInstanceBtn->name = "remove";
		removeInstanceBtn->layout->minHeight = 20;
		removeInstanceBtn->layout->maxWidth = 16;
		layout->AddChild(removeInstanceBtn);

		auto saveInstanceBtn = o2UI.CreateWidget<Button>("save asset instance");
		saveInstanceBtn->name = "save";
		saveInstanceBtn->layout->minHeight = 20;
		saveInstanceBtn->layout->maxWidth = 16;
		layout->AddChild(saveInstanceBtn);

		auto box = mnew Widget();
		box->name = "box";
		box->SetFocusable(true);
		box->layout->minHeight = 19;
		layout->AddChild(box);

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
									   Layout::BothStretch(-9, -9, -9, -9));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										 Layout::BothStretch(-9, -9, -9, -9));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"),
										Layout::BothStretch(-9, -9, -9, -9));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		auto nameText = mnew Text("stdFont.ttf");
		nameText->text = "--";
		nameText->horAlign = HorAlign::Left;
		nameText->verAlign = VerAlign::Middle;
		nameText->dotsEngings = true;
		nameText->color = Color4(96, 125, 139);;
		box->AddLayer("caption", nameText, Layout::BothStretch(2, 2, 2, 2));

		box->SetFocusable(true);

		AnimationClip instanceAnim;
		*instanceAnim.AddTrack<float>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/internalWidget/instanceCaption/transparency") =
			AnimationTrack<float>::EaseInOut(0, 1, 0.2f);

		*instanceAnim.AddTrack<float>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/box/transparency") =
			AnimationTrack<float>::EaseInOut(1, 0, 0.2f);

		*instanceAnim.AddTrack<bool>("child/spoiler/internalWidget/expand/enabled") =
			AnimationTrack<bool>::EaseInOut(false, true, 0.2f);

		*instanceAnim.AddTrack<float>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/create/layout/maxWidth") =
			AnimationTrack<float>::EaseInOut(16, 0, 0.2f);

		auto createAnim = instanceAnim.AddTrack<bool>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/create/enabled");
		createAnim->AddKey(0, true);
		createAnim->AddKey(0.05f, true);
		createAnim->AddKey(0.055f, false);
		createAnim->AddKey(0.2f, false);

		*instanceAnim.AddTrack<float>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/save/layout/maxWidth") =
			AnimationTrack<float>::EaseInOut(0, 16, 0.2f);

		auto saveAnim = instanceAnim.AddTrack<bool>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/save/enabled");
		saveAnim->AddKey(0, false);
		saveAnim->AddKey(0.1f, false);
		saveAnim->AddKey(0.105f, true);
		saveAnim->AddKey(0.2f, true);

		*instanceAnim.AddTrack<float>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/remove/layout/maxWidth") =
			AnimationTrack<float>::EaseInOut(0, 16, 0.2f);

		auto removeAnim = instanceAnim.AddTrack<bool>("child/spoiler/internalWidget/mainLayout/child/container/child/layout/child/remove/enabled");
		removeAnim->AddKey(0, false);
		removeAnim->AddKey(0.1f, false);
		removeAnim->AddKey(0.105f, true);
		removeAnim->AddKey(0.2f, true);

		sample->AddState("instance", instanceAnim);

		// 		auto linkBtn = o2UI.CreateWidget<Button>("asset link");
		// 		*linkBtn->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(15, 15), Vec2F());
		// 		box->AddChild(linkBtn);

		o2UI.AddWidgetStyle(sample, "standard");
		o2UI.AddWidgetStyle(sample->CloneAs<AssetProperty>(), "with caption");
	}

	void EditorUIStyleBuilder::RebuildSceneLayersListProperty()
	{
		auto sample = mnew SceneLayersListProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto box = mnew Button();
		box->name = "box";
		box->SetFocusable(true);
		*box->layout = WidgetLayout::BothStretch();

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
									   Layout::BothStretch(-9, -9, -9, -9));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										 Layout::BothStretch(-9, -9, -9, -9));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"),
										Layout::BothStretch(-9, -9, -9, -9));

		auto pressedLayer = box->AddLayer("pressed", mnew Sprite("ui/UI4_Editbox_pressed.png"),
										  Layout::BothStretch(-4, -4, -5, -5));

		auto arrowLayer = box->AddLayer("arrow", mnew Sprite("ui/UI4_Down_icn.png"),
										Layout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0)));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		auto nameText = mnew Text("stdFont.ttf");
		nameText->text = "--";
		nameText->horAlign = HorAlign::Left;
		nameText->verAlign = VerAlign::Middle;
		nameText->dotsEngings = true;
		nameText->color = Color4(96, 125, 139);
		box->AddLayer("caption", nameText, Layout::BothStretch(2, 2, 2, 2));

		box->SetFocusable(true);
		layout->AddChild(box);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildBoolProperty()
	{
		auto sample = mnew BooleanProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		Toggle* toggle = o2UI.CreateToggle("", "without caption");
		toggle->name = "toggle";
		toggle->layout->maxWidth = 20;
		layout->AddChild(toggle);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildBorderFProperty()
	{
		auto sample = mnew BorderFProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto leftLabel = o2UI.CreateLabel("L");
		leftLabel->name = "left label";
		leftLabel->horOverflow = Label::HorOverflow::None;
		*leftLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(leftLabel);

		auto leftProperty = o2UI.CreateWidget<FloatProperty>();
		leftProperty->name = "left";
		*leftProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.25f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(leftProperty);

		auto topLabel = o2UI.CreateLabel("T");
		topLabel->name = "top label";
		topLabel->horOverflow = Label::HorOverflow::None;
		*topLabel->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.25f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(topLabel);

		auto topProperty = o2UI.CreateWidget<FloatProperty>();
		topProperty->name = "top";
		*topProperty->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(topProperty);

		auto rightLabel = o2UI.CreateLabel("R");
		rightLabel->name = "right label";
		rightLabel->horOverflow = Label::HorOverflow::None;
		*rightLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(rightLabel);

		auto rightProperty = o2UI.CreateWidget<FloatProperty>();
		rightProperty->name = "right";
		*rightProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.75f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(rightProperty);

		auto bottomLabel = o2UI.CreateLabel("B");
		bottomLabel->name = "bottom label";
		bottomLabel->horOverflow = Label::HorOverflow::None;
		*bottomLabel->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(0.75f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(bottomLabel);

		auto bottomProperty = o2UI.CreateWidget<FloatProperty>();
		bottomProperty->name = "bottom";
		*bottomProperty->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(bottomProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildBorderIProperty()
	{
		auto sample = mnew BorderIProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto leftLabel = o2UI.CreateLabel("L");
		leftLabel->name = "left label";
		leftLabel->horOverflow = Label::HorOverflow::None;
		*leftLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(leftLabel);

		auto leftProperty = o2UI.CreateWidget<IntegerProperty>();
		leftProperty->name = "left";
		*leftProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.25f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(leftProperty);

		auto topLabel = o2UI.CreateLabel("T");
		topLabel->name = "top label";
		topLabel->horOverflow = Label::HorOverflow::None;
		*topLabel->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.25f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(topLabel);

		auto topProperty = o2UI.CreateWidget<IntegerProperty>();
		topProperty->name = "top";
		*topProperty->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(topProperty);

		auto rightLabel = o2UI.CreateLabel("R");
		rightLabel->name = "right label";
		rightLabel->horOverflow = Label::HorOverflow::None;
		*rightLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(rightLabel);

		auto rightProperty = o2UI.CreateWidget<IntegerProperty>();
		rightProperty->name = "right";
		*rightProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.75f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(rightProperty);

		auto bottomLabel = o2UI.CreateLabel("B");
		bottomLabel->name = "bottom label";
		bottomLabel->horOverflow = Label::HorOverflow::None;
		*bottomLabel->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(0.75f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(bottomLabel);

		auto bottomProperty = o2UI.CreateWidget<IntegerProperty>();
		bottomProperty->name = "bottom";
		*bottomProperty->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(bottomProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildColorPropety()
	{
		auto sample = mnew ColorProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto box = mnew Widget();
		box->name = "box";
		*box->layout = WidgetLayout::BothStretch();
		box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
					  Layout::BothStretch(-9, -9, -9, -9));

		layout->AddChild(box);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildComponentProperty()
	{
		auto sample = mnew ComponentProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto box = mnew Widget();
		box->name = "box";
		box->SetFocusable(true);
		*box->layout = WidgetLayout::BothStretch();

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
									   Layout::BothStretch(-9, -9, -9, -9));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										 Layout::BothStretch(-9, -9, -9, -9));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"),
										Layout::BothStretch(-9, -9, -9, -9));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		auto nameText = mnew Text("stdFont.ttf");
		nameText->text = "--";
		nameText->horAlign = HorAlign::Left;
		nameText->verAlign = VerAlign::Middle;
		nameText->dotsEngings = true;
		nameText->color = Color4(96, 125, 139);
		box->AddLayer("caption", nameText, Layout::BothStretch(2, 2, 2, 2));
		layout->AddChild(box);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildCurveProperty()
	{
		auto sample = mnew CurveProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto box = mnew Widget();
		box->name = "box";
		box->SetFocusable(true);
		*box->layout = WidgetLayout::BothStretch();

		auto backLayer = box->AddLayer("back", mnew Sprite("ui/UI4_Editbox_regular.png"),
									   Layout::BothStretch(-9, -9, -9, -9));

		auto selectLayer = box->AddLayer("hover", mnew Sprite("ui/UI4_Editbox_select.png"),
										 Layout::BothStretch(-9, -9, -9, -9));

		auto focusLayer = box->AddLayer("focus", mnew Sprite("ui/UI4_Editbox_focus.png"),
										Layout::BothStretch(-9, -9, -9, -9));

		box->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		box->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
			->offStateAnimationSpeed = 0.5f;

		layout->AddChild(box);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildEnumProperty()
	{
		auto sample = mnew EnumProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		DropDown* dropDown = o2UI.CreateDropdown();
		dropDown->name = "dropdown";
		*dropDown->layout = WidgetLayout::BothStretch();
		layout->AddChild(dropDown);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildLayerProperty()
	{
		auto sample = mnew LayerProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		DropDown* dropDown = o2UI.CreateDropdown();
		dropDown->name = "dropdown";
		*dropDown->layout = WidgetLayout::BothStretch();
		layout->AddChild(dropDown);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildRectFProperty()
	{
		auto sample = mnew RectFProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto leftLabel = o2UI.CreateLabel("L");
		leftLabel->name = "left label";
		leftLabel->horOverflow = Label::HorOverflow::None;
		*leftLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(leftLabel);

		auto leftProperty = o2UI.CreateWidget<FloatProperty>();
		leftProperty->name = "left";
		*leftProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.25f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(leftProperty);

		auto topLabel = o2UI.CreateLabel("T");
		topLabel->name = "top label";
		topLabel->horOverflow = Label::HorOverflow::None;
		*topLabel->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.25f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(topLabel);

		auto topProperty = o2UI.CreateWidget<FloatProperty>();
		topProperty->name = "top";
		*topProperty->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(topProperty);

		auto rightLabel = o2UI.CreateLabel("R");
		rightLabel->name = "right label";
		rightLabel->horOverflow = Label::HorOverflow::None;
		*rightLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(rightLabel);

		auto rightProperty = o2UI.CreateWidget<FloatProperty>();
		rightProperty->name = "right";
		*rightProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.75f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(rightProperty);

		auto bottomLabel = o2UI.CreateLabel("B");
		bottomLabel->name = "bottom label";
		bottomLabel->horOverflow = Label::HorOverflow::None;
		*bottomLabel->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(0.75f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(bottomLabel);

		auto bottomProperty = o2UI.CreateWidget<FloatProperty>();
		bottomProperty->name = "bottom";
		*bottomProperty->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(bottomProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildRectIProperty()
	{
		auto sample = mnew RectIProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto leftLabel = o2UI.CreateLabel("L");
		leftLabel->name = "left label";
		leftLabel->horOverflow = Label::HorOverflow::None;
		*leftLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(leftLabel);

		auto leftProperty = o2UI.CreateWidget<IntegerProperty>();
		leftProperty->name = "left";
		*leftProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.25f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(leftProperty);

		auto topLabel = o2UI.CreateLabel("T");
		topLabel->name = "top label";
		topLabel->horOverflow = Label::HorOverflow::None;
		*topLabel->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.25f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(topLabel);

		auto topProperty = o2UI.CreateWidget<IntegerProperty>();
		topProperty->name = "top";
		*topProperty->layout = WidgetLayout(Vec2F(0.25f, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(topProperty);

		auto rightLabel = o2UI.CreateLabel("R");
		rightLabel->name = "right label";
		rightLabel->horOverflow = Label::HorOverflow::None;
		*rightLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(rightLabel);

		auto rightProperty = o2UI.CreateWidget<IntegerProperty>();
		rightProperty->name = "right";
		*rightProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.75f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(rightProperty);

		auto bottomLabel = o2UI.CreateLabel("B");
		bottomLabel->name = "bottom label";
		bottomLabel->horOverflow = Label::HorOverflow::None;
		*bottomLabel->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(0.75f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(bottomLabel);

		auto bottomProperty = o2UI.CreateWidget<IntegerProperty>();
		bottomProperty->name = "bottom";
		*bottomProperty->layout = WidgetLayout(Vec2F(0.75f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(bottomProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildStringProperty()
	{
		auto sample = mnew StringProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		EditBox* editBox = o2UI.CreateEditBox("singleline");
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildWStringProperty()
	{
		auto sample = mnew WStringProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		EditBox* editBox = o2UI.CreateEditBox("singleline");
		*editBox->layout = WidgetLayout::BothStretch();
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildTagProperty()
	{
		auto sample = mnew TagsProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		EditBox* editBox = o2UI.CreateEditBox("singleline");
		layout->AddChild(editBox);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildVector2FProperty()
	{
		auto sample = mnew Vec2FProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto xLabel = o2UI.CreateLabel("X");
		xLabel->name = "x label";
		xLabel->horOverflow = Label::HorOverflow::None;
		*xLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F());
		propertiesLayout->AddChild(xLabel);

		auto xProperty = o2UI.CreateWidget<FloatProperty>();
		xProperty->name = "x";
		*xProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(xProperty);

		auto yLabel = o2UI.CreateLabel("Y");
		yLabel->name = "y label";
		yLabel->horOverflow = Label::HorOverflow::None;
		*yLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(yLabel);

		auto yProperty = o2UI.CreateWidget<FloatProperty>();
		yProperty->name = "y";
		*yProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(yProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildVector2IProperty()
	{
		auto sample = mnew Vec2IProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto xLabel = o2UI.CreateLabel("X");
		xLabel->name = "x label";
		xLabel->horOverflow = Label::HorOverflow::None;
		*xLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F());
		propertiesLayout->AddChild(xLabel);

		auto xProperty = o2UI.CreateWidget<IntegerProperty>();
		xProperty->name = "x";
		*xProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(xProperty);

		auto yLabel = o2UI.CreateLabel("Y");
		yLabel->name = "y label";
		yLabel->horOverflow = Label::HorOverflow::None;
		*yLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(yLabel);

		auto yProperty = o2UI.CreateWidget<IntegerProperty>();
		yProperty->name = "y";
		*yProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(yProperty);

		o2UI.AddWidgetStyle(sample, "standard");
	}

	void EditorUIStyleBuilder::RebuildColoredVector2Property()
	{
		auto sample = mnew Vec2FProperty();
		sample->layout->minHeight = 20;
		sample->expandHeight = true;
		sample->expandWidth = true;
		sample->fitByChildren = false;

		auto layoutContainer = mnew Widget();
		layoutContainer->name = "container";
		*layoutContainer->layout = WidgetLayout::BothStretch();
		sample->AddChild(layoutContainer);

		auto layout = mnew HorizontalLayout();
		layout->name = "layout";
		*layout->layout = WidgetLayout::BothStretch();
		layoutContainer->AddChild(layout);

		auto propertiesLayout = mnew Widget();
		propertiesLayout->name = "properties";
		*propertiesLayout->layout = WidgetLayout::BothStretch();
		layout->AddChild(propertiesLayout);

		auto xLabel = o2UI.CreateLabel("X");
		xLabel->name = "x label";
		xLabel->horOverflow = Label::HorOverflow::None;
		*xLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F());
		propertiesLayout->AddChild(xLabel);

		auto xProperty = o2UI.CreateWidget<FloatProperty>("red");
		xProperty->name = "x";
		*xProperty->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0.5f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(xProperty);

		auto yLabel = o2UI.CreateLabel("Y");
		yLabel->name = "y label";
		yLabel->horOverflow = Label::HorOverflow::None;
		*yLabel->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(0.5f, 0), Vec2F(0, 0), Vec2F(20, 20));
		propertiesLayout->AddChild(yLabel);

		auto yProperty = o2UI.CreateWidget<FloatProperty>("green");
		yProperty->name = "y";
		*yProperty->layout = WidgetLayout(Vec2F(0.5f, 0), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0));
		propertiesLayout->AddChild(yProperty);

		o2UI.AddWidgetStyle(sample, "colored");
	}

	void EditorUIStyleBuilder::RebuildPropertiesWithCaptins()
	{
		BuildPropertyWithCaption<ActorProperty>("standard", "with caption");
		BuildPropertyWithCaption<BooleanProperty>("standard", "with caption");
		BuildPropertyWithCaption<BorderFProperty>("standard", "with caption");
		BuildPropertyWithCaption<BorderIProperty>("standard", "with caption");
		BuildPropertyWithCaption<ColorProperty>("standard", "with caption");
		BuildPropertyWithCaption<ComponentProperty>("standard", "with caption");
		BuildPropertyWithCaption<CurveProperty>("standard", "with caption");
		BuildPropertyWithCaption<EnumProperty>("standard", "with caption");
		BuildPropertyWithCaption<FloatProperty>("standard", "with caption");
		BuildPropertyWithCaption<IntegerProperty>("standard", "with caption");
		BuildPropertyWithCaption<LayerProperty>("standard", "with caption");
		BuildPropertyWithCaption<RectFProperty>("standard", "with caption");
		BuildPropertyWithCaption<RectIProperty>("standard", "with caption");
		BuildPropertyWithCaption<StringProperty>("standard", "with caption");
		BuildPropertyWithCaption<TagsProperty>("standard", "with caption");
		BuildPropertyWithCaption<Vec2FProperty>("standard", "with caption");
		BuildPropertyWithCaption<Vec2FProperty>("colored", "colored with caption");
		BuildPropertyWithCaption<Vec2IProperty>("standard", "with caption");
		BuildPropertyWithCaption<WStringProperty>("standard", "with caption");
		BuildPropertyWithCaption<SceneLayersListProperty>("standard", "with caption");
	}

	void EditorUIStyleBuilder::RebuildEditorUIManager(const String& stylesFolder, bool saveStyle /*= true*/, bool checkEditedDate /*= true*/)
	{
		PushEditorScopeOnStack scope;

		String thisSourcePath = "../../o2/Editor/Sources/o2Editor/Core/UIStyle/EditorUIStyle.cpp";
		TimeStamp thisSourceEditedDate = o2FileSystem.GetFileInfo(thisSourcePath).editDate;

		DataDocument stylesRebuildDateData;
		if (stylesRebuildDateData.LoadFromFile(GetEditorAssetsPath() + stylesFolder + "/rebuildDate.json"))
		{
			TimeStamp cachedDate = stylesRebuildDateData["generatedDate"];

			if (thisSourceEditedDate == cachedDate && checkEditedDate)
			{
				o2UI.LoadStyle(stylesFolder);
				return;
			}
		}

		o2UI.ClearStyle();

		auto funcs = GetType().GetFunctionsWithBaseClasses();

		Timer timer;
		for (auto func : funcs)
		{
			if (func->GetName() == "RebuildBasicUIManager" || func->GetName() == "RebuildEditorUIManager" ||
				func->GetProtectionSection() != ProtectSection::Public)
			{
				continue;
			}

			timer.Reset();
			func->Invoke<void>(this);

			auto time = timer.GetDeltaTime();
			o2Debug.Log(func->GetName() + " for " + (String)time + " sec");
		}

		if (saveStyle)
		{
			o2UI.SaveStyle(stylesFolder);

			DataDocument newStylesRebuildDateData;
			newStylesRebuildDateData["generatedDate"] = thisSourceEditedDate;
			newStylesRebuildDateData.SaveToFile(GetEditorAssetsPath() + stylesFolder + "/rebuildDate.json");
		}
	}
}

DECLARE_CLASS(Editor::EditorUIStyleBuilder);
