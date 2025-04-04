#include "o2Editor/stdafx.h"
#include "AnimationStateGraphWindow.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/System/Time/Time.h"

DECLARE_SINGLETON(Editor::AnimationStateGraphWindow);

namespace Editor
{
    void AnimationStateGraphWindow::Update(float dt)
    {
    }

	void AnimationStateGraphWindow::SetGraph(const Ref<AnimationStateGraphAsset>& graph, 
                                             const Ref<AnimationStateGraphComponent>& component)
	{
		mEditor->SetGraph(graph, component);
	}

	Ref<RefCounterable> AnimationStateGraphWindow::CastToRefCounterable(const Ref<AnimationStateGraphWindow>& ref)
	{
		return DynamicCast<Singleton<AnimationStateGraphWindow>>(ref);
	}

	AnimationStateGraphWindow::AnimationStateGraphWindow(RefCounter* refCounter) :
        Singleton<AnimationStateGraphWindow>(refCounter), IEditorWindow(refCounter)
    {
        InitializeWindow();
    }

    AnimationStateGraphWindow::~AnimationStateGraphWindow()
    {}

    void AnimationStateGraphWindow::InitializeWindow()
    {
        mWindow->caption = "State Graph";
        mWindow->name = "animation state graph window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_log_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 5, 5, 16));
        mWindow->SetClippingLayout(Layout::BothStretch(1, 1, 1, 16));

		mEditor = mmake<AnimationStateGraphEditor>();
		*mEditor->layout = WidgetLayout::BothStretch(5, 5, 5, 5);

		auto horScroll = o2UI.CreateHorScrollBar();
		*horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 10, -10);
        mEditor->SetHorScrollbar(horScroll);

		auto verScroll = o2UI.CreateVerScrollBar();
		*verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 10, -10);
        mEditor->SetVerScrollbar(verScroll);

        mEditor->SetSelectionSpriteImage(AssetRef<ImageAsset>("ui/UI_Window_place.png"));

		mWindow->AddChild(mEditor);
    }
}
// --- META ---

DECLARE_CLASS(Editor::AnimationStateGraphWindow, Editor__AnimationStateGraphWindow);
// --- END META ---
