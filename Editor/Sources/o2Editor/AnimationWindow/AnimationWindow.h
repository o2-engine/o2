#pragma once

#include "o2/Utils/Singleton.h"
#include "o2Editor/Core/Actions/ActionsList.h"
#include "o2Editor/Core/WindowsSystem/IEditorWindow.h"

using namespace o2;

namespace o2
{
	FORWARD_CLASS_REF(AnimationClip);
	FORWARD_CLASS_REF(Button);
	FORWARD_CLASS_REF(EditBox);
	FORWARD_CLASS_REF(HorizontalLayout);
	FORWARD_CLASS_REF(HorizontalScrollBar);
	FORWARD_CLASS_REF(Toggle);
	FORWARD_CLASS_REF(WidgetDragHandle);
}

// Editor animation window accessor macros
#define o2EditorAnimationWindow AnimationWindow::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(AnimationTimeline);
    FORWARD_CLASS_REF(AnimationTree);
    FORWARD_CLASS_REF(CurvesSheet);
    FORWARD_CLASS_REF(KeyHandlesSheet);

	// TODO
	// - �������� ����� ����� �����, ��������, �������
	// - �������� assign �������
	// - �������� "������" ��������� �����
	// - ������� �������� ������ ��� Vec2F

	class AnimationWindow : public IEditorWindow, public Singleton<AnimationWindow>
	{
		IOBJECT(AnimationWindow);

	public:
		// Default constructor
		AnimationWindow(RefCounter* refCounter);

		// Destructor
		~AnimationWindow();

		// Updates window logic
		void Update(float dt) override;

		// Sets editing animation
		void SetAnimation(const Ref<AnimationClip>& animation, Ref<AnimationPlayer> player = nullptr);

		// Sets animation editable
		void SetAnimationEditable(const Ref<IEditableAnimation>& editable);

		// Sets target actor
		void SetTarget(const Ref<Actor>& actor);

		// Sets curves or handles mode
		void SetCurvesMode(bool enabled);

		// Returns is curves mode enabled
		bool IsCurvesMode() const;

	protected:
		float mTreeViewWidth = 325.0f;    // Width of tree area. Changed by dragable separator
		float mMinTreeViewWidth = 250.0f; // Minimal tree width

		Ref<Actor>           mTargetActor;       // Target actor on animation
		Ref<AnimationPlayer> mPlayer;            // Animation player
		bool                 mOwnPlayer = false; // Is player owned by this
		Ref<AnimationClip>   mAnimation;         // Editing animation

		WeakRef<IEditableAnimation> mAnimationEditable; // Editable animation holder. Deactivating when editing animation

		bool mDisableTimeTracking = false; // When true animation time changes has no effect

		Ref<Widget> mUpPanel;  // Up panel with control buttons
		Ref<Widget> mWorkArea; // Working area with tree and time line

		Ref<HorizontalLayout> mControlsPanel; // Panel with buttons described below

		Ref<Toggle> mPreviewToggle;    // Preview toggle
		Ref<Toggle> mRecordToggle;     // Record toggle
		Ref<Button> mRewindLeft;       // Rewind animation to start button
		Ref<Button> mMoveLeft;         // Move time one frame left
		Ref<Toggle> mPlayPauseToggle;  // Play - pause toggle
		Ref<Button> mMoveRight;        // Move time one frame right
		Ref<Button> mRewindRight;      // Rewind animation to end
		Ref<Toggle> mLoopToggle;       // Animation loop toggle
		Ref<Toggle> mCurvesToggle;     // Toggle curves view
		Ref<Button> mAddKeyButton;     // Add key on current time button
		Ref<Button> mPropertiesButton; // Open properties window

		Ref<AnimationTimeline>   mTimeline;     // Animation time line
		Ref<HorizontalScrollBar> mTimeScroll;   // Time line horizontal scrollbar
		Ref<AnimationTree>       mTree;         // animation tracks tree
		Ref<KeyHandlesSheet>     mHandlesSheet; // Animation keys handles sheet
		Ref<CurvesSheet>         mCurves;       // Animation curves sheet

		Ref<WidgetDragHandle> mTreeSeparatorHandle; // Tree separator handle. When it moves, it changes size of all dependent widgets

		Ref<ActionsList> mActionsList = mmake<ActionsList>(); // List of actions in animation editor, also injecting into curves editor

	protected:
		// Called when editor window has closed
		void OnClosed() override;

		// Initializes window
		void InitializeWindow();

		// Initializes handles sheet
		void InitializeHandlesSheet();

		// Initializes nodes tree
		void InitializeTree();

		// Initializes timeline and scrollbar
		void InitializeTimeline();

		// Initializes curves sheet widget
		void InitializeCurvesSheet();

		// Initializes up control panel
		void InitializeUpPanel();

		// Initializes separator handle view and events
		void InitializeSeparatorHandle();

		// Called when editing animation changed. Invokes change methods in tree, curves etc
		void OnAnimationChanged();

		// Called when animation has updated
		void OnAnimationUpdate(float time);

		// Called when play/pause button was pressed
		void OnPlayPauseToggled(bool play);

		// Called when loop button was pressed
		void OnLoopToggled(bool loop);

		// Called when search edit box text was changed
		void OnSearchEdited(const WString& search);

		// Called when menu filter button was pressed
		void OnMenuFilterPressed();

		// Called when menu preview button was pressed
		void OnMenuPreviewToggle(bool value);

		// Called when menu record button was pressed
		void OnMenuRecordToggle(bool value);

		friend class AnimationTimeline;
		friend class AnimationTree;
		friend class CurvesSheet;
		friend class KeyHandlesSheet;

		template<typename AnimationTrackType>
		friend class KeyFramesTrackControl;
	};
}
// --- META ---

CLASS_BASES_META(Editor::AnimationWindow)
{
    BASE_CLASS(Editor::IEditorWindow);
    BASE_CLASS(o2::Singleton<AnimationWindow>);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationWindow)
{
    FIELD().PROTECTED().DEFAULT_VALUE(325.0f).NAME(mTreeViewWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(250.0f).NAME(mMinTreeViewWidth);
    FIELD().PROTECTED().NAME(mTargetActor);
    FIELD().PROTECTED().NAME(mPlayer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOwnPlayer);
    FIELD().PROTECTED().NAME(mAnimation);
    FIELD().PROTECTED().NAME(mAnimationEditable);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDisableTimeTracking);
    FIELD().PROTECTED().NAME(mUpPanel);
    FIELD().PROTECTED().NAME(mWorkArea);
    FIELD().PROTECTED().NAME(mControlsPanel);
    FIELD().PROTECTED().NAME(mPreviewToggle);
    FIELD().PROTECTED().NAME(mRecordToggle);
    FIELD().PROTECTED().NAME(mRewindLeft);
    FIELD().PROTECTED().NAME(mMoveLeft);
    FIELD().PROTECTED().NAME(mPlayPauseToggle);
    FIELD().PROTECTED().NAME(mMoveRight);
    FIELD().PROTECTED().NAME(mRewindRight);
    FIELD().PROTECTED().NAME(mLoopToggle);
    FIELD().PROTECTED().NAME(mCurvesToggle);
    FIELD().PROTECTED().NAME(mAddKeyButton);
    FIELD().PROTECTED().NAME(mPropertiesButton);
    FIELD().PROTECTED().NAME(mTimeline);
    FIELD().PROTECTED().NAME(mTimeScroll);
    FIELD().PROTECTED().NAME(mTree);
    FIELD().PROTECTED().NAME(mHandlesSheet);
    FIELD().PROTECTED().NAME(mCurves);
    FIELD().PROTECTED().NAME(mTreeSeparatorHandle);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<ActionsList>()).NAME(mActionsList);
}
END_META;
CLASS_METHODS_META(Editor::AnimationWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const Ref<AnimationClip>&, Ref<AnimationPlayer>);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimationEditable, const Ref<IEditableAnimation>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurvesMode, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCurvesMode);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosed);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeHandlesSheet);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTree);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTimeline);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeCurvesSheet);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeUpPanel);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeSeparatorHandle);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlayPauseToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLoopToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSearchEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuFilterPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuPreviewToggle, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuRecordToggle, bool);
}
END_META;
// --- END META ---
