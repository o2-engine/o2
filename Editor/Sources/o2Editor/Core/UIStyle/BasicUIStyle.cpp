#include "o2Editor/stdafx.h"
#include "BasicUIStyle.h"

#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Animation/Tracks/AnimationColor4Track.h"
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
#include "o2/Scene/UI/Widgets/EditBoxDropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widgets/Window.h"

namespace o2
{
    void BasicUIStyleBuilder::RebuildButtonStyle()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(20, 20);

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_button_regular.png"), 
                                             Layout::BothStretch(-9, -9, -10, -10));

        auto hoverLayer   = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_button_select.png"),
                                             Layout::BothStretch(-9, -9, -10, -10));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_button_pressed.png"), 
                                             Layout::BothStretch(-9, -9, -10, -10));

        auto focusLayer   = sample->AddLayer("focus", mmake<Sprite>("ui/UI4_button_focus.png"), 
                                             Layout::BothStretch(-9, -9, -10, -10));

        Ref<Text> captionText        = mmake<Text>("stdFont.ttf");
        captionText->text        = "Button";
        captionText->horAlign    = HorAlign::Middle;
        captionText->verAlign    = VerAlign::Middle;
        captionText->dotsEngings = true;
        captionText->wordWrap    = true;
        captionText->color       = Color4(96, 125, 139);
        sample->AddLayer("caption", captionText);

        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildCloseButtonStyle()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(5, 5);

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_Close_button_regular.png"),
                                             Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Close_button_select.png"),
                                           Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Close_button_pressed.png"),
                                             Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "close");
    }

    void BasicUIStyleBuilder::RebuildArrowButtonStyle()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(5, 5);

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_Options_button_regular.png"),
                                             Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Options_button_select.png"),
                                           Layout::Based(BaseCorner::Center, Vec2F(20, 20)));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Options_button_pressed.png"),
                                             Layout::Based(BaseCorner::Center, Vec2F(20, 20)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "arrow");
    }

    void BasicUIStyleBuilder::RebuildHorSeparatorStyle()
    {
        auto sample = mmake<Widget>();
        sample->AddLayer("line", mmake<Sprite>("ui/UI4_Separator.png"),
                                  Layout::HorStretch(VerAlign::Middle, 0, 0, 5, 0));
        *sample->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 5);
        sample->layout->maxHeight = 5;
        sample->layout->minHeight = 5;
        sample->layout->height = 5;

        o2UI.AddWidgetStyle(sample, "hor separator");
    }

    void BasicUIStyleBuilder::RebuildHorProgressBarStyle()
    {
        auto sample = mmake<HorizontalProgress>();
        sample->layout->minSize = Vec2F(5, 5);
        auto backLayer = sample->AddLayer("back", nullptr);
        auto spriteBackLayer = backLayer->AddChildLayer("sprite", mmake<Sprite>("ui/UI4_Hor_scrollbar_bk.png"),
                                                        Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                               Vec2F(-2, -4), Vec2F(2, 5)));

        backLayer->interactableLayout = Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(0, -4), Vec2F(0, 4));

        auto barLayer = sample->AddLayer("bar", nullptr);
        auto barRegularSprite = barLayer->AddChildLayer("regular", mmake<Sprite>("ui/UI4_Hor_scrollhandle_regular.png"),
                                                        Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                               Vec2F(-2, -4), Vec2F(2, 5)));

        auto barSelectSprite = barLayer->AddChildLayer("hover", mmake<Sprite>("ui/UI4_Hor_scrollhandle_select.png"),
                                                       Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                              Vec2F(-2, -4), Vec2F(2, 5)));

        auto barPressedSprite = barLayer->AddChildLayer("pressed", mmake<Sprite>("ui/UI4_Hor_scrollhandle_pressed.png"),
                                                        Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                               Vec2F(-2, -4), Vec2F(2, 5)));

        sample->AddState("hover", AnimationClip::EaseInOut("layer/bar/child/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/bar/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->SetOrientation(HorizontalProgress::Orientation::Right);

        o2UI.AddWidgetStyle(sample, "standard");
    }


    void BasicUIStyleBuilder::RebuildVerProgressBarStyle()
    {
        auto sample = mmake<VerticalProgress>();
        sample->layout->minSize = Vec2F(5, 5);
        auto backLayer = sample->AddLayer("back", nullptr);
        auto spriteBackLayer = backLayer->AddChildLayer("sprite", mmake<Sprite>("ui/UI4_Ver_scrollbar_bk.png"),
                                                        Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                               Vec2F(-4, -2), Vec2F(5, 2)));
        backLayer->interactableLayout = Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f), Vec2F(-4, 0), Vec2F(4, 0));

        auto barLayer = sample->AddLayer("bar", nullptr);
        auto barRegularSprite = barLayer->AddChildLayer("regular", mmake<Sprite>("ui/UI4_Ver_scrollhandle_regular.png"),
                                                        Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                               Vec2F(-4, -2), Vec2F(5, 2)));

        auto barSelectSprite = barLayer->AddChildLayer("hover", mmake<Sprite>("ui/UI4_Ver_scrollhandle_select.png"),
                                                       Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                              Vec2F(-4, -2), Vec2F(5, 2)));

        auto barPressedSprite = barLayer->AddChildLayer("pressed", mmake<Sprite>("ui/UI4_Ver_scrollhandle_pressed.png"),
                                                        Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                               Vec2F(-4, -2), Vec2F(5, 2)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/bar/child/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/bar/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildHorScrollBarStyle()
    {
        auto sample = mmake<HorizontalScrollBar>();
        sample->layout->minSize = Vec2F(5, 5);
        sample->SetScrollSense(0.25f);
        sample->SetMinimalScrollHandleSize(10);

        auto backLayer = sample->AddLayer("back", nullptr);
        backLayer->interactableLayout = Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(0, -4), Vec2F(0, 4));
        auto spriteBackLayer = backLayer->AddChildLayer("sprite", mmake<Sprite>("ui/UI4_Hor_scrollbar_bk.png"),
                                                        Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                               Vec2F(-2, -4), Vec2F(2, 5)));

        auto handleLayer = sample->AddLayer("handle", nullptr);
        handleLayer->interactableLayout = Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(0, -4), Vec2F(0, 4));
        auto barRegularSprite = handleLayer->AddChildLayer("regular", mmake<Sprite>("ui/UI4_Hor_scrollhandle_regular.png"),
                                                           Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                                  Vec2F(-2, -4), Vec2F(2, 5)));

        auto barSelectSprite = handleLayer->AddChildLayer("hover", mmake<Sprite>("ui/UI4_Hor_scrollhandle_select.png"),
                                                          Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                                 Vec2F(-2, -4), Vec2F(2, 5)));

        auto barPressedSprite = handleLayer->AddChildLayer("pressed", mmake<Sprite>("ui/UI4_Hor_scrollhandle_pressed.png"),
                                                           Layout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 0.5f),
                                                                  Vec2F(-2, -4), Vec2F(2, 5)));
                                            
        sample->AddState("hover", AnimationClip::EaseInOut("layer/handle/child/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/handle/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildVerScrollBarStyle()
    {
        auto sample = mmake<VerticalScrollBar>();
        sample->layout->minSize = Vec2F(5, 5);
        sample->SetScrollSense(0.25f);
        sample->SetMinimalScrollHandleSize(10);


        auto backLayer = sample->AddLayer("back", nullptr);
        backLayer->interactableLayout = Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f), Vec2F(-4, 0), Vec2F(4, 0));
        auto spriteBackLayer = backLayer->AddChildLayer("sprite", mmake<Sprite>("ui/UI4_Ver_scrollbar_bk.png"),
                                                        Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                               Vec2F(-4, -2), Vec2F(5, 2)));

        auto handleLayer = sample->AddLayer("handle", nullptr);
        handleLayer->interactableLayout = Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f), Vec2F(-4, 0), Vec2F(4, 0));
        auto barRegularSprite = handleLayer->AddChildLayer("regular", mmake<Sprite>("ui/UI4_Ver_scrollhandle_regular.png"),
                                                           Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                                  Vec2F(-4, -2), Vec2F(5, 2)));

        auto barSelectSprite = handleLayer->AddChildLayer("hover", mmake<Sprite>("ui/UI4_Ver_scrollhandle_select.png"),
                                                          Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                                 Vec2F(-4, -2), Vec2F(5, 2)));

        auto barPressedSprite = handleLayer->AddChildLayer("pressed", mmake<Sprite>("ui/UI4_Ver_scrollhandle_pressed.png"),
                                                           Layout(Vec2F(0.5f, 0.0f), Vec2F(0.5f, 1.0f),
                                                                  Vec2F(-4, -2), Vec2F(5, 2)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/handle/child/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/handle/child/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildCheckboxStyle()
    {
        auto sample = mmake<Toggle>();
        sample->layout->minSize = Vec2F(20, 20);
        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Check_bk.png"),
                                          Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(0, -10), Vec2F(20, 10)));

        auto hoverLayer = sample->AddLayer("backSelect", mmake<Sprite>("ui/UI4_Check_bk_select.png"),
                                            Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(0, -10), Vec2F(20, 10)));

        auto pressedLayer = sample->AddLayer("backPressed", mmake<Sprite>("ui/UI4_Check_bk_pressed.png"),
                                             Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(0, -10), Vec2F(20, 10)));

        auto focusLayer = sample->AddLayer("backFocus", mmake<Sprite>("ui/UI4_Check_bk_focus.png"),
                                           Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(0, -10), Vec2F(20, 10)));

        auto checkLayer = sample->AddLayer("check", mmake<Sprite>("ui/UI4_Ckeck.png"),
                                           Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(1, -11), Vec2F(21, 10)));

        auto unknownLayer = sample->AddLayer("unknown", mmake<Sprite>("ui/UI4_Check_unknown.png"),
                                           Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(1, -11), Vec2F(21, 10)));

        Ref<Text> captionText = mmake<Text>("stdFont.ttf");
        captionText->text = "Checkbox";
        captionText->horAlign = HorAlign::Left;
        captionText->verAlign = VerAlign::Middle;
        captionText->dotsEngings = true;
        captionText->color = Color4(96, 125, 139);
        sample->AddLayer("caption", captionText, Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(20, 0), Vec2F(0, 0)));

        sample->AddState("hover", AnimationClip::EaseInOut("layer/backSelect/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("value", AnimationClip::EaseInOut("layer/check/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("unknown", AnimationClip::EaseInOut("layer/unknown/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/backPressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/backFocus/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildCheckboxWithoutCaptionStyle()
    {
        auto sample = mmake<Toggle>();
        sample->layout->minSize = Vec2F(20, 20);
        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Check_bk.png"),
                                          Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        auto hoverLayer = sample->AddLayer("backSelect", mmake<Sprite>("ui/UI4_Check_bk_select.png"),
                                            Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        auto pressedLayer = sample->AddLayer("backPressed", mmake<Sprite>("ui/UI4_Check_bk_pressed.png"),
                                             Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        auto focusLayer = sample->AddLayer("backFocus", mmake<Sprite>("ui/UI4_Check_bk_focus.png"),
                                           Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        auto checkLayer = sample->AddLayer("check", mmake<Sprite>("ui/UI4_Ckeck.png"),
                                           Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        auto unknownLayer = sample->AddLayer("unknown", mmake<Sprite>("ui/UI4_Check_unknown.png"),
                                             Layout::Based(BaseCorner::Right, Vec2F(20, 20)));

        sample->AddState("hover", AnimationClip::EaseInOut("layer/backSelect/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("value", AnimationClip::EaseInOut("layer/check/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("unknown", AnimationClip::EaseInOut("layer/unknown/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/backPressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/backFocus/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "without caption");
    }

    void BasicUIStyleBuilder::RebuildScrollAreaStraightBarsStyle()
    {
        auto sample = mmake<ScrollArea>();
        sample->layout->minSize = Vec2F(30, 30);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(false);
        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));

        auto horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        auto enableHorScrollAnim = mmake<AnimationClip>();
        *enableHorScrollAnim->AddTrack<float>("mVerScrollBar/layout/offsetBottom") =
            AnimationTrack<float>::EaseInOut(5, 15, 0.2f);

        *enableHorScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMin") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(5, 5), Vec2F(5, 15), 0.2f);

        auto enableHorScrollState = sample->AddState("enableHorBar", enableHorScrollAnim);

        auto enableVerScrollAnim = mmake<AnimationClip>();
        *enableVerScrollAnim->AddTrack<float>("mHorScrollBar/layout/offsetRight") =
            AnimationTrack<float>::EaseInOut(-5, -15, 0.2f);

        *enableVerScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMax") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(-5, -5), Vec2F(-15, -5), 0.2f);

        auto enableVerScrollState = sample->AddState("enableVerBar", enableVerScrollAnim);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "straight bars");
    }

    void BasicUIStyleBuilder::RebuildPopupStyle()
    {
        Ref<PopupWidget> sample = mmake<PopupWidget>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        sample->SetEnableScrollsHiding(true);
        sample->SetMinFitSize(50);

        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildScrollAreaStyle()
    {
        auto sample = mmake<ScrollArea>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);
        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildLabelStyle()
    {
        auto sample = mmake<Label>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->horOverflow = Label::HorOverflow::Dots;
        sample->verOverflow = Label::VerOverflow::None;

        sample->SetFont(AssetRef<VectorFontAsset>("stdFont.ttf")->GetFont());
        sample->SetHorAlign(HorAlign::Middle);
        sample->SetVerAlign(VerAlign::Middle);
        sample->SetColor(Color4(96, 125, 139));

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildEditBoxStyle()
    {
        Ref<EditBox> sample = mmake<EditBox>();
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(false);
        sample->SetCaretBlinkingDelay(1.15f);
        sample->SetSelectionColor(Color4(0, 156, 141, 120));
        sample->layout->minSize = Vec2F(30, 40);

        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto focusLayer = sample->AddLayer("focused", mmake<Sprite>("ui/UI4_Editbox_focus.png"), Layout::BothStretch(-9, -9, -9, -9));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        auto enableHorScrollAnim = mmake<AnimationClip>();
        *enableHorScrollAnim->AddTrack<float>("mVerScrollBar/layout/offsetBottom") =
            AnimationTrack<float>::EaseInOut(5, 15, 0.2f);

        *enableHorScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMin") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(5, 5), Vec2F(5, 15), 0.2f);

        auto enableHorScrollState = sample->AddState("enableHorBar", enableHorScrollAnim);

        auto enableVerScrollAnim = mmake<AnimationClip>();
        *enableVerScrollAnim->AddTrack<float>("mHorScrollBar/layout/offsetRight") =
            AnimationTrack<float>::EaseInOut(-5, -15, 0.2f);

        *enableVerScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMax") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(-5, -5), Vec2F(-15, -5), 0.2f);

        auto enableVerScrollState = sample->AddState("enableVerBar", enableVerScrollAnim);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        Ref<Text> textDrawable = sample->GetTextDrawable();
        textDrawable->verAlign = VerAlign::Top;
        textDrawable->horAlign = HorAlign::Left;
        textDrawable->color = Color4(96, 125, 139);
        textDrawable->SetFontAsset(AssetRef<FontAsset>("stdFont.ttf"));

        Ref<Sprite> caretDrawable = sample->GetCaretDrawable();
        *caretDrawable = Sprite();
        caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetFontHeight())*1.7f);
        caretDrawable->pivot = Vec2F(0, 0.16f);
        caretDrawable->color = Color4::Black();

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildCustomListStyle()
    {
        Ref<CustomList> sample = mmake<CustomList>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);
        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));

        Ref<Sprite> selection = mmake<Sprite>("ui/UI4_Context_menu_select.png");
        *sample->GetSelectionDrawable() = *selection;
        sample->SetSelectionDrawableLayout(Layout::BothStretch(-12, -16, -10, -16));

        Ref<Sprite> hover = mmake<Sprite>("ui/UI4_Context_menu_white.png");
        *sample->GetHoverDrawable() = *hover;
        sample->SetHoverDrawableLayout(Layout::BothStretch(-12, -16, -10, -16));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
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

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildLongListStyle()
    {
        auto sample = mmake<LongList>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);

        Ref<Sprite> selection = mmake<Sprite>("ui/UI4_Context_menu_select.png");
        *sample->GetSelectionDrawable() = *selection;
        sample->SetSelectionDrawableLayout(Layout::BothStretch(-12, -16, -10, -16));

        Ref<Sprite> hover = mmake<Sprite>("ui/UI4_Context_menu_white.png");
        *sample->GetHoverDrawable() = *hover;
        sample->SetHoverDrawableLayout(Layout::BothStretch(-12, -16, -10, -16));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
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

        sample->AddState("focused", AnimationClip::EaseInOut("mSelectionDrawable/transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildListStyle()
    {
        Ref<List> sample = mmake<List>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);
        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));

        Ref<Sprite> selection = mmake<Sprite>("ui/UI4_Context_menu_select.png");
        *sample->GetSelectionDrawable() = *selection;
        sample->SetSelectionDrawableLayout(Layout::BothStretch(-10, -16, -10, -16));

        Ref<Sprite> hover = mmake<Sprite>("ui/UI4_Context_menu_white.png");
        *sample->GetHoverDrawable() = *hover;
        sample->SetHoverDrawableLayout(Layout::BothStretch(-10, -16, -10, -16));

        Ref<Label> itemSample = o2UI.CreateLabel("empty");
        itemSample->layout->size = Vec2F(20, 0);
        itemSample->horAlign = HorAlign::Left;
        itemSample->verAlign = VerAlign::Middle;
        sample->SetItemSample(itemSample);

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
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

        sample->AddState("focused", AnimationClip::EaseInOut("mSelectionDrawable/transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildCustomDropDownStyle()
    {
        auto sample = mmake<CustomDropDown>();
        sample->layout->minSize = Vec2F(20, 20);
        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Editbox_pressed.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto arrowLayer = sample->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn.png"),
                                           Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));

        sample->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

        Ref<Widget> itemSample = mmake<Widget>();
        itemSample->layout->size = Vec2F(20, 20);
        sample->SetItemSample(itemSample);

        auto list = sample->GetListView();
        *list = *o2UI.GetWidgetStyle<CustomList>("standard");
        list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        list->layer["back"]->SetDrawable(mmake<Sprite>("ui/UI4_Box_regular.png"));
        list->layout->pivot = Vec2F(0.5f, 1.0f);
        list->layout->anchorMin = Vec2F(0, 0);
        list->layout->anchorMax = Vec2F(1, 0);
        list->layout->offsetMin = Vec2F(-1, -60);
        list->layout->offsetMax = Vec2F(0, 3);

        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildDropDownStyle()
    {
        Ref<DropDown> sample = mmake<DropDown>();
        sample->layout->minSize = Vec2F(20, 20);
        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), 
                                          Layout::BothStretch(-9, -9, -9, -9));

        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), 
                                           Layout::BothStretch(-9, -9, -9, -9));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Editbox_pressed.png"), 
                                             Layout::BothStretch(-9, -9, -9, -9));

        auto arrowLayer = sample->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn.png"),
                                           Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));

        sample->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

        auto list = sample->GetListView();
        *list = *o2UI.GetWidgetStyle<CustomList>("standard");
        list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        list->layer["back"]->SetDrawable(mmake<Sprite>("ui/UI4_Box_regular.png"));
        list->layout->pivot = Vec2F(0.5f, 1.0f);
        list->layout->anchorMin = Vec2F(0, 0);
        list->layout->anchorMax = Vec2F(1, 0);
        list->layout->offsetMin = Vec2F(2, -60);
        list->layout->offsetMax = Vec2F(0, 3);

        Ref<Label> itemSample = o2UI.CreateLabel("empty");
        itemSample->horAlign = HorAlign::Left;
        sample->SetItemSample(itemSample);

        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildEditBoxDropDownStyle()
    {
        auto sample = mmake<EditBoxDropDown>();
        sample->layout->minSize = Vec2F(20, 20);
        
        // Background layers
        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), 
                                          Layout::BothStretch(-9, -9, -9, -9));

        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), 
                                           Layout::BothStretch(-9, -9, -9, -9));

        auto focusLayer = sample->AddLayer("focused", mmake<Sprite>("ui/UI4_Editbox_focus.png"), 
                                           Layout::BothStretch(-9, -9, -9, -9));

        auto arrowLayer = sample->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn.png"),
                                           Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));

        sample->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

        // Configure the EditBox
        auto editBox = sample->GetEditBox();
        *editBox = *o2UI.GetWidgetStyle<EditBox>("standard");
        editBox->RemoveLayer("back");
        editBox->RemoveLayer("hover");
        editBox->RemoveLayer("focused");
        editBox->layout->minSize = Vec2F(20, 20);
        editBox->layout->offsetMin = Vec2F(5, 2);
        editBox->layout->offsetMax = Vec2F(-25, -2);
        editBox->SetMultiLine(false);
        
        // Configure the dropdown list
        auto list = sample->GetListView();
        *list = *o2UI.GetWidgetStyle<CustomList>("standard");
        list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        list->layer["back"]->SetDrawable(mmake<Sprite>("ui/UI4_Box_regular.png"));
        list->layout->pivot = Vec2F(0.5f, 1.0f);
        list->layout->anchorMin = Vec2F(0, 0);
        list->layout->anchorMax = Vec2F(1, 0);
        list->layout->offsetMin = Vec2F(2, -160);
        list->layout->offsetMax = Vec2F(0, 3);

        // Create item sample
        Ref<Label> itemSample = o2UI.CreateLabel("empty");
        itemSample->horAlign = HorAlign::Left;
        sample->SetItemSample(itemSample);

        // Add states and animations
        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildWindowStyle()
    {
        auto sample = mmake<Window>();
        sample->layout->minSize = Vec2F(100, 50);
        sample->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 20));
        sample->SetEnableScrollsHiding(true);

        auto regularBackLayer = sample->AddLayer("regularBack", mmake<Sprite>("ui/UI4_window_frame_regular.png"),
                                                 Layout::BothStretch(-13, -13, -13, -11));

        auto iconLayer = sample->AddLayer("icon", mmake<Sprite>("ui/UI4_o2_sign.png"),
                                          Layout::Based(BaseCorner::LeftTop, Vec2F(18, 18), Vec2F(-1, 1)));

        Ref<Text> captionText = mmake<Text>("stdFont.ttf");
        captionText->text = "Window";
        captionText->horAlign = HorAlign::Left;
        captionText->verAlign = VerAlign::Middle;
        captionText->dotsEngings = true;
        auto textLayer = sample->AddLayer("caption", captionText,
                                          Layout::HorStretch(VerAlign::Top, 20, 35, 20, -2));

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        *horScrollBar->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 15, -5);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        *verScrollBar->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 15, 15, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 2.0f;

        Ref<Button> closeBtn = o2UI.CreateWidget<Button>("close");
        closeBtn->name = "closeButton";
        *closeBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(1, 2));
        sample->AddInternalWidget(closeBtn);

        Ref<Button> optionsBtn = o2UI.CreateWidget<Button>("arrow");
        optionsBtn->name = "optionsButton";
        *optionsBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-15, 2));
        sample->AddInternalWidget(optionsBtn);

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

    void BasicUIStyleBuilder::RebuildContextMenuStyle()
    {
        Ref<ContextMenu> sample = mmake<ContextMenu>();

        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        sample->SetEnableScrollsHiding(true);
        sample->SetMinFitSize(50);

        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));

        Ref<Sprite> selection = mmake<Sprite>("ui/UI4_Context_menu_select.png");
        *sample->GetSelectionDrawable() = *selection;
        sample->SetSelectionDrawableLayout(Layout::BothStretch(-10, -16, -10, -16));

        // Separator sample
        Ref<Widget> separatorSample = sample->GetSeparatorSample();
        separatorSample->AddLayer("line", mmake<Sprite>("ui/UI4_Separator.png"),
                                  Layout::HorStretch(VerAlign::Middle, 0, 0, 5, 0));

        // Item sample
        Ref<ContextMenuItem> itemSample = sample->GetItemSample();

        Ref<WidgetLayer> captionLayer = itemSample->FindLayer("caption");
        Ref<Text> captionLayerText = mmake<Text>("stdFont.ttf");
        captionLayer->SetDrawable(captionLayerText);
        captionLayer->layout = Layout::BothStretch(20, 1, 0, -1);
        captionLayerText->horAlign = HorAlign::Left;
        captionLayerText->verAlign = VerAlign::Middle;
        captionLayerText->color = Color4(96, 125, 139);

        Ref<WidgetLayer> shortcutLayer = itemSample->FindLayer("shortcut");
        Ref<Text> shortcutLayerText = mmake<Text>("stdFont.ttf");
        shortcutLayer->layout = Layout::BothStretch(20, 1, 10, -1);
        shortcutLayer->SetDrawable(shortcutLayerText);
        shortcutLayerText->horAlign = HorAlign::Right;
        shortcutLayerText->verAlign = VerAlign::Middle;
        shortcutLayerText->color = Color4(96, 125, 139);
        shortcutLayer->transparency = 0.7f;

        Ref<WidgetLayer> subIconLayer = itemSample->FindLayer("subIcon");
        subIconLayer->SetDrawable(mmake<Sprite>("ui/UI4_Right_icn.png"));
        subIconLayer->layout = Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-15, 10), Vec2F(5, -10));

        itemSample->AddLayer("check", mmake<Sprite>("ui/UI4_Ckeck.png"), 
                             Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(0, 0)));

        Ref<WidgetLayer> basicLayer = itemSample->FindLayer("basic");

        itemSample->AddState("enabled", AnimationClip::EaseInOut("layer/basic/transparency", 0.5f, 1.0f, 0.2f));
        itemSample->SetStateForcible("enabled", true);

        // Search panel
        Ref<Widget> searchPanel = mmake<Widget>();
        searchPanel->name = "search panel";
        searchPanel->layout->minHeight = 25;
        searchPanel->layout->minWidth = 200;
        searchPanel->enabled = false;

        searchPanel->AddLayer("search icon", mmake<Sprite>("ui/UI4_search_regular.png"),
                              Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F()));

        searchPanel->AddLayer("line", mmake<Sprite>("ui/UI4_Separator.png"),
                              Layout::HorStretch(VerAlign::Bottom, 0, 0, 5, 0));

        Ref<EditBox> searchEditBox = o2UI.CreateEditBox("singleline");
        *searchEditBox->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 0, 20);
        searchPanel->AddChild(searchEditBox);

        auto itemsLayout = sample->FindChildByType<VerticalLayout>();
        itemsLayout->AddChild(searchPanel);

        // Scrollbars
        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        // States
        sample->AddState("enableHorBar", AnimationClip::EaseInOut("mVerScrollBar/layout/offsetBottom",
                                                              5.0f, 15.0f, 0.2f));

        sample->AddState("enableVerBar", AnimationClip::EaseInOut("mHorScrollBar/layout/offsetRight",
                                                              -5.0f, -15.0f, 0.2f));

        sample->AddState("hover", AnimationClip::EaseInOut("mSelectionDrawable/transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f));

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildMenuPanelStyle()
    {
        auto sample = mmake<MenuPanel>();

        sample->AddLayer("back", mmake<Sprite>("ui/UI4_Up_Menu.png"), Layout::BothStretch(-15, -15, -15, -17));

        Ref<Sprite> selection = mmake<Sprite>("ui/UI_ListBox_selection_hover.png");
        *sample->GetSelectionDrawable() = *selection;
        sample->SetSelectionDrawableLayout(Layout::BothStretch(-10, -16, -10, -16));

        auto itemSample = o2UI.CreateLabel("unknown");
        itemSample->horOverflow = Label::HorOverflow::Expand;
        itemSample->expandBorder = Vec2F(10, 0);
        itemSample->color = Color4(235, 255, 253);
        sample->SetItemSample(itemSample);

        sample->AddState("hover", AnimationClip::EaseInOut("mSelectionDrawable/transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f));

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildTreeStyle()
    {
        Ref<Tree> sample = mmake<Tree>();
        sample->layout->minSize = Vec2F(20, 20);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);
        sample->SetChildsNodesOffset(10.0f);

        *sample->GetHoverDrawable() = Sprite("ui/UI4_Context_menu_white.png");

        auto itemSample = sample->GetNodeSample();

        auto itemSelectionLayer = itemSample->AddLayer("select", nullptr);

        auto itemFocusedLayer = itemSelectionLayer->AddChildLayer("focused", mmake<Sprite>("ui/UI4_Context_menu_select.png"),
                                                                  Layout::BothStretch(-10, -16, -10, -16));

        auto itemUnfocusedLayer = itemSelectionLayer->AddChildLayer("unfocused", mmake<Sprite>("ui/UI4_Context_menu_white.png"),
                                                                    Layout::BothStretch(-10, -16, -10, -16));

        Ref<Text> captionLayerText = mmake<Text>("stdFont.ttf");
        captionLayerText->horAlign = HorAlign::Left;
        captionLayerText->verAlign = VerAlign::Middle;
        captionLayerText->color = Color4(96, 125, 139);
        itemSample->AddLayer("name", captionLayerText, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(12, -20), Vec2F(0, 0)));

        Ref<Button> itemSampleExpandBtn = mmake<Button>();
        itemSampleExpandBtn->layout->minSize = Vec2F(5, 5);
        itemSampleExpandBtn->name = "expandBtn";

        auto regularLayer = itemSampleExpandBtn->AddLayer("regular", mmake<Sprite>("ui/UI4_Right_icn.png"),
                                                          Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto hoverLayer = itemSampleExpandBtn->AddLayer("hover", mmake<Sprite>("ui/UI4_Right_icn_select.png"),
                                                         Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto pressedLayer = itemSampleExpandBtn->AddLayer("pressed", mmake<Sprite>("ui/UI4_Right_icn_pressed.png"),
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

        auto expandedStateAnim = mmake<AnimationClip>();
        *expandedStateAnim->AddTrack<float>("child/expandBtn/layer/regular/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);
        *expandedStateAnim->AddTrack<float>("child/expandBtn/layer/hover/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);
        *expandedStateAnim->AddTrack<float>("child/expandBtn/layer/pressed/mDrawable/angle") = 
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        itemSample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

        itemSample->AddState("selected", AnimationClip::EaseInOut("layer/select/transparency", 0.0f, 1.0f, 0.2f));

        auto focusedItemAnim = AnimationClip::EaseInOut("layer/select/child/focused/transparency", 0.0f, 1.0f, 0.2f);
        *focusedItemAnim->AddTrack<float>("layer/select/child/unfocused/transparency") = AnimationTrack<float>::EaseInOut(1.0f, 0.0f, 0.2f);
        itemSample->AddState("focused", focusedItemAnim);

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
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

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5;

        o2UI.AddWidgetStyle(sample, "standard");
    }

    void BasicUIStyleBuilder::RebuildBacklessDropdown()
    {
        Ref<DropDown> sample = mmake<DropDown>();
        sample->layout->minSize = Vec2F(20, 20);
        auto arrowLayer = sample->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn_white.png"),
                                           Layout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, -1)));

        sample->SetClippingLayout(Layout::BothStretch(4, 2, 20, 2));

        auto list = sample->GetListView();
        *list = *o2UI.GetWidgetStyle<CustomList>("standard");

        list->SetViewLayout(Layout::BothStretch(2, 2, 2, 2));
        list->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));

        list->RemoveLayer("back");
        list->AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-22, -19, -19, -19));

        list->layout->pivot = Vec2F(0.5f, 1.0f);
        list->layout->anchorMin = Vec2F(0, 0);
        list->layout->anchorMax = Vec2F(1, 0);
        list->layout->offsetMin = Vec2F(0, -60);
        list->layout->offsetMax = Vec2F(0, 0);

        Ref<Label> itemSample = o2UI.CreateLabel("empty");
        itemSample->horAlign = HorAlign::Left;
        sample->SetItemSample(itemSample);

        // Selected text
        Ref<Text> captionText = mmake<Text>("stdFont.ttf");
        captionText->horAlign = HorAlign::Left;
        captionText->verAlign = VerAlign::Middle;
        captionText->dotsEngings = true;
        captionText->wordWrap = false;
        captionText->color = Color4(235, 255, 253);
        sample->AddLayer("selectedText", captionText, Layout::BothStretch(4, 0, 0, 0));

        // States
        sample->AddState("opened", AnimationClip::EaseInOut("layer/arrow/mDrawable/scale", Vec2F(1, 1), Vec2F(1, -1), 0.2f));

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "backless");
    }

    void BasicUIStyleBuilder::RebuildBacklessEditbox()
    {
        Ref<EditBox> sample = mmake<EditBox>();
        sample->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        sample->SetViewLayout(Layout::BothStretch(0, 0, 2, 0));
        sample->SetCaretBlinkingDelay(1.15f);
        sample->SetSelectionColor(Color4(0, 156, 141, 120));
        sample->SetMultiLine(false);
        sample->layout->minSize = Vec2F(30, 40);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        Ref<Text> textDrawable = sample->GetTextDrawable();
        textDrawable->verAlign = VerAlign::Middle;
        textDrawable->horAlign = HorAlign::Left;
        textDrawable->color = Color4(96, 125, 139);
        textDrawable->SetFontAsset(AssetRef<FontAsset>("stdFont.ttf"));

        Ref<Sprite> caretDrawable = sample->GetCaretDrawable();
        *caretDrawable = Sprite();
        caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetFontHeight())*1.7f);
        caretDrawable->pivot = Vec2F(0, 0.26f);
        caretDrawable->color = Color4::Black();

        o2UI.AddWidgetStyle(sample, "backless");
    }

    void BasicUIStyleBuilder::RebuildBacklessMultilineEditbox()
    {
        Ref<EditBox> sample = mmake<EditBox>();
        sample->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        sample->SetViewLayout(Layout::BothStretch(0, 0, 2, 0));
        sample->SetCaretBlinkingDelay(1.15f);
        sample->SetSelectionColor(Color4(0, 156, 141, 120));
        sample->layout->minSize = Vec2F(30, 40);

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        auto enableHorScrollAnim = mmake<AnimationClip>();
        *enableHorScrollAnim->AddTrack<float>("mVerScrollBar/layout/offsetBottom") =
            AnimationTrack<float>::EaseInOut(5, 15, 0.2f);

        *enableHorScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMin") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(5, 5), Vec2F(5, 15), 0.2f);

        auto enableHorScrollState = sample->AddState("enableHorBar", enableHorScrollAnim);

        auto enableVerScrollAnim = mmake<AnimationClip>();
        *enableVerScrollAnim->AddTrack<float>("mHorScrollBar/layout/offsetRight") =
            AnimationTrack<float>::EaseInOut(-5, -15, 0.2f);

        *enableVerScrollAnim->AddTrack<Vec2F>("mViewAreaLayout/offsetMax") =
            AnimationTrack<Vec2F>::EaseInOut(Vec2F(-5, -5), Vec2F(-15, -5), 0.2f);

        auto enableVerScrollState = sample->AddState("enableVerBar", enableVerScrollAnim);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("focused", AnimationClip::EaseInOut("layer/focused/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        Ref<Text> textDrawable = sample->GetTextDrawable();
        textDrawable->verAlign = VerAlign::Top;
        textDrawable->horAlign = HorAlign::Left;
        textDrawable->color = Color4(96, 125, 139);
        textDrawable->SetFontAsset(AssetRef<FontAsset>("stdFont.ttf"));

        Ref<Sprite> caretDrawable = sample->GetCaretDrawable();
        *caretDrawable = Sprite();
        caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetFontHeight())*1.7f);
        caretDrawable->pivot = Vec2F(0, 0.16f);
        caretDrawable->color = Color4::Black();

        o2UI.AddWidgetStyle(sample, "backless multiline");
    }

    void BasicUIStyleBuilder::RebuildSinglelineEditbox()
    {
        Ref<EditBox> sample = mmake<EditBox>();
        sample->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        sample->SetViewLayout(Layout::BothStretch(3, 1, 3, -1));
        sample->SetCaretBlinkingDelay(1.15f);
        sample->SetMultiLine(false);
        sample->SetSelectionColor(Color4(0, 156, 141, 120));
        sample->layout->minSize = Vec2F(25, 25);

        auto backLayer = sample->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto hoverLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9));
        auto focusLayer = sample->AddLayer("focus", mmake<Sprite>("ui/UI4_Editbox_focus.png"), Layout::BothStretch(-9, -9, -9, -9));

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        auto focusAnim = AnimationClip::EaseInOut("layer/focus/transparency", 0.0f, 1.0f, 0.05f);
        *focusAnim->AddTrack<float>("layer/hover/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.05f);
        sample->AddState("focused", focusAnim)
            ->offStateAnimationSpeed = 0.5f;

        Ref<Text> textDrawable = sample->GetTextDrawable();
        textDrawable->verAlign = VerAlign::Middle;
        textDrawable->horAlign = HorAlign::Left;
        textDrawable->color = Color4(96, 125, 139);
        textDrawable->SetFontAsset(AssetRef<FontAsset>("stdFont.ttf"));

        Ref<Sprite> caretDrawable = sample->GetCaretDrawable();
        *caretDrawable = Sprite();
        caretDrawable->size = Vec2F(1, textDrawable->GetFont()->GetHeightPx(textDrawable->GetFontHeight())*1.7f);
        caretDrawable->pivot = Vec2F(0, 0.26f);
        caretDrawable->color = Color4::Black();

        o2UI.AddWidgetStyle(sample, "singleline");
    }

    void BasicUIStyleBuilder::RebuildBacklessScrollarea()
    {
        Ref<ScrollArea> sample = mmake<ScrollArea>();
        sample->layout->minSize = Vec2F(10, 10);
        sample->SetClippingLayout(Layout::BothStretch(1, 2, 1, 1));
        sample->SetViewLayout(Layout::BothStretch(5, 5, 5, 5));
        sample->SetEnableScrollsHiding(true);

        Ref<HorizontalScrollBar> horScrollBar = o2UI.CreateHorScrollBar();
        horScrollBar->layout->anchorMin = Vec2F(0, 0);
        horScrollBar->layout->anchorMax = Vec2F(1, 0);
        horScrollBar->layout->offsetMin = Vec2F(5, 0);
        horScrollBar->layout->offsetMax = Vec2F(-15, 15);
        sample->SetHorizontalScrollBar(horScrollBar);

        Ref<VerticalScrollBar> verScrollBar = o2UI.CreateVerScrollBar();
        verScrollBar->layout->anchorMin = Vec2F(1, 0);
        verScrollBar->layout->anchorMax = Vec2F(1, 1);
        verScrollBar->layout->offsetMin = Vec2F(-15, 15);
        verScrollBar->layout->offsetMax = Vec2F(0, -5);
        sample->SetVerticalScrollBar(verScrollBar);

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "backless");
    }

    void BasicUIStyleBuilder::RebuildExpandButton()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(5, 5);
        sample->name = "expandBtn";

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_Right_icn.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto selectLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Right_icn_select.png"),
                                            Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Right_icn_pressed.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        auto expandedStateAnim = mmake<AnimationClip>();
        *expandedStateAnim->AddTrack<float>("layer/regular/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        *expandedStateAnim->AddTrack<float>("layer/hover/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        *expandedStateAnim->AddTrack<float>("layer/pressed/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        sample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

        o2UI.AddWidgetStyle(sample, "expand");
    }

    void BasicUIStyleBuilder::RebuildExpandDownButton()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(5, 5);
        sample->name = "expandBtn";

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_Down_icn.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto selectLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Down_icn_select.png"),
                                            Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Down_icn_pressed.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        o2UI.AddWidgetStyle(sample, "expand down");
    }

    void BasicUIStyleBuilder::RebuildExpandWhiteButton()
    {
        auto sample = mmake<Button>();
        sample->layout->minSize = Vec2F(5, 5);
        sample->name = "expandBtn";

        auto regularLayer = sample->AddLayer("regular", mmake<Sprite>("ui/UI4_Right_icn_white.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto selectLayer = sample->AddLayer("hover", mmake<Sprite>("ui/UI4_Right_icn_white_select.png"),
                                            Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));

        auto pressedLayer = sample->AddLayer("pressed", mmake<Sprite>("ui/UI4_Right_icn_white_pressed.png"),
                                             Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-10, -10), Vec2F(10, 10)));


        sample->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.1f))
            ->offStateAnimationSpeed = 1.0f / 4.0f;

        sample->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))
            ->offStateAnimationSpeed = 0.5f;

        sample->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.2f))
            ->offStateAnimationSpeed = 0.5f;

        auto expandedStateAnim = mmake<AnimationClip>();
        *expandedStateAnim->AddTrack<float>("layer/regular/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        *expandedStateAnim->AddTrack<float>("layer/hover/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        *expandedStateAnim->AddTrack<float>("layer/pressed/mDrawable/angle") =
            AnimationTrack<float>::EaseInOut(Math::Deg2rad(0.0f), Math::Deg2rad(-90.0f), 0.1f);

        sample->AddState("expanded", expandedStateAnim)->offStateAnimationSpeed = 2.5f;

        o2UI.AddWidgetStyle(sample, "expand white");
    }

    void BasicUIStyleBuilder::RebuildSpoiler()
    {
        auto sample = mmake<Spoiler>();
        sample->spacing = 5.0f;
        sample->borderLeft = 10;
        sample->expandHeight = false;
        sample->expandWidth = true;
        sample->fitByChildren = true;
        sample->baseCorner = BaseCorner::RightTop;

        Ref<Text> captionText        = mmake<Text>("stdFont.ttf");
        captionText->text        = "Button";
        captionText->horAlign    = HorAlign::Left;
        captionText->verAlign    = VerAlign::Middle;
        captionText->dotsEngings = true;
        captionText->wordWrap = true;
        captionText->color = Color4(96, 125, 139);
        sample->AddLayer("caption", captionText, Layout::HorStretch(VerAlign::Top, 11, 0, 20));

        auto expandBtn = o2UI.CreateWidget<Button>("expand");
        *expandBtn->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-7, 0));
        expandBtn->SetInternalParent(sample, false);

        sample->SetHeadHeight(20);

        o2UI.AddWidgetStyle(sample, "expand with caption");
    }
}
// --- META ---

DECLARE_CLASS(o2::BasicUIStyleBuilder, o2__BasicUIStyleBuilder);
// --- END META ---
