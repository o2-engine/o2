#include "o2/Utils/Math/Math.h"
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
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/AnimationWindow/CurvesSheet.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/PropertiesListDlg.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

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
        {
			o2EditorPropertiesWindow.onPropertyChangeCompleted += THIS_FUNC(OnPropertyChangeCompleted);

            for (auto& tool : o2EditorSceneScreen.GetTools())
            {
                if (auto transformTool = DynamicCast<ITransformTool>(tool))
                {
                    transformTool->onTransformBegin += THIS_FUNC(OnToolTransformBegin);
                    transformTool->onTransformEnd += THIS_FUNC(OnToolTransformEnd);
                }
            }
        }
		else
        {
			o2EditorPropertiesWindow.onPropertyChangeCompleted -= THIS_FUNC(OnPropertyChangeCompleted);

            for (auto& tool : o2EditorSceneScreen.GetTools())
            {
                if (auto transformTool = DynamicCast<ITransformTool>(tool))
                {
                    transformTool->onTransformBegin -= THIS_FUNC(OnToolTransformBegin);
                    transformTool->onTransformEnd -= THIS_FUNC(OnToolTransformEnd);
                }
            }
        }
    }

    bool AnimationWindow::CheckObjectIsUnderHierachy(const Ref<SceneEditableObject>& object, String& hierarchyPath)
    {
        auto targetActorEditable = dynamic_cast<SceneEditableObject*>(mTargetActor.Lock().Get());
        if (!targetActorEditable)
            return false;

        bool isUnderHierarchy = false;
        auto objectIt = object;
        while (objectIt)
        {
            if (objectIt == targetActorEditable)
            {
                isUnderHierarchy = true;
                break;
            }

            hierarchyPath = "child/" + objectIt->GetName() + "/" + hierarchyPath;
            objectIt = objectIt->GetEditableParent();
        }

        return isUnderHierarchy;
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

            String targetHierarchyPath;
			bool isTargetUnderHierarchy = CheckObjectIsUnderHierachy(Ref(targetEditable), targetHierarchyPath);

			if (!isTargetUnderHierarchy)
				continue;

			String trackPath = targetHierarchyPath + path;
			o2Debug.Log("Recording property change on track: " + trackPath);

			AddTrackKey(trackPath, mTimeline->mTimeCursor, {});

            break;
		}
	}

    Vector<AnimationWindow::TransformState> AnimationWindow::StoreObjectsTransforms()
    {
        Vector<TransformState> transforms;

        for (auto& object : o2EditorSceneScreen.GetSelectedObjects())
        {
            TransformState transform;
            transform.object = object;

            if (auto actor = DynamicCast<Actor>(object))
            {
                transform.position = actor->transform->GetPosition2D();
                transform.size = actor->transform->GetSize2D();
                transform.scale = actor->transform->GetScale2D();
                transform.pivot = actor->transform->GetPivot2D();
                transform.angle = actor->transform->GetAngleDegrees();
                transform.shear = actor->transform->GetShear2D();
            }

            if (auto widget = DynamicCast<Widget>(object))
            {
                transform.anchorMin = widget->layout->GetAnchorMin();
                transform.anchorMax = widget->layout->GetAnchorMax();
                transform.offsetMin = widget->layout->GetOffsetMin();
                transform.offsetMax = widget->layout->GetOffsetMax();
            }

            transforms.Add(transform);
        }

        return transforms;
    }

    void AnimationWindow::OnToolTransformBegin()
    {
        mBeforeTransforms = StoreObjectsTransforms();
    }

    void AnimationWindow::OnToolTransformEnd()
    {
        auto afterTransforms = StoreObjectsTransforms();
        
        const float time = mTimeline->mTimeCursor;

        mDisableTimeTracking = true;

        for (auto& beforeTransform : mBeforeTransforms)
        {
            String hierarchyPath;
            bool isUnderHierarchy = CheckObjectIsUnderHierachy(beforeTransform.object.Lock(), hierarchyPath);
            if (!isUnderHierarchy)
                continue;

            auto afterTransform = afterTransforms.Find([&](const TransformState& transform) {
                return transform.object == beforeTransform.object;
            });

            if (!afterTransform)
                continue;

            if (beforeTransform == *afterTransform)
                continue;

            const float epsilon = 0.01f;

            if (auto widget = DynamicCast<Widget>(beforeTransform.object.Lock()))
            {
                if (!Math::Equals(beforeTransform.anchorMax.x, afterTransform->anchorMax.x, epsilon))
                    AddTrackKey(hierarchyPath + "layout/anchorRight", time, afterTransform->anchorMax.x);

                if (!Math::Equals(beforeTransform.anchorMax.y, afterTransform->anchorMax.y, epsilon))
                    AddTrackKey(hierarchyPath + "layout/anchorTop", time, afterTransform->anchorMax.y);

                if (!Math::Equals(beforeTransform.anchorMin.x, afterTransform->anchorMin.x, epsilon))
                    AddTrackKey(hierarchyPath + "layout/anchorLeft", time, afterTransform->anchorMin.x);

                if (!Math::Equals(beforeTransform.anchorMin.y, afterTransform->anchorMin.y, epsilon))
                    AddTrackKey(hierarchyPath + "layout/anchorBottom", time, afterTransform->anchorMin.y);

                if (!Math::Equals(beforeTransform.offsetMax.x, afterTransform->offsetMax.x, epsilon))
                    AddTrackKey(hierarchyPath + "layout/offsetRight", time, afterTransform->offsetMax.x);

                if (!Math::Equals(beforeTransform.offsetMax.y, afterTransform->offsetMax.y, epsilon))
                    AddTrackKey(hierarchyPath + "layout/offsetTop", time, afterTransform->offsetMax.y);

                if (!Math::Equals(beforeTransform.offsetMin.x, afterTransform->offsetMin.x, epsilon))
                    AddTrackKey(hierarchyPath + "layout/offsetLeft", time, afterTransform->offsetMin.x);

                if (!Math::Equals(beforeTransform.offsetMin.y, afterTransform->offsetMin.y, epsilon))
                    AddTrackKey(hierarchyPath + "layout/offsetBottom", time, afterTransform->offsetMin.y);
            }
            else if (auto actor = DynamicCast<Actor>(beforeTransform.object.Lock()))
            {
                if (!Math::Equals(beforeTransform.position.x, afterTransform->position.x, epsilon))
                    AddTrackKey(hierarchyPath + "transform/positionX", time, afterTransform->position.x);

                if (!Math::Equals(beforeTransform.position.y, afterTransform->position.y, epsilon))
                    AddTrackKey(hierarchyPath + "transform/positionY", time, afterTransform->position.y);

                if (!Math::Equals(beforeTransform.size.x, afterTransform->size.x, epsilon))
                    AddTrackKey(hierarchyPath + "transform/width", time, afterTransform->size.x);

                if (!Math::Equals(beforeTransform.size.y, afterTransform->size.y, epsilon))
                    AddTrackKey(hierarchyPath + "transform/height", time, afterTransform->size.y);

                if (!Math::Equals(beforeTransform.scale.x, afterTransform->scale.x, epsilon))
                    AddTrackKey(hierarchyPath + "transform/scaleX", time, afterTransform->scale.x);

                if (!Math::Equals(beforeTransform.scale.y, afterTransform->scale.y, epsilon))
                    AddTrackKey(hierarchyPath + "transform/scaleY", time, afterTransform->scale.y);

                if (!Math::Equals(beforeTransform.pivot.x, afterTransform->pivot.x, epsilon))
                    AddTrackKey(hierarchyPath + "transform/pivotX", time, afterTransform->pivot.x);

                if (!Math::Equals(beforeTransform.pivot.y, afterTransform->pivot.y, epsilon))
                    AddTrackKey(hierarchyPath + "transform/pivotY", time, afterTransform->pivot.y);

                if (!Math::Equals(beforeTransform.angle, afterTransform->angle, epsilon))
                    AddTrackKey(hierarchyPath + "transform/angleDegrees", time, afterTransform->angle);

                if (!Math::Equals(beforeTransform.shear, afterTransform->shear, epsilon))
                    AddTrackKey(hierarchyPath + "transform/shear2D", time, afterTransform->shear);
            }
        }

        mDisableTimeTracking = false;

        mTimeline->SetTimeCursor(time);
    }

    Ref<IAnimationTrack> AnimationWindow::FindOrCreateTrack(const String& path, const Type& type)
    {
        auto animation = mAnimation.Lock();
        if (!animation)
            return nullptr;

        auto existingTrack = animation->GetTrack(path);
        if (existingTrack)
            return existingTrack;

        return animation->AddTrack(path, type);
    }

	void AnimationWindow::AddTrackKey(const String& path, float time, const Variant<float, int, Vec2F, Color4>& value)
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

		auto fieldValueProxy = fieldInfo->GetType()->GetValueProxy(fieldPtr);

        if (fieldType == &TypeOf(float))
        {
            if (auto floatProxy = DynamicCast<IValueProxy<float>>(fieldValueProxy))
            {
                float keyValue = value.Holds<float>() ? value.Get<float>() : floatProxy->GetValue();
                if (auto floatTrack = DynamicCast<AnimationTrack<float>>(FindOrCreateTrack(path, *fieldType)))
                {
                    floatTrack->RemoveKey(time);
                    floatTrack->AddKey(time, keyValue);
                }
			}
        }
        else if (fieldType == &TypeOf(int))
        {   
            if (auto intProxy = DynamicCast<IValueProxy<int>>(fieldValueProxy))
            {
                int keyValue = value.Holds<int>() ? value.Get<int>() : intProxy->GetValue();
                if (auto intTrack = DynamicCast<AnimationTrack<int>>(FindOrCreateTrack(path, *fieldType)))
                {
                    intTrack->RemoveKey(time);
                    intTrack->AddKey(time, keyValue);
                }
			}
        }
        else if (fieldType == &TypeOf(Vec2F))
		{
            if (auto vec2Proxy = DynamicCast<IValueProxy<Vec2F>>(fieldValueProxy))
            {
                Vec2F keyValue = value.Holds<Vec2F>() ? value.Get<Vec2F>() : vec2Proxy->GetValue();
                if (auto vec2Track = DynamicCast<AnimationTrack<Vec2F>>(FindOrCreateTrack(path, *fieldType)))
                {
                    vec2Track->spline->InsertKey(vec2Track->spline->GetNextKey(time), keyValue);

                    if (vec2Track->timeCurve->GetKeys().IsEmpty())
                        vec2Track->timeCurve->InsertKey(time, 0.0f);
                    else if (vec2Track->timeCurve->GetKeys().Count() == 1)
                        vec2Track->timeCurve->InsertKey(time, 1.0f);
                }
			}
        }
		else if (fieldType == &TypeOf(Color4)) 
        {
            if (auto colorProxy = DynamicCast<IValueProxy<Color4>>(fieldValueProxy))
            {
                Color4 keyValue = value.Holds<Color4>() ? value.Get<Color4>() : colorProxy->GetValue();
                if (auto colorTrack = DynamicCast<AnimationTrack<Color4>>(FindOrCreateTrack(path, *fieldType)))
                {
                    colorTrack->RemoveKey(time);
                    colorTrack->AddKey(time, keyValue);
                }
			}
        }

		mTree->OnAnimationChanged();
	}

    bool AnimationWindow::TransformState::operator==(const TransformState& other) const
    {
        return object == other.object &&
               position == other.position &&
               size == other.size &&
               scale == other.scale &&
               pivot == other.pivot &&
               Math::Equals(angle, other.angle) &&
               Math::Equals(shear, other.shear) &&
               anchorMin == other.anchorMin &&
               anchorMax == other.anchorMax &&
               offsetMin == other.offsetMin &&
               offsetMax == other.offsetMax;
    }
}
// --- META ---

DECLARE_CLASS(Editor::AnimationWindow, Editor__AnimationWindow);
// --- END META ---
