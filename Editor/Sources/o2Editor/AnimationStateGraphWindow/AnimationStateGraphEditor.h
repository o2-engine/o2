#pragma once

#include "o2/Assets/Types/AnimationStateGraphAsset.h"
#include "o2/Scene/Components/AnimationStateGraphComponent.h"
#include "o2/Scene/UI/Widgets/EditBoxDropDown.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2Editor/Core/Actions/ActionsList.h"
#include "o2Editor/Core/Actions/IAction.h"
#include "o2Editor/Core/Properties/Basic/VectorProperty.h"
#include "o2Editor/Core/Properties/Objects/DefaultObjectPropertiesViewer.h"
#include "o2Editor/Core/UI/FrameScrollView.h"
#include "GraphAnimationStateViewer.h"

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

        // Sets selection sprite image
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
			PROPERTIES(StateAnimation);
			PROPERTY(String, name, SetName, GetName);      // Animation name property @ITEMS_SOURCE(GetAvailableStates)
			PROPERTY(float, weight, SetWeight, GetWeight); // Animation weight property @RANGE(0, 1)

		public:
			WeakRef<IAnimationState>                state;     // Animation state
			WeakRef<AnimationGraphState::Animation> animation; // Animation
			WeakRef<StateWidget>                    owner;     // Owner state widget

        public:
			// Returns available states names from AnimationComponent
			Vector<String> GetAvailableStates() const;

			// Sets animation name
			void SetName(const String& name);

			// Returns name
			const String& GetName() const;

			// Sets animation weight
			void SetWeight(float weight);

			// Returns animation weight
			float GetWeight() const;

			IOBJECT(StateAnimation);
        };

		// --------------------------------------------------------------------
		// State transition struct. Contains owner, destination and drag handle
		// Used to control state transition
		// --------------------------------------------------------------------
        struct StateTransition : public RefCounterable, public IObject, public CursorAreaEventsListener
        {
	        enum class TransitionStatus { None, Planned, Started };

			WeakRef<StateWidget> owner;       // Owner state widget
			WeakRef<StateWidget> destination; // Destination state widget

			Ref<AnimationGraphTransition> transition; // Animation transition reference @NO_HEADER

        	TransitionStatus status = TransitionStatus::None; // Current transition planning status @EDITOR_IGNORE
			
			bool mIsSelected = false; // True when transition is selected @EDITOR_IGNORE

        public:
			// Default constructor
			StateTransition();

			// Draws transition
            void Draw();

        	// Sets transition animation status
        	void SetStatus(TransitionStatus status);

			// Sets selected state
			void SetSelected(bool selected);

			// Returns true if transition is selected
			bool IsSelected() const;

			// Returns true if point is in this object
			bool IsUnderPoint(const Vec2F& point) override;

			IOBJECT(StateTransition);
			
		protected:
			// Called when cursor pressed on this
			void OnCursorPressed(const Input::Cursor& cursor) override;

			// Called when right mouse button was released
			void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

			REF_COUNTERABLE_IMPL(RefCounterable);
        };

		// -----------------------------------------------------------------------------------
		// State widget struct. Contains state, player, animations, transitions and widget
		// Used to control state widget: play, stop, draw transitions, update state and player
		// -----------------------------------------------------------------------------------
		struct StateWidget : public RefCounterable, public IObject
        {
			enum class TransitionState { None, Finished, Planned };

			static const Vec2F defaultWidgetSize; // Default size for state widget
			
			WeakRef<AnimationGraphState>                       state;  // Animation state reference
			WeakRef<AnimationStateGraphComponent::StatePlayer> player; // State player reference, when state is playing

			Vector<Ref<StateAnimation>> animations;  // Animations list @DONT_DELETE @DEFAULT_TYPE(StateAnimation) @INVOKE_ON_CHANGE(OnAnimationsListChanged)

			Vector<Ref<StateTransition>>                                 transitions;    // Transitions list
			Map<WeakRef<AnimationGraphTransition>, Ref<StateTransition>> transitionsMap; // Transitions map by destination state

			Ref<VerticalLayout>    widget;                 // Widget layout
			Ref<VectorProperty>    animationsListProperty; // Animations list property
			Ref<PropertiesContext> propertiesContext;      // Properties context

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

			// Opens context menu
			void OpenContextMenu();

			// Called when state was pressed
            void OnPressed();

		protected:
			// Called when animations list was changed, creates or removes animations in original state
            void OnAnimationsListChanged();

			IOBJECT(StateWidget);
        };

    protected:
        Ref<ContextMenu> mContextMenu;           // Context menu for editing keys properties, copying, pasting and other        
        Ref<ContextMenu> mStateContextMenu;      // Common context menu for all states
		Ref<ContextMenu> mTransitionContextMenu; // Common context menu for all transitions

		WeakRef<AnimationStateGraphAsset>     mGraph;     // Animation state graph asset
		WeakRef<AnimationStateGraphComponent> mComponent; // Animation state graph component

		Ref<Widget>                                         mStateWidgetsContainer; // Container for states widgets
		Vector<Ref<StateWidget>>                            mStatesWidgets;         // States widgets
		Map<WeakRef<AnimationGraphState>, Ref<StateWidget>> mStatesWidgetsMap;      // States widgets map by state
		Map<WeakRef<DragHandle>, Ref<StateWidget>>          mStateHandlesMap;       // States widgets map by drag handle

		Ref<Sprite> mSelectionSprite;       // Selection sprite @SERIALIZABLE
		Vec2F       mSelectingPressedPoint; // Point, where cursor was pressed, selection starts here, in local space

		Vec2F            mContextMenuPos;   // Context menu position when right mouse button was pressed
		Ref<StateWidget> mContextMenuState; // State widget, where context menu was opened

		Ref<StateTransition> mSelectedTransition; // Current selected transition

		bool mCreatingTransition = false; // True when creating transition

		bool mNeedAdjustView = false; // True when need to adjust view scale. This works in update

		float mRefreshViewersTimer = 0.0f; // Timer for refreshing viewers

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

		// Overriding from ISceneDrawable to skip drawing children
		void DrawInheritedDepthChildren() override;

        // Redraws content into render target
		void RedrawContent() override;

		// Called when selection is changed - some handle was added or removed from selection; updates selected states in property viewer
    	void OnSelectionChanged() override;

    	// Deselects all in group
    	void DeselectAll() override;

		// Initializes context menu items
		void InitializeContextMenus();

        // Recalculates view area by curves approximated points
		void RecalculateViewArea();

        // Draws handles
        void DrawHandles();

        // Draws selection sprite
        void DrawSelection();

        // Draws states transitions
        void DrawTransitions();

		// Draws single transition
        static void DrawTransition(Vec2F from, Vec2F to, StateTransition::TransitionStatus status, float progress, bool selected = false);

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

		// Creates state widget
		void CreateState();

		// Sets current state default
        void SetCurrentStateInitial();

		// Starts adding transition from current state
		void StartAddingTransition();

		// Removes current selected states
		void RemoveCurrentStates();

		// Removes current transition
        void RemoveCurrentTransition();

		// Opens context menu for state
		void OpenStateContextMenu(const Ref<StateWidget>& state);

		// Called when state was pressed
		void OnStatePressed(const Ref<StateWidget>& state);

		// Checks if viewers need to be refreshed, uses timer to refresh them (mRefreshViewersTimer)
		void CheckRefreshViewersTimer(float dt);
        
        REF_COUNTERABLE_IMPL(FrameScrollView, SelectableDragHandlesGroup);

        friend class AnimationGraphTransitionViewer;
        friend class GraphAnimationStateViewer;
	};
}
// --- META ---

PRE_ENUM_META(Editor::AnimationStateGraphEditor::StateTransition::TransitionStatus);

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
    FIELD().PROTECTED().NAME(mTransitionContextMenu);
    FIELD().PROTECTED().NAME(mGraph);
    FIELD().PROTECTED().NAME(mComponent);
    FIELD().PROTECTED().NAME(mStateWidgetsContainer);
    FIELD().PROTECTED().NAME(mStatesWidgets);
    FIELD().PROTECTED().NAME(mStatesWidgetsMap);
    FIELD().PROTECTED().NAME(mStateHandlesMap);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().NAME(mSelectingPressedPoint);
    FIELD().PROTECTED().NAME(mContextMenuPos);
    FIELD().PROTECTED().NAME(mContextMenuState);
    FIELD().PROTECTED().NAME(mSelectedTransition);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCreatingTransition);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedAdjustView);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mRefreshViewersTimer);
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
    FUNCTION().PROTECTED().SIGNATURE(void, DrawInheritedDepthChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, DeselectAll);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenus);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateViewArea);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawTransitions);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, DrawTransition, Vec2F, Vec2F, StateTransition::TransitionStatus, float, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeStates);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphStateStarted, const Ref<AnimationStateGraphComponent::StatePlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphStateFinished, const Ref<AnimationStateGraphComponent::StatePlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionStarted, const Ref<AnimationGraphTransition>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionFinished, const Ref<AnimationGraphTransition>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionsPlanned, const Vector<Ref<AnimationGraphTransition>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateGraphTransitionCancelled, const Ref<AnimationGraphTransition>&);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateState);
    FUNCTION().PROTECTED().SIGNATURE(void, SetCurrentStateInitial);
    FUNCTION().PROTECTED().SIGNATURE(void, StartAddingTransition);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveCurrentStates);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveCurrentTransition);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenStateContextMenu, const Ref<StateWidget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStatePressed, const Ref<StateWidget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckRefreshViewersTimer, float);
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
    FIELD().PUBLIC().ITEMS_SOURCE_ATTRIBUTE(GetAvailableStates).NAME(name);
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 1).NAME(weight);
    FIELD().PUBLIC().NAME(state);
    FIELD().PUBLIC().NAME(animation);
    FIELD().PUBLIC().NAME(owner);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphEditor::StateAnimation)
{

    FUNCTION().PUBLIC().SIGNATURE(Vector<String>, GetAvailableStates);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWeight);
}
END_META;

CLASS_BASES_META(Editor::AnimationStateGraphEditor::StateTransition)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphEditor::StateTransition)
{
    FIELD().PUBLIC().NAME(owner);
    FIELD().PUBLIC().NAME(destination);
    FIELD().PUBLIC().NO_HEADER_ATTRIBUTE().NAME(transition);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().DEFAULT_VALUE(TransitionStatus::None).NAME(status);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsSelected);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphEditor::StateTransition)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetStatus, TransitionStatus);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelected, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSelected);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
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
    FIELD().PUBLIC().DEFAULT_TYPE_ATTRIBUTE(StateAnimation).DONT_DELETE_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(OnAnimationsListChanged).NAME(animations);
    FIELD().PUBLIC().NAME(transitions);
    FIELD().PUBLIC().NAME(transitionsMap);
    FIELD().PUBLIC().NAME(widget);
    FIELD().PUBLIC().NAME(animationsListProperty);
    FIELD().PUBLIC().NAME(propertiesContext);
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
    FUNCTION().PUBLIC().SIGNATURE(void, OpenContextMenu);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationsListChanged);
}
END_META;
// --- END META ---
