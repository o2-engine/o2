#include "o2/Utils/Types/Ref.h"
#include "o2Editor/stdafx.h"
#include "AnimationWindow.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AnimationWindow/CurvesSheet.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/PropertiesListDlg.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"

DECLARE_SINGLETON(Editor::AnimationWindow);

namespace Editor
{
    AnimationWindow::AnimationWindow(RefCounter* refCounter):
        IAssetEditorWindow(refCounter), Singleton<AnimationWindow>(refCounter)
    {
        InitializeWindow();
    }

    AnimationWindow::~AnimationWindow()
    {}

    void AnimationWindow::Update(float dt)
    {
        if (mPreviewPlayer && mOwnPreviewPlayer)
            mPreviewPlayer->Update(dt);

        if (auto targetActor = mTargetActor.Lock())
            targetActor->UpdateTransform();

        if (mPreviewPlayer && mPreviewPlayer->IsPlaying() != mPlayPauseToggle->GetValue())
            mPlayPauseToggle->SetValue(mPreviewPlayer->IsPlaying());

        IAssetEditorWindow::Update(dt);
    }

    void AnimationWindow::SetCurvesMode(bool enabled)
    {
        mCurves->SetEnabled(enabled);
        mHandlesSheet->SetEnabled(!enabled);
        mTimeline->SetViewMoveDisabled(enabled);
        mTimeScroll->enabled = !enabled;
        mTree->SetCurveViewMode(enabled);
    }

    bool AnimationWindow::IsCurvesMode() const
    {
        return mCurves->IsEnabled();
    }

	const Type& AnimationWindow::GetAssetType() const
	{
		return TypeOf(AnimationAsset);
	}

	Ref<RefCounterable> AnimationWindow::CastToRefCounterable(const Ref<AnimationWindow>& ref)
    {
        return DynamicCast<Singleton<AnimationWindow>>(ref);
    }

    void AnimationWindow::OnClosed()
    {
        SetComponentPreview(false);
    }

    void AnimationWindow::InitializeWindow()
    {
        PushEditorScopeOnStack scope;

        mWindow->caption = "Animation";
        mWindow->name = "animation window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_animation_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 0, 0, 19));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 18));

        InitializeUpPanel();

        mWorkArea = mmake<Widget>();
        *mWorkArea->layout = WidgetLayout::BothStretch(0, 0, 0, 20);
        mWindow->AddChild(mWorkArea);

        InitializeHandlesSheet();
        InitializeTree();
        InitializeCurvesSheet();
        InitializeTimeline();
        InitializeSeparatorHandle();

        auto thisRef = Ref(this);
        mCurves->mAnimationWindow = thisRef;
        mHandlesSheet->mAnimationWindow = thisRef;
        mTimeline->mAnimationWindow = thisRef;
        mTree->mAnimationWindow = thisRef;

        SetCurvesMode(false);

        PropertiesListDlg::InitializeSingleton();
    }

	void AnimationWindow::OnFocused()
	{
        mCurves->OnAnimationWindowFocused();
	}

	void AnimationWindow::OnUnfocused()
	{
        mCurves->OnAnimationWindowUnfocused();
	}

	String AnimationWindow::GetWindowTitle() const
    {
        return "Animation";
    }

    void AnimationWindow::InitializeHandlesSheet()
    {
        mHandlesSheet = mmake<KeyHandlesSheet>();
        *mHandlesSheet->layout = WidgetLayout::BothStretch(mTreeViewWidth, 0, 0, 0);
        mWorkArea->AddChild(mHandlesSheet);
    }

    void AnimationWindow::InitializeTree()
    {
        mTree = o2UI.CreateWidget<AnimationTree>();
        *mTree->layout = WidgetLayout::BothStretch();
        mTree->SetTreeWidth(mTreeViewWidth);
        mWorkArea->AddChild(mTree);
    }

    void AnimationWindow::InitializeTimeline()
    {
        mTimeline = mmake<AnimationTimeline>();
        *mTimeline->layout = WidgetLayout::BothStretch(mTreeViewWidth, 0.0f, 0.0f, 0.0f);

        mTimeScroll = o2UI.CreateHorScrollBar();
        *mTimeScroll->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 10, 10, 10, 0);

        mTimeline->SetScrollBar(mTimeScroll);

        mWindow->AddChild(mTimeline);
    }

    void AnimationWindow::InitializeCurvesSheet()
    {
        mCurves = mmake<CurvesSheet>();
        *mCurves->layout = WidgetLayout::BothStretch(mTreeViewWidth, 0, 0, 0);
        mCurves->Disable();
        mWorkArea->AddChild(mCurves);
    }

    void AnimationWindow::InitializeUpPanel()
    {
        mRecordToggle = o2UI.CreateWidget<Toggle>("menu record");
        mRecordToggle->onToggle = THIS_FUNC(OnMenuRecordToggle);
        mButtonsPanel->AddChild(mRecordToggle);

        mRewindLeft = o2UI.CreateWidget<Button>("menu rewind left");
        mButtonsPanel->AddChild(mRewindLeft);

        mMoveLeft = o2UI.CreateWidget<Button>("menu move left");
        mButtonsPanel->AddChild(mMoveLeft);

        mPlayPauseToggle = o2UI.CreateWidget<Toggle>("menu play-stop");
        mPlayPauseToggle->SetValue(false);
        mPlayPauseToggle->onToggleByUser = THIS_FUNC(OnPlayPauseToggled);
        mButtonsPanel->AddChild(mPlayPauseToggle);

        mMoveRight = o2UI.CreateWidget<Button>("menu move right");
        mButtonsPanel->AddChild(mMoveRight);

        mRewindRight = o2UI.CreateWidget<Button>("menu rewind right");
        mButtonsPanel->AddChild(mRewindRight);

        mLoopToggle = o2UI.CreateWidget<Toggle>("menu loop-nonloop");
        mLoopToggle->SetValue(true);
        mLoopToggle->onToggleByUser = THIS_FUNC(OnLoopToggled);
        mButtonsPanel->AddChild(mLoopToggle); 

        mCurvesToggle = o2UI.CreateWidget<Toggle>("menu curves");
        mCurvesToggle->SetValue(false);
        mCurvesToggle->onToggleByUser = [&](bool value) { SetCurvesMode(value); };
        mButtonsPanel->AddChild(mCurvesToggle); 

        mPropertiesButton = o2UI.CreateWidget<Button>("menu properties");
        mPropertiesButton->onClick = [&]() { PropertiesListDlg::Show(mAnimation.Lock(), mTargetActor.Lock()); };
        mButtonsPanel->AddChild(mPropertiesButton);

        mAddKeyButton = o2UI.CreateWidget<Button>("menu add key");
        mButtonsPanel->AddChild(mAddKeyButton); 
    }

    void AnimationWindow::InitializeSeparatorHandle()
    {
        mTreeSeparatorHandle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/UI4_Ver_separator.png"));
        mTreeSeparatorHandle->GetRegularDrawable()->pivot = Vec2F(0.5f, 0.5f);
        mTreeSeparatorHandle->GetRegularDrawable()->szPivot = Vec2F(4, mTreeSeparatorHandle->GetRegularDrawable()->szPivot.Get().y);

        mTreeSeparatorHandle->onChangedPos = [&](const Vec2F& point) {
            mTreeViewWidth = Math::Max(point.x, mMinTreeViewWidth);
            mTimeline->layout->left = mTreeViewWidth;
            mHandlesSheet->layout->left = mTreeViewWidth;
            mCurves->layout->left = mTreeViewWidth;
            mTree->SetTreeWidth(mTreeViewWidth);
        };

        mTreeSeparatorHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(Math::Max(point.x, mMinTreeViewWidth), mWorkArea->layout->GetHeight()*0.5f);
        };

        mTreeSeparatorHandle->onLayoutUpdated = [&]() {
            mTreeSeparatorHandle->SetDrawablesSize(Vec2F(5.0f, mWorkArea->layout->GetHeight() + 50.0f));
            mTreeSeparatorHandle->SetPosition(Vec2F(mTreeViewWidth, mWorkArea->layout->GetHeight()*0.5f));
        };

        mTreeSeparatorHandle->cursorType = CursorType::SizeWE;

        mWorkArea->AddChild(mTreeSeparatorHandle);
    }

    void AnimationWindow::OnStartEditingAsset()
    {
        auto animationAsset = DynamicCast<AnimationAsset>(mEditingAsset.Lock());
        mAnimationAsset = animationAsset;

        auto animation = animationAsset->animation;
        mAnimation = animation;

        if (!mAnimation)
            return;

        if (mPreviewPlayer)
        {
            mPreviewPlayer->onUpdate -= THIS_FUNC(OnAnimationUpdate);
            mPreviewPlayer = nullptr;
            mOwnPreviewPlayer = false;
        }

        mTargetActor = nullptr;

        animation->onChanged += THIS_FUNC(OnAnimationChanged);

        mLoopToggle->SetValue(animation->GetLoop() == Loop::Repeat);
        mPlayPauseToggle->SetValue(false);

        mHandlesSheet->SetAnimation(animation);
        mTimeline->SetAnimation(animation, nullptr);
        mTree->SetAnimation(animation);
        mCurves->SetAnimation(animation);
    }

    void AnimationWindow::OnCompletedEditingAsset()
    {
        if (auto animation = mAnimation.Lock())
            animation->onChanged -= THIS_FUNC(OnAnimationChanged);
    }

	void AnimationWindow::OnAssetEditablePreviewEnabled()
	{
		InitializeOwnAnimationPlayer();
	}

	void AnimationWindow::OnAssetEditablePreviewDisabled()
	{
		InitializeExternalAnimationPlayer();
	}

    void AnimationWindow::InitializeOwnAnimationPlayer()
	{
		// Reset previous player if exists
        if (mPreviewPlayer)
			mPreviewPlayer->onUpdate -= THIS_FUNC(OnAnimationUpdate);

		mPreviewPlayer = nullptr;

		// Get target actor from editable preview
        if (auto editablePreview = mEditingAssetEditablePreview.Lock())
		    mTargetActor = editablePreview->GetPreviewActor();

        if (!mTargetActor || !mAnimation)
            return;

        auto animation = mAnimation.Lock();

		// Create new own animation player
		mOwnPreviewPlayer = true;
        mPreviewPlayer = mmake<AnimationPlayer>(mTargetActor.Lock().Get(), Ref(animation));
        mPreviewPlayer->onUpdate += THIS_FUNC(OnAnimationUpdate);

		mPreviewPlayer->SetTime(0.0f);

		mTimeline->SetAnimation(animation, mPreviewPlayer);
		mTree->SetAnimation(animation);
    }

	void AnimationWindow::InitializeExternalAnimationPlayer()
	{
		// Reset previous player if exists
		if (mPreviewPlayer)
			mPreviewPlayer->onUpdate -= THIS_FUNC(OnAnimationUpdate);

		mPreviewPlayer = nullptr;

		// Get target actor and animation player from editable preview
        if (auto editablePreview = mEditingAssetEditablePreview.Lock())
        {
            mTargetActor = editablePreview->GetPreviewActor();

            if (auto animationPreviewEditable = DynamicCast<AnimationAssetEditablePreview>(editablePreview))
            {
                mPreviewPlayer = DynamicCast<AnimationPlayer>(animationPreviewEditable->GetPreviewPlayer());
				mOwnPreviewPlayer = false;
            }
		}

		if (!mTargetActor || !mAnimation || !mPreviewPlayer)
			return;

		// Subscribe to animation update
		mPreviewPlayer->onUpdate += THIS_FUNC(OnAnimationUpdate);

		mPreviewPlayer->SetTime(0.0f);

		mTimeline->SetAnimation(mAnimation.Lock(), mPreviewPlayer);
		mTree->SetAnimation(mAnimation.Lock());
    }

    bool AnimationWindow::IsComponentPreviewAvailable() const
    {
        return true;
    }

    void AnimationWindow::OnAnimationChanged()
    {
        mTree->OnAnimationChanged();
        mCurves->OnAnimationChanged();

        if (mPreviewPlayer)
            mPreviewPlayer->SetTime(mPreviewPlayer->GetTime());
    }

    void AnimationWindow::OnAnimationUpdate(float time)
    {
        if (!mDisableTimeTracking)
            mTimeline->mTimeCursor = mPreviewPlayer->GetLoopTime();
    }

    void AnimationWindow::OnPlayPauseToggled(bool play)
    {
        if (mPreviewPlayer)
        {
            if (mPreviewPlayer->GetLoop() != Loop::Repeat && Math::Equals(mPreviewPlayer->GetTime(), mPreviewPlayer->GetDuration()))
                mPreviewPlayer->SetTime(0.0f);

            mPreviewPlayer->SetPlaying(play);
        }
    }

    void AnimationWindow::OnLoopToggled(bool loop)
    {
        if (auto animation = mAnimation.Lock())
            animation->SetLoop(loop ? Loop::Repeat : Loop::None);

        if (mPreviewPlayer)
            mPreviewPlayer->SetLoop(loop ? Loop::Repeat : Loop::None);

        if (auto targetActor = mTargetActor.Lock())
            o2Scene.OnObjectChanged(targetActor);
    }

    void AnimationWindow::OnSearchEdited(const WString& search)
    {

    }

    void AnimationWindow::OnMenuFilterPressed()
    {

    }

    void AnimationWindow::OnMenuRecordToggle(bool value)
    {
		mRecording = value;

		if (mRecording)
			o2EditorPropertiesWindow.onPropertyChangeCompleted += THIS_FUNC(OnPropertyChangeCompleted);
		else
			o2EditorPropertiesWindow.onPropertyChangeCompleted -= THIS_FUNC(OnPropertyChangeCompleted);
    }

	void AnimationWindow::OnPropertyChangeCompleted(const Vector<IObject*>& targets, const String& path,
                                                    const Vector<DataDocument>& before, const Vector<DataDocument>& after)
	{
        if (!mRecording)
            return;

		auto targetActorEditable = dynamic_cast<SceneEditableObject*>(mTargetActor.Lock().Get());
        if (!targetActorEditable)
			return;

        for (auto target : targets)
        {
			auto targetEditable = dynamic_cast<SceneEditableObject*>(target);
            if (!targetEditable)
				continue;

			bool isTargetInHierarchy = false;
            String targetHierarchyPath;

			auto targetEditableIt = targetEditable;
            while (targetEditableIt)
            {
                if (targetEditableIt == targetActorEditable)
                {
					isTargetInHierarchy = true;
                    break;
                }

				targetHierarchyPath = "children/" + targetEditableIt->GetName() + "/" + targetHierarchyPath;
				targetEditableIt = targetEditableIt->GetEditableParent().Get();
            }

			if (!isTargetInHierarchy)
				continue;

			String trackPath = targetHierarchyPath + path;
			o2Debug.Log("Recording property change on track: " + trackPath);

			AddTrackKey(trackPath);

            break;
		}
	}

	void AnimationWindow::AddTrackKey(const String& path)
	{
		auto animation = mAnimation.Lock();
        if (!animation)
			return;

		auto targetEditable = DynamicCast<SceneEditableObject>(mTargetActor.Lock());
        if (!targetEditable)
			return;

		const FieldInfo* fieldInfo = nullptr;
		void* fieldPtr = nullptr;

		if (auto objectType = dynamic_cast<const ObjectType*>(&targetEditable->GetType()))
		{
			void* realTypeObject = objectType->DynamicCastFromIObject(dynamic_cast<IObject*>(targetEditable.Get()));
            fieldPtr = objectType->GetFieldPtr(realTypeObject, path, fieldInfo);
		}

        if (!fieldInfo || !fieldPtr)
        {
            o2Debug.LogError("Failed to add key: field '" + path + "' not found");
            return;
		}

		const Type* fieldType = fieldInfo->GetType();
        if (fieldType->GetUsage() == Type::Usage::Property)
        {
            if (auto propertyType = dynamic_cast<const PropertyType*>(fieldType))
				fieldType = propertyType->GetValueType();
        }

        if (!fieldType)
            return;

        auto existingTrack = animation->GetTrack(path);
        if (!existingTrack)
        {
            existingTrack = animation->AddTrack(path, *fieldType);
            if (!existingTrack)
            {
                o2Debug.LogError("Failed to add key: can't create track for field '" + path + "'");
                return;
            }
        }

        if (!existingTrack)
			return;

		auto fieldValueProxy = fieldInfo->GetType()->GetValueProxy(fieldPtr);

        if (fieldType == &TypeOf(float))
        {
            if (auto floatTrack = DynamicCast<AnimationTrack<float>>(existingTrack))
            {
                if (auto floatProxy = DynamicCast<IValueProxy<float>>(fieldValueProxy))
                {
                    float value = floatProxy->GetValue();
                    floatTrack->AddKey(mTimeline->mTimeCursor, value);
				}
            }
        }
        else if (fieldType == &TypeOf(int))
        {
            if (auto intTrack = DynamicCast<AnimationTrack<int>>(existingTrack))
            {
                if (auto intProxy = DynamicCast<IValueProxy<int>>(fieldValueProxy))
                {
                    int value = intProxy->GetValue();
                    intTrack->AddKey(mTimeline->mTimeCursor, value);
                }
            }
        }
        else if (fieldType == &TypeOf(Vec2F))
		{
			if (auto vec2Track = DynamicCast<AnimationTrack<Vec2F>>(existingTrack))
			{
				if (auto vec2Proxy = DynamicCast<IValueProxy<Vec2F>>(fieldValueProxy))
				{
					Vec2F value = vec2Proxy->GetValue();
					vec2Track->spline->InsertKey(vec2Track->spline->GetNextKey(mTimeline->mTimeCursor), value);

                    if (vec2Track->timeCurve->GetKeys().IsEmpty())
                        vec2Track->timeCurve->InsertKey(mTimeline->mTimeCursor, 0.0f);
                    else if (vec2Track->timeCurve->GetKeys().Count() == 1)
                        vec2Track->timeCurve->InsertKey(mTimeline->mTimeCursor, 1.0f);
				}
			}
        }
		else if (fieldType == &TypeOf(Color4)) 
        {
            if (auto colorTrack = DynamicCast<AnimationTrack<Color4>>(existingTrack))
            {
                if (auto colorProxy = DynamicCast<IValueProxy<Color4>>(fieldValueProxy))
                {
                    Color4 value = colorProxy->GetValue();
                    colorTrack->AddKey(mTimeline->mTimeCursor, value);
                }
			}
		}

		mTree->OnAnimationChanged();
	}

}
// --- META ---

DECLARE_CLASS(Editor::AnimationWindow, Editor__AnimationWindow);
// --- END META ---
