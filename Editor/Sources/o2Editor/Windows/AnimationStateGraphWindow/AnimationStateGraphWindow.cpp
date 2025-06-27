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
	const Type& AnimationStateGraphWindow::GetAssetType() const
	{
		return TypeOf(AnimationStateGraphAsset);
	}

	Ref<RefCounterable> AnimationStateGraphWindow::CastToRefCounterable(const Ref<AnimationStateGraphWindow>& ref)
	{
		return DynamicCast<Singleton<AnimationStateGraphWindow>>(ref);
	}

	AnimationStateGraphWindow::AnimationStateGraphWindow(RefCounter* refCounter) :
        Singleton<AnimationStateGraphWindow>(refCounter), IAssetEditorWindow(refCounter)
    {
        InitializeWindow();
    }

    AnimationStateGraphWindow::~AnimationStateGraphWindow()
    {}

    void AnimationStateGraphWindow::InitializeWindow()
    {
        IAssetEditorWindow::InitializeWindow();

        mWindow->caption = "State Graph";
        mWindow->name = "animation state graph window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_log_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));

		mEditor = mmake<AnimationStateGraphEditor>();
		*mEditor->layout = WidgetLayout::BothStretch(0, 0, 0, 20);
		mWindow->AddChild(mEditor);

		auto horScroll = o2UI.CreateHorScrollBar();
		*horScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 5, 15, 10);
		mEditor->SetHorScrollbar(horScroll);

		auto verScroll = o2UI.CreateVerScrollBar();
		*verScroll->layout = WidgetLayout::VerStretch(HorAlign::Right, 5, 15, 10);
		mEditor->SetVerScrollbar(verScroll);

        mEditor->SetSelectionSpriteImage(AssetRef<ImageAsset>("ui/UI_Window_place.png"));
    }

	void AnimationStateGraphWindow::OnStartEditingAsset()
	{
		if (!mEditingComponent)
			mEditor->SetGraph(AssetRef<AnimationStateGraphAsset>(mEditingAsset), nullptr);
	}

	void AnimationStateGraphWindow::OnCompletedEditingAsset()
	{}

	void AnimationStateGraphWindow::OnStartEditingComponent()
	{
		mEditor->SetGraph(AssetRef<AnimationStateGraphAsset>(mEditingAsset),
						  DynamicCast<AnimationStateGraphComponent>(mEditingComponent));
	}

	void AnimationStateGraphWindow::OnCompletedEditingComponent()
	{}

	void AnimationStateGraphWindow::OnComponentPreviewEnabled()
	{
		mEditor->SetPreviewEnabled(true);
	}

	void AnimationStateGraphWindow::OnComponentPreviewDisabled()
	{
		mEditor->SetPreviewEnabled(false);
	}

	void AnimationStateGraphWindow::ComponentSetAsset(const AssetRef<Asset>& asset)
	{
		if (!mEditingComponent)
			return;

		AssetRef<AnimationStateGraphAsset> graphAsset = asset;
		auto component = DynamicCast<AnimationStateGraphComponent>(mEditingComponent);

		if (graphAsset && component)
		{
			component->SetGraph(graphAsset);
			mEditor->SetGraph(graphAsset, component);
		}
	}

}
// --- META ---

DECLARE_CLASS(Editor::AnimationStateGraphWindow, Editor__AnimationStateGraphWindow);
// --- END META ---
