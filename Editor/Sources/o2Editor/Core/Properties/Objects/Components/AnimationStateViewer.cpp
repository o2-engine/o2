#include "o2Editor/stdafx.h"
#include "AnimationStateViewer.h"

#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
	const Type* AnimationStateViewer::GetViewingObjectType() const
	{
		return GetViewingObjectTypeStatic();
	}

	const Type* AnimationStateViewer::GetViewingObjectTypeStatic()
	{
		return &TypeOf(AnimationState);
	}

	Ref<Spoiler> AnimationStateViewer::CreateSpoiler()
	{
		mSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");

		mPlayPause = o2UI.CreateWidget<Toggle>("animation state play-stop");
		mPlayPause->name = "play-stop";
		*mPlayPause->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(7, 1));
		mPlayPause->onToggle = THIS_FUNC(OnPlayPauseToggled);
		mSpoiler->AddInternalWidget(mPlayPause);

		mEditBtn = o2UI.CreateWidget<Button>("edit animation state");
		mEditBtn->name = "edit";
		*mEditBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-40, 1));
		mEditBtn->onClick = THIS_FUNC(OnEditPressed);
		mSpoiler->AddInternalWidget(mEditBtn);

		mLooped = o2UI.CreateWidget<Toggle>("animation state loop");
		mLooped->name = "loop";
		*mLooped->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-20, 1));
		mLooped->onToggle = THIS_FUNC(OnLoopToggled);
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

		return mSpoiler;
	}

	void AnimationStateViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->onUpdate -= THIS_FUNC(OnAnimationUpdated);

		mSubscribedPlayer = nullptr;

		if (!targetObjets.IsEmpty())
		{
			mSubscribedPlayer = dynamic_cast<AnimationState*>(targetObjets.Last().first)->player;
			mSubscribedPlayer.Lock()->onUpdate += THIS_FUNC(OnAnimationUpdated);
		}
	}

	void AnimationStateViewer::OnFree()
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->onUpdate -= THIS_FUNC(OnAnimationUpdated);

		mSubscribedPlayer = nullptr;
	}

	void AnimationStateViewer::OnPlayPauseToggled(bool play)
	{
		if (auto subscribedPlayer = mSubscribedPlayer.Lock())
		{
			if (play && subscribedPlayer->GetRelativeTime() >= 1.0f - FLT_EPSILON)
				subscribedPlayer->SetRelTime(0.0f);

			subscribedPlayer->SetPlaying(play);
		}

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void AnimationStateViewer::OnLoopToggled(bool looped)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->SetLoop(looped ? Loop::Repeat : Loop::None);

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void AnimationStateViewer::OnEditPressed()
	{
		if (!mTargetObjects.IsEmpty())
		{
			auto animationState = dynamic_cast<AnimationState*>(mTargetObjects.Last().first);
			auto animationRef = animationState->GetAnimation();
			if (!animationRef) {
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
	}

	void AnimationStateViewer::OnTimeProgressChanged(float value)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->SetRelTime(value);
	}

	void AnimationStateViewer::OnAnimationUpdated(float time)
	{
		if (auto subscribedPlayer = mSubscribedPlayer.Lock()) 
		{
			mTimeProgress->value = subscribedPlayer->GetLoopTime()/ subscribedPlayer->GetDuration();
			mPlayPause->value = subscribedPlayer->IsPlaying();
		}
	}

}
// --- META ---

DECLARE_CLASS(Editor::AnimationStateViewer, Editor__AnimationStateViewer);
// --- END META ---
