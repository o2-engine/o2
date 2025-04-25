#include "o2Editor/stdafx.h"
#include "GraphAnimationStateViewer.h"

#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/Actor.h"
#include "o2/Animation/AnimationState.h"
#include "o2Editor/AnimationStateGraphWindow/AnimationStateGraphEditor.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	const Type* GraphAnimationStateViewer::GetViewingObjectType() const
	{
		if (mRealObjectType)
			return mRealObjectType;

		return GetViewingObjectTypeStatic();
	}

	const Type* GraphAnimationStateViewer::GetViewingObjectTypeStatic()
	{
		return &TypeOf(AnimationStateGraphEditor::StateAnimation);
	}

	Ref<Spoiler> GraphAnimationStateViewer::CreateSpoiler(const Ref<Widget>& parent)
	{
		mSpoiler = IObjectPropertiesViewer::CreateSpoiler(parent);

		mPlayPause = o2UI.CreateWidget<Toggle>("animation state play-stop");
		mPlayPause->name = "play-stop";
		*mPlayPause->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(7, 1));
		mPlayPause->onToggleByUser = THIS_FUNC(OnPlayPauseToggled);
		mSpoiler->AddInternalWidget(mPlayPause);

		mEditBtn = o2UI.CreateWidget<Button>("edit animation state");
		mEditBtn->name = "edit";
		*mEditBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-40, 1));
		mEditBtn->onClick = THIS_FUNC(OnEditPressed);
		mSpoiler->AddInternalWidget(mEditBtn);

		mLooped = o2UI.CreateWidget<Toggle>("animation state loop");
		mLooped->name = "loop";
		*mLooped->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-20, 1));
		mLooped->onToggleByUser = THIS_FUNC(OnLoopToggled);
		mSpoiler->AddInternalWidget(mLooped);

		mTimeProgress = o2UI.CreateWidget<HorizontalProgress>("animation state bar");
		mTimeProgress->name = "bar";
		*mTimeProgress->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 2, 18);
		mTimeProgress->onChangeByUser = THIS_FUNC(OnTimeProgressChanged);
		mSpoiler->AddInternalWidget(mTimeProgress);

		if (auto textLayer = GetSpoiler()->GetLayer("caption"))
		{
			textLayer->layout.offsetLeft = 27;
			textLayer->layout.offsetBottom = -19;
			textLayer->layout.offsetTop = 1;
		}

		if (auto header = parent->GetChildByType<Widget>("caption/header"))
		{
			auto spacer = mmake<Widget>();
			spacer->layout->maxWidth = 40;
			header->AddChild(spacer, 1);
		}

		return mSpoiler;
	}

	void GraphAnimationStateViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->onUpdate -= THIS_FUNC(OnAnimationUpdated);

		mSubscribedPlayer = nullptr;

		if (!targetObjets.IsEmpty())
		{
			auto stateAnimation = dynamic_cast<AnimationStateGraphEditor::StateAnimation*>(targetObjets.Last().first);
			if (stateAnimation && stateAnimation->state)
			{
				auto state = stateAnimation->state.Lock();

				mSubscribedPlayer = Ref(&state->GetPlayer());
				mLooped->value = state->IsLooped();
				mEditBtn->enabled = DynamicCast<AnimationState>(state) != nullptr;
			}

			if (auto player = mSubscribedPlayer.Lock())
			{
				player->onUpdate += THIS_FUNC(OnAnimationUpdated);
				player->onPlay += THIS_FUNC(OnAnimationStarted);
				player->onStop += THIS_FUNC(OnAnimationFinished);

				mPlayPause->value = player->IsPlaying();
			}
		}
	}

	void GraphAnimationStateViewer::OnFree()
	{
		if (auto player = mSubscribedPlayer.Lock())
		{
			player->onUpdate -= THIS_FUNC(OnAnimationUpdated);
			player->onPlay -= THIS_FUNC(OnAnimationStarted);
			player->onStop -= THIS_FUNC(OnAnimationFinished);
		}

		mSubscribedPlayer = nullptr;
	}

	void GraphAnimationStateViewer::OnPlayPauseToggled(bool play)
	{
		if (auto stateWidget = mPropertiesContext->FindOnStack<AnimationStateGraphEditor::StateWidget>())
		{
			if (auto component = stateWidget->editor.Lock()->mComponent.Lock())
			{
				if (play)
					component->GoToState(stateWidget->state.Lock());
			}
		}

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void GraphAnimationStateViewer::OnLoopToggled(bool looped)
	{
		for (auto& targets : mTargetObjects)
		{
			if (!targets.first)
				continue;

			auto stateAnimation = dynamic_cast<AnimationStateGraphEditor::StateAnimation*>(targets.first);
			if (stateAnimation && stateAnimation->state)
				stateAnimation->state.Lock()->SetLooped(looped);
		}

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void GraphAnimationStateViewer::OnEditPressed()
	{
		if (mTargetObjects.IsEmpty())
			return;

		auto state = dynamic_cast<AnimationStateGraphEditor::StateAnimation*>(mTargetObjects.Last().first);
		auto animationState = DynamicCast<AnimationState>(state->state.Lock());
		if (!animationState)
			return;

		auto animationRef = animationState->GetAnimation();
		if (!animationRef)
		{
			animationRef.CreateInstance();
			animationState->SetAnimation(animationRef);

			GetSpoiler()->Expand();
		}

		if (animationRef)
		{
			o2EditorAnimationWindow.SetAnimation(animationRef->animation);

			if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
				o2EditorAnimationWindow.SetTarget(DynamicCast<Actor>(o2EditorSceneScreen.GetSelectedObjects().Last()));

			o2EditorAnimationWindow.SetAnimationEditable(Ref(mPropertiesContext->FindOnStack<IEditableAnimation>()));
			o2EditorAnimationWindow.GetWindow()->Focus();
		}
	}

	void GraphAnimationStateViewer::OnTimeProgressChanged(float value)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->SetRelTime(value);
	}

	void GraphAnimationStateViewer::OnAnimationUpdated(float time)
	{
		if (auto subscribedPlayer = mSubscribedPlayer.Lock())
			mTimeProgress->value = subscribedPlayer->GetLoopTime()/ subscribedPlayer->GetDuration();
	}

	void GraphAnimationStateViewer::OnAnimationStarted()
	{
		mPlayPause->value = true;
	}

	void GraphAnimationStateViewer::OnAnimationFinished()
	{
		mPlayPause->value = false;
		mTimeProgress->value = 0.0f;
	}
}
// --- META ---

DECLARE_CLASS(Editor::GraphAnimationStateViewer, Editor__GraphAnimationStateViewer);
// --- END META ---
