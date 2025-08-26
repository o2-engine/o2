#pragma once

#include "o2/Utils/Singleton.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Windows/IAssetEditorWindow.h"
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

    class AnimationWindow : public Singleton<AnimationWindow>, public IAssetEditorWindow
    {
    public:
        // Default constructor
        AnimationWindow(RefCounter* refCounter);

        // Destructor
        ~AnimationWindow();

        // Updates window logic
        void Update(float dt) override;

        // Sets curves or handles mode
        void SetCurvesMode(bool enabled);

        // Returns is curves mode enabled
        bool IsCurvesMode() const;

		// Returns asset type that this editor window can edit
        const Type& GetAssetType() const override;

        // Dynamic cast to RefCounterable via Singleton<AnimationWindow>
        static Ref<RefCounterable> CastToRefCounterable(const Ref<AnimationWindow>& ref);

        IOBJECT(AnimationWindow);
        REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<AnimationWindow>);

    protected:
        float mTreeViewWidth = 325.0f;    // Width of tree area. Changed by draggable separator
        float mMinTreeViewWidth = 325.0f; // Minimal tree width

		WeakRef<Actor>       mTargetActor;              // Target actor on animation
		Ref<AnimationPlayer> mPreviewPlayer;            // Animation player
		bool                 mOwnPreviewPlayer = false; // True if this window owns preview player, otherwise it is from target actor

        WeakRef<AnimationAsset> mAnimationAsset; // Editing animation asset
        WeakRef<AnimationClip>  mAnimation;      // Editing animation

        bool mDisableTimeTracking = false; // When true animation time changes has no effect

        Ref<Widget> mWorkArea; // Working area with tree and time line

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
        void InitializeWindow() override;

		// Called when window has focused, calls focus events in child widgets
		void OnFocused() override;

		// Called when window has unfocused, calls unfocus events in child widgets
		void OnUnfocused() override;

        // Returns window title
        String GetWindowTitle() const override;

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

        // Initializes animation player with current target actor and animation
        void InitializeOwnAnimationPlayer();

		// Initializes external animation player from asset preview interface
		void InitializeExternalAnimationPlayer();

        // Called when asset editing starts
        void OnStartEditingAsset() override;

        // Called when asset editing ends
        void OnCompletedEditingAsset() override;

		// Called when asset editable preview is enabled
		void OnAssetEditablePreviewEnabled() override;

		// Called when asset editable preview is disabled
		void OnAssetEditablePreviewDisabled() override;

		// Returns true if component preview is available for this asset type
        bool IsComponentPreviewAvailable() const override;

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
    BASE_CLASS(o2::Singleton<AnimationWindow>);
    BASE_CLASS(Editor::IAssetEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationWindow)
{
    FIELD().PROTECTED().DEFAULT_VALUE(325.0f).NAME(mTreeViewWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(325.0f).NAME(mMinTreeViewWidth);
    FIELD().PROTECTED().NAME(mTargetActor);
    FIELD().PROTECTED().NAME(mPreviewPlayer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOwnPreviewPlayer);
    FIELD().PROTECTED().NAME(mAnimationAsset);
    FIELD().PROTECTED().NAME(mAnimation);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDisableTimeTracking);
    FIELD().PROTECTED().NAME(mWorkArea);
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
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurvesMode, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCurvesMode);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<AnimationWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosed);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeHandlesSheet);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTree);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTimeline);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeCurvesSheet);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeUpPanel);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeSeparatorHandle);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeOwnAnimationPlayer);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeExternalAnimationPlayer);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetEditablePreviewEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetEditablePreviewDisabled);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsComponentPreviewAvailable);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlayPauseToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLoopToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSearchEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuFilterPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuRecordToggle, bool);
}
END_META;
// --- END META ---
