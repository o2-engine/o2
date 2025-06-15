#include "o2Editor/stdafx.h"
#include "AnimationWindow.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Windows/AnimationWindow/CurvesSheet.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/PropertiesListDlg.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2/Utils/Editor/EditorScope.h"

DECLARE_SINGLETON(Editor::AnimationWindow);

namespace Editor
{
    AnimationWindow::AnimationWindow(RefCounter* refCounter):
        IEditorWindow(refCounter), Singleton<AnimationWindow>(refCounter)
    {
        InitializeWindow();
    }

    AnimationWindow::~AnimationWindow()
    {}

#undef DrawText

    void AnimationWindow::Update(float dt)
    {
        if (mPlayer && mOwnPlayer)
            mPlayer->Update(dt);

        if (mTargetActor)
        {
            mTargetActor->Update(dt);
            mTargetActor->UpdateChildren(dt);
        }

        if (mPlayer && mPlayer->IsPlaying() != mPlayPauseToggle->GetValue())
            mPlayPauseToggle->SetValue(mPlayer->IsPlaying());

        if (o2Input.IsKeyDown(VK_F1))
        {
            int line = 0;
            for (int i = 0; i < mActionsList->GetUndoActions().Count(); i++, line++)
                o2Debug.DrawText(Vec2F(0, line*20.0f), mActionsList->GetUndoActions()[i]->GetName());

            for (int i = 0; i < mActionsList->GetRedoActions().Count(); i++, line++)
                o2Debug.DrawText(Vec2F(0, line*20.0f), mActionsList->GetRedoActions()[i]->GetName(), Color4::Red());
        }
    }

    void AnimationWindow::SetAnimation(const Ref<AnimationClip>& animation, Ref<AnimationPlayer> player /*= nullptr*/)
    {
        if (mAnimation)
            mAnimation->onChanged -= THIS_FUNC(OnAnimationChanged);

        if (mPlayer)
        {
            mPlayer->onUpdate -= THIS_FUNC(OnAnimationUpdate);
        }

        if (mAnimationEditable)
            mAnimationEditable.Lock()->EndAnimationEdit();

        mAnimation = animation;
        mPlayer = player;
        mOwnPlayer = false;
        mAnimationEditable = nullptr;

        if (!mAnimation)
            return;

        mAnimation->onChanged += THIS_FUNC(OnAnimationChanged);

        if (mPlayer)
        {
            mPlayer->Stop();
            mLoopToggle->SetValue(mPlayer->GetLoop() == Loop::Repeat);

            mPlayer->onUpdate += THIS_FUNC(OnAnimationUpdate);
        }
        else if (mAnimation)
            mLoopToggle->SetValue(mAnimation->GetLoop() == Loop::Repeat);

        mPlayPauseToggle->SetValue(false);

        mHandlesSheet->SetAnimation(animation);
        mTimeline->SetAnimation(animation, player);
        mTree->SetAnimation(animation);
        mCurves->SetAnimation(animation);
    }

    void AnimationWindow::SetAnimationEditable(const Ref<IEditableAnimation>& editable)
    {
        if (mAnimationEditable)
            mAnimationEditable.Lock()->EndAnimationEdit();

        mAnimationEditable = editable;

        if (mAnimationEditable)
        {
            mPreviewToggle->value = true;
            mAnimationEditable.Lock()->BeginAnimationEdit();
        }
    }

    void AnimationWindow::SetTarget(const Ref<Actor>& actor)
    {
        mTargetActor = actor;

        if (!mPlayer && mAnimation)
        {
            mPlayer = mmake<AnimationPlayer>(mTargetActor.Get(), Ref(mAnimation));
            SetAnimation(mAnimation, mPlayer);
            mOwnPlayer = true;
        }
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

    Ref<RefCounterable> AnimationWindow::CastToRefCounterable(const Ref<AnimationWindow>& ref)
    {
        return DynamicCast<Singleton<AnimationWindow>>(ref);
    }

    void AnimationWindow::OnClosed()
    {
        SetAnimation(nullptr);
        SetAnimationEditable(nullptr);
    }

    void AnimationWindow::InitializeWindow()
    {
        PushEditorScopeOnStack scope;

        mWindow->caption = "Animation";
        mWindow->name = "animation window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_animation_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 0, 0, 18));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 18));

        InitializeUpPanel();

        mWorkArea = mmake<Widget>();
        *mWorkArea->layout = WidgetLayout::BothStretch(0, 0, 0, 18);
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
        mUpPanel = mmake<Widget>();
        *mUpPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20);
        mUpPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_back.png"), Layout::BothStretch(-4, -4, -5, -5));
        mWindow->AddChild(mUpPanel);

        mControlsPanel = mmake<HorizontalLayout>();
        mControlsPanel->name = "controls panel";
        *mControlsPanel->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(mTreeViewWidth, 20.0f));
        mControlsPanel->expandWidth = false;
        mControlsPanel->AddLayer("back", mmake<Sprite>("ui/UI4_square_field.png"), Layout::BothStretch(-4, -4, -5, -5));

        mPreviewToggle = o2UI.CreateWidget<Toggle>("menu preview");
        mPreviewToggle->onToggle = THIS_FUNC(OnMenuPreviewToggle);
        mControlsPanel->AddChild(mPreviewToggle);

        mRecordToggle = o2UI.CreateWidget<Toggle>("menu record");
        mRecordToggle->onToggle = THIS_FUNC(OnMenuRecordToggle);
        mControlsPanel->AddChild(mRecordToggle);

        mRewindLeft = o2UI.CreateWidget<Button>("menu rewind left");
        mControlsPanel->AddChild(mRewindLeft);

        mMoveLeft = o2UI.CreateWidget<Button>("menu move left");
        mControlsPanel->AddChild(mMoveLeft);

        mPlayPauseToggle = o2UI.CreateWidget<Toggle>("menu play-stop");
        mPlayPauseToggle->SetValue(false);
        mPlayPauseToggle->onToggleByUser = THIS_FUNC(OnPlayPauseToggled);
        mControlsPanel->AddChild(mPlayPauseToggle);

        mMoveRight = o2UI.CreateWidget<Button>("menu move right");
        mControlsPanel->AddChild(mMoveRight);

        mRewindRight = o2UI.CreateWidget<Button>("menu rewind right");
        mControlsPanel->AddChild(mRewindRight);

        mLoopToggle = o2UI.CreateWidget<Toggle>("menu loop-nonloop");
        mLoopToggle->SetValue(true);
        mLoopToggle->onToggleByUser = THIS_FUNC(OnLoopToggled);
        mControlsPanel->AddChild(mLoopToggle);

        mCurvesToggle = o2UI.CreateWidget<Toggle>("menu curves");
        mCurvesToggle->SetValue(false);
        mCurvesToggle->onToggleByUser = [&](bool value) { SetCurvesMode(value); };
        mControlsPanel->AddChild(mCurvesToggle);

        mPropertiesButton = o2UI.CreateWidget<Button>("menu properties");
        mPropertiesButton->onClick = [&]() { PropertiesListDlg::Show(mAnimation, mTargetActor); };
        mControlsPanel->AddChild(mPropertiesButton);

        mAddKeyButton = o2UI.CreateWidget<Button>("menu add key");
        mControlsPanel->AddChild(mAddKeyButton);

        mUpPanel->AddChild(mControlsPanel);
    }

    void AnimationWindow::InitializeSeparatorHandle()
    {
        mTreeSeparatorHandle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/UI4_Ver_separator.png"));
        mTreeSeparatorHandle->GetRegularDrawable()->pivot = Vec2F(0.5f, 0.5f);
        mTreeSeparatorHandle->GetRegularDrawable()->szPivot = Vec2F(4, mTreeSeparatorHandle->GetRegularDrawable()->szPivot.Get().y);

        mTreeSeparatorHandle->onChangedPos = [&](const Vec2F& point) {
            mTreeViewWidth = Math::Max(point.x, mMinTreeViewWidth);
            mControlsPanel->layout->right = mTreeViewWidth;
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

    void AnimationWindow::OnAnimationChanged()
    {
        mTree->OnAnimationChanged();
        mCurves->OnAnimationChanged();

        if (mPlayer)
            mPlayer->SetTime(mPlayer->GetTime());
    }

    void AnimationWindow::OnAnimationUpdate(float time)
    {
        if (!mDisableTimeTracking)
            mTimeline->mTimeCursor = mPlayer->GetLoopTime();
    }

    void AnimationWindow::OnPlayPauseToggled(bool play)
    {
        if (mPlayer)
        {
            if (mPlayer->GetLoop() != Loop::Repeat && Math::Equals(mPlayer->GetTime(), mPlayer->GetDuration()))
                mPlayer->SetTime(0.0f);

            mPlayer->SetPlaying(play);
        }
    }

    void AnimationWindow::OnLoopToggled(bool loop)
    {
        if (mAnimation)
            mAnimation->SetLoop(loop ? Loop::Repeat : Loop::None);

        if (mPlayer)
            mPlayer->SetLoop(loop ? Loop::Repeat : Loop::None);

        o2Scene.OnObjectChanged(mTargetActor);
    }

    void AnimationWindow::OnSearchEdited(const WString& search)
    {

    }

    void AnimationWindow::OnMenuFilterPressed()
    {

    }

    void AnimationWindow::OnMenuPreviewToggle(bool value)
    {
        if (mAnimationEditable)
        {
            if (value)
                mAnimationEditable.Lock()->BeginAnimationEdit();
            else
                mAnimationEditable.Lock()->EndAnimationEdit();
        }
    }

    void AnimationWindow::OnMenuRecordToggle(bool value)
    {

    }

}
// --- META ---

DECLARE_CLASS(Editor::AnimationWindow, Editor__AnimationWindow);
// --- END META ---
