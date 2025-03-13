#pragma once

#include "o2/Assets/Types/AnimationStateGraphAsset.h"
#include "o2/Scene/Components/AnimationStateGraphComponent.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2Editor/Core/Actions/ActionsList.h"
#include "o2Editor/Core/Actions/IAction.h"
#include "o2Editor/Core/Properties/Objects/DefaultObjectPropertiesViewer.h"
#include "o2Editor/Core/Properties/Basic/VectorProperty.h"
#include "o2Editor/Core/UI/FrameScrollView.h"

using namespace o2;

namespace o2
{
    class ContextMenu;
    class EditBox;
    class Window;
}

namespace Editor
{
    // ---------------------
    // Curves editing widget
    // ---------------------
    class AnimationStateGraphEditor : public FrameScrollView, public SelectableDragHandlesGroup
    {
    public:
        Ref<ActionsList> actionsListDelegate; // Actions fall down list. When it is null, editor uses local actions list

    public:
        // Default constructor
        explicit AnimationStateGraphEditor(RefCounter* refCounter);

        // Destructor
        ~AnimationStateGraphEditor();

        // Sets graph and component
        void SetGraph(const Ref<AnimationStateGraphAsset>& graph,
                      const Ref<AnimationStateGraphComponent>& component);

        // Draws widget, updates render target 
        void Draw() override;

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Sets selection rectangle sprite image
        void SetSelectionSpriteImage(const AssetRef<ImageAsset>& image);

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns context menu
        const Ref<ContextMenu>& GetContextMenu() const;

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<AnimationStateGraphEditor>& ref);

        SERIALIZABLE(AnimationStateGraphEditor);
        CLONEABLE_REF(AnimationStateGraphEditor);

	public:
		struct StateWidget;

        // -----------------------------------------------------------
		// State animation struct. Contains state, animation and owner
		// Used to control state animation
		// -----------------------------------------------------------
        struct StateAnimation : public RefCounterable, public IObject
        {
			String name; // Name of animation

			Ref<IAnimationState>                state;     // Animation state
			Ref<AnimationGraphState::Animation> animation; // Animation
			WeakRef<StateWidget>                owner;     // Owner state widget

            IOBJECT(StateAnimation);
        };

		// --------------------------------------------------------------------
		// State transition struct. Contains owner, destination and drag handle
		// Used to control state transition
		// --------------------------------------------------------------------
        struct StateTransition : public RefCounterable
        {
	        enum class Status { None, Planned, Started };

			WeakRef<StateWidget> owner;       // Owner state widget
			WeakRef<StateWidget> destination; // Destination state widget

			Ref<DragHandle> dragHandle; // Drag handle

        	Status status = Status::None;

        public:
			// Draws transition
            void Draw();

        	// Sets transition animation status
        	void SetStatus(Status status);
        };

		// -----------------------------------------------------------------------------------
		// State widget struct. Contains state, player, animations, transitions and widget
		// Used to control state widget: play, stop, draw transitions, update state and player
		// -----------------------------------------------------------------------------------
		struct StateWidget : public RefCounterable, public IObject
        {
			enum class TransitionState { None, Finished, Planned };

			WeakRef<AnimationGraphState>                       state;  // Animation state reference
			WeakRef<AnimationStateGraphComponent::StatePlayer> player; // State player reference, when state is playing

			Vector<Ref<StateAnimation>>  animations;  // Animations list @DONT_DELETE @DEFAULT_TYPE(StateAnimation)

			Vector<Ref<StateTransition>>                                 transitions;    // Transitions list
			Map<WeakRef<AnimationGraphTransition>, Ref<StateTransition>> transitionsMap; // Transitions map by destination state

			Ref<VerticalLayout>    widget;                 // Widget layout
			Ref<VectorProperty>    animationsListProperty; // Animations list property
			Ref<PropertiesContext> propertiesContext;      // Properties context

			Ref<WidgetLayer> borderLayer; // Border layer

			Ref<DragHandle> dragHandle; // Drag handle

			WeakRef<AnimationStateGraphEditor> editor; // Owner editor reference

        public:
			// Default constructor
			StateWidget() = default;

			// Constructor with ref counter, owner and state
			StateWidget(RefCounter* refCounter, const Ref<AnimationStateGraphEditor>& owner,
						const Ref<AnimationGraphState>& state);

			// Initializes transitions. Used after states list was initialized
			void InitializeTransitions();

			// Removes widget from parent
            void RemoveWidget();

			// Draws transitions
            void DrawTransitions();

			// Updates appearance by state transition state
            void UpdateState(TransitionState state);

			// Sets player, when state is started to play
            void SetPlayer(const Ref<AnimationStateGraphComponent::StatePlayer>& player);

			IOBJECT(StateWidget);
        };

    protected:
        Ref<ContextMenu> mContextMenu; // Context menu for editing keys properties, copying, pasting and other
        
        Ref<ContextMenu> mStateContextMenu; // Common context menu for all states

		WeakRef<AnimationStateGraphAsset>     mGraph;     // Animation state graph asset
		WeakRef<AnimationStateGraphComponent> mComponent; // Animation state graph component

		Vector<Ref<StateWidget>>                            mStatesWidgets;    // States widgets
        Map<WeakRef<AnimationGraphState>, Ref<StateWidget>> mStatesWidgetsMap; // States widgets map by state
        
        WeakRef<StateWidget> mContextMenuStateTarget; // Target state for context menu operations

		Ref<Sprite> mSelectionSprite;       // Selection sprite @SERIALIZABLE
		Vec2F       mSelectingPressedPoint; // Point, where cursor was pressed, selection starts here, in local space

		Ref<CursorAreaEventListenersLayer> mListenersLayer = mmake<CursorAreaEventListenersLayer>(); // Listeners layer

		bool mNeedAdjustView = false; // True when need to adjust view scale. This works in update

        ActionsList mActionsList; // Local actions list. It uses when actionFallDown is null

    protected:
        // Called when visible was changed. Sets context menu items priority
        void OnEnabled() override;

        // Called when visible was changed. Sets context menu items priority
        void OnDisabled() override;

        // Called when scrolling
		void OnScrolled(float scroll) override;

		// Called when cursor pressed on this
		void OnCursorPressed(const Input::Cursor& cursor) override;

		// Called when cursor released (only when cursor pressed this at previous time)
		void OnCursorReleased(const Input::Cursor& cursor) override;

		// Called when cursor stay down during frame
		void OnCursorStillDown(const Input::Cursor& cursor) override;

		// Called when right mouse button stay down on this, overriding from scroll view to call context menu
		void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

		// Called when right mouse button was released (only when right mouse button pressed this at previous time), overriding from scroll view to call context menu
		void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Redraws content into render target
		void RedrawContent() override;

		// Initializes context menu items
		void InitializeContextMenu();
		
		// Initializes common state context menu
		void InitializeStateContextMenu();

        // Recalculates view area by curves approximated points
		void RecalculateViewArea();

        // Draws handles
        void DrawHandles();

        // Draws selection sprite
        void DrawSelection();

        // Draws states transitions
        void DrawTransitions();

        // Initializes states list
        void InitializeStates();

		// Called when state started, updates states transition animation
		void OnStateGraphStateStarted(const Ref<AnimationStateGraphComponent::StatePlayer>& player);

		// Called when state finished, updates states transition animation
		void OnStateGraphStateFinished(const Ref<AnimationStateGraphComponent::StatePlayer>& player);

		// Called when transition started, updates states transition animation
		void OnStateGraphTransitionStarted(const Ref<AnimationGraphTransition>& transition);

		// Called when transition finished, updates states transition animation
		void OnStateGraphTransitionFinished(const Ref<AnimationGraphTransition>& transition);

		// Called when transitions planned, updates states transition animation
		void OnStateGraphTransitionsPlanned(const Vector<Ref<AnimationGraphTransition>>& path);

		// Called when transition cancelled, updates states transition animation
		void OnStateGraphTransitionCancelled(const Ref<AnimationGraphTransition>& transition);
        
        REF_COUNTERABLE_IMPL(FrameScrollView, SelectableDragHandlesGroup);

		friend class GraphAnimationStateViewer;
	};

	// ------------------------------------
	// AnimationComponent properties viewer
	// ------------------------------------
	class GraphAnimationStateViewer : public DefaultObjectPropertiesViewer
	{
	public:
		// Returns viewing objects type
		const Type* GetViewingObjectType() const override;

		// Creates spoiler for properties
		Ref<Spoiler> CreateSpoiler(const Ref<Widget>& parent) override;

		// Returns viewing objects base type by static function
		static const Type* GetViewingObjectTypeStatic();

		IOBJECT(GraphAnimationStateViewer);

	private:
		Ref<Toggle> mPlayPause;
		Ref<Button> mEditBtn;
		Ref<Toggle> mLooped;

		Ref<HorizontalProgress> mTimeProgress;

		WeakRef<IAnimation> mSubscribedPlayer;

	private:
		// Called when viewer is refreshed
		void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;

		// ThCalled when the viewer is freed
		void OnFree() override;

		// Called when play pause toggled
		void OnPlayPauseToggled(bool play);

		// Called when loop toggled
		void OnLoopToggled(bool looped);

		// Called when edit button pressed, sets animation editing
		void OnEditPressed();

		// Called when time progress changed by user, sets subscribed player time 
		void OnTimeProgressChanged(float value);

		// Called when animation updates
		void OnAnimationUpdated(float time);

		// Called when animation started
		void OnAnimationStarted();

		// Called when animation finished
		void OnAnimationFinished();
	};
}
// --- META ---

PRE_ENUM_META(Editor::AnimationStateGraphEditor::StateTransition::Status);

PRE_ENUM_META(Editor::AnimationStateGraphEditor::StateWidget::TransitionState);

CLASS_BASES_META(Editor::AnimationStateGraphEditor)
{
    BASE_CLASS(Editor::FrameScrollView);
    BASE_CLASS(o2::SelectableDragHandlesGroup);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphEditor)
{
    FIELD().PUBLIC().NAME(actionsListDelegate);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().NAME(mStateContextMenu);
    FIELD().PROTECTED().NAME(mGraph);
    FIELD().PROTECTED().NAME(mComponent);
    FIELD().PROTECTED().NAME(mStatesWidgets);
    FIELD().PROTECTED().NAME(mStatesWidgetsMap);
    FIELD().PROTECTED().NAME(mContextMenuStateTarget);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().NAME(mSelectingPressedPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<CursorAreaEventListenersLayer>()).NAME(mListenersLayer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedAdjustView);
    FIELD().PROTECTED().NAME(mActionsList);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphEditor)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGraph, const Ref<AnimationStateGraphAsset>&, const Ref<AnimationStateGraphComponent>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionSpriteImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetContextMenu);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<AnimationStateGraphEditor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeStateContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateViewArea);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawTransitions);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeStates);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphStateStarted, const Ref<AnimationStateGraphComponent::StatePlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphStateFinished, const Ref<AnimationStateGraphComponent::StatePlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionStarted, const Ref<AnimationGraphTransition>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionFinished, const Ref<AnimationGraphTransition>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionsPlanned, const Vector<Ref<AnimationGraphTransition>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionCancelled, const Ref<AnimationGraphTransition>&);
}
END_META;

CLASS_BASES_META(Editor::GraphAnimationStateViewer)
{
    BASE_CLASS(Editor::DefaultObjectPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::GraphAnimationStateViewer)
{
    FIELD().PRIVATE().NAME(mPlayPause);
    FIELD().PRIVATE().NAME(mEditBtn);
    FIELD().PRIVATE().NAME(mLooped);
    FIELD().PRIVATE().NAME(mTimeProgress);
    FIELD().PRIVATE().NAME(mSubscribedPlayer);
}
END_META;
CLASS_METHODS_META(Editor::GraphAnimationStateViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Spoiler>, CreateSpoiler, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PRIVATE().SIGNATURE(void, OnRefreshed, _tmp1);
    FUNCTION().PRIVATE().SIGNATURE(void, OnFree);
    FUNCTION().PRIVATE().SIGNATURE(void, OnPlayPauseToggled, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, OnLoopToggled, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, OnEditPressed);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTimeProgressChanged, float);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAnimationUpdated, float);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAnimationStarted);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAnimationFinished);
}
END_META;

CLASS_BASES_META(Editor::AnimationStateGraphEditor::StateAnimation)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphEditor::StateAnimation)
{
    FIELD().PUBLIC().NAME(name);
    FIELD().PUBLIC().NAME(state);
    FIELD().PUBLIC().NAME(animation);
    FIELD().PUBLIC().NAME(owner);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphEditor::StateAnimation)
{
}
END_META;

CLASS_BASES_META(Editor::AnimationStateGraphEditor::StateWidget)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphEditor::StateWidget)
{
    FIELD().PUBLIC().NAME(state);
    FIELD().PUBLIC().NAME(player);
    FIELD().PUBLIC().DEFAULT_TYPE_ATTRIBUTE(StateAnimation).DONT_DELETE_ATTRIBUTE().NAME(animations);
    FIELD().PUBLIC().NAME(transitions);
    FIELD().PUBLIC().NAME(transitionsMap);
    FIELD().PUBLIC().NAME(widget);
    FIELD().PUBLIC().NAME(animationsListProperty);
    FIELD().PUBLIC().NAME(propertiesContext);
    FIELD().PUBLIC().NAME(borderLayer);
    FIELD().PUBLIC().NAME(dragHandle);
    FIELD().PUBLIC().NAME(editor);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphEditor::StateWidget)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<AnimationStateGraphEditor>&, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, InitializeTransitions);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawTransitions);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateState, TransitionState);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlayer, const Ref<AnimationStateGraphComponent::StatePlayer>&);
}
END_META;
// --- END META ---
