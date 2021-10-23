#pragma once

#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/Math/Curve.h"

namespace o2
{
	class Sprite;
	class Button;
	class TreeNode;

	// -------
	// UI Tree
	// -------
	class Tree: public ScrollArea, public DragDropArea, public ISelectableDragableObjectsGroup
	{
	public:
		// Tree nodes rearrange available types
		enum class RearrangeType { Disabled, Enabled, OnlyReparent };

		// Tree node expand states
		enum class ExpandState { None, Expanding, Collaping };

	public:
		Function<void*(void*)>           getObjectParentDelegate;      // Getting objects' parent delegate
		Function<Vector<void*>(void*)>   getObjectChildrenDelegate;    // Getting objects' childs count delegate 
		Function<void(TreeNode*, void*)> fillNodeDataByObjectDelegate; // Setup tree node item delegate
		Function<void(TreeNode*, void*)> freeNodeDataDelegate;         // Free tree node data delegate

		Function<String(void*)> getDebugForObject; // Getting debug string for object delegate

		Function<void(TreeNode*)> onNodeDoubleClicked;      // Node double clicked event
		Function<void(TreeNode*)> onNodeRightButtonClicked; // Node right button click event

		Function<void(Vector<void*>)> onObjectsSelectionChanged; // Objects selected event

		Function<void(Vector<void*>, void*, void*)> onDraggedObjects; // Objects dragged event

	public:
		// Default constructor
		Tree();

		// Copy-constructor
		Tree(const Tree& other);

		// Destructor
		~Tree();

		// Copy-operator
		Tree& operator=(const Tree& other);

		// Creates tree node for object
		void OnObjectCreated(void* object, void* parent);

		// Removes tree node for object
		void OnObjectRemoved(void* object);

		// Updates tree for changed objects
		void OnObjectsChanged(const Vector<void*>& objects);

		// Draws widget
		void Draw() override;

		// Updates widget
		void Update(float dt) override;

		// Updates childs
		void UpdateChildren(float dt) override;

		// Updates children and internal children transforms
		void UpdateChildrenTransforms() override;

		// Rebuilds all tree
		void UpdateNodesView(bool immediately = true);

		// Returns ui node for object
		TreeNode* GetNode(void* object);

		// Expands all nodes
		void ExpandAll();

		// Collapses all nodes
		void CollapseAll();

		// Returns selected objects vector
		Vector<void*> GetSelectedObjects() const;

		// Sets selected objects
		void SetSelectedObjects(const Vector<void*>& objects);

		// Selects object
		void SelectObject(void* object);

		// Selects object
		void SelectAndHighlightObject(void* object);

		// Deselects object
		void DeselectObject(void* object);

		// Deselects all objects
		void DeselectAllObjects();

		// Scrolls view to object
		void ScrollTo(void* object);

		// Scrolls view to object and highlights
		void ScrollToAndHighlight(void* object);

		// Expands all parent objects for specified object
		void ExpandParentObjects(void* object);

		// Returns item widget under point
		TreeNode* GetTreeNodeUnderPoint(const Vec2F& point);

		// Sets rearrange type
		void SetRearrangeType(RearrangeType type);

		// Returns available rearrange type 
		RearrangeType GetRearrangeType() const;

		// Sets available multi selection
		void SetMultipleSelectionAvailable(bool available);

		// Returns is multi selection available
		bool IsMultiSelectionAvailable() const;

		// Returns item sample
		TreeNode* GetNodeSample() const;

		// Sets item sample
		void SetNodeSample(TreeNode* sample);

		// Returns hover drawable
		Sprite* GetHoverDrawable() const;

		// Sets hover layout
		void SetHoverLayout(const Layout& layout);

		// Returns node highlight drawable
		Sprite* GetHighlightDrawable() const;

		// Sets highlight animation
		void SetHighlightAnimation(const AnimationClip& animation);

		// Sets highlight layout
		void SetHighlightLayout(const Layout& layout);

		// Sets zebra back line sprite drawable. When it is null no zebra back isn't drawing
		void SetZebraBackLine(Sprite* sprite);

		// Return zebra back line sprite
		Sprite* GetZebraBackLine() const;

		// Sets node pressing and expanding time
		void SetNodeExpandTimer(float time);

		// Returns node pressing and expanding time
		float GetNodeExpandTimer() const;

		// Sets children nodes horizontal offset from parent
		void SetChildsNodesOffset(float offset);

		// Returns childs nodes horizontal offset
		float GetChildsNodesOffset() const;

		// Returns is listener scrollable
		bool IsScrollable() const override;

		// Returns is this widget can be selected
		bool IsFocusable() const override;

		// Returns true if point is in this object
		bool IsUnderPoint(const Vec2F& point) override;

		// Updates layout
		void UpdateSelfTransform() override;

		// Returns true when input events can be handled by down listeners
		bool IsInputTransparent() const override;

		// Returns create menu group in editor
		static String GetCreateMenuGroup();

		SERIALIZABLE(Tree);

	protected:
		struct Node;

		// --------------------
		// Tree node definition
		// --------------------
		struct Node
		{
			String     id;
			void*      object;             // Pointer to object
			TreeNode*  widget = nullptr;   // Node widget
			int        level = 0;          // Hierarchy depth level
			bool       isSelected = false; // Is node selected
			bool       isExpanded = false; // Is node expanded

			Node*         parent = nullptr; // Parent node definition
			Vector<Node*> childs;           // Children nodes definitions

			bool  inserting = false; // Node insertion flag
			float insertCoef = 0.0f; // Inserting coefficient (0...1)

		public:
			// Returns height of node with children and coefficients
			float GetHeight() const;

			// Sets widget state to selected and stores selected flag
			void SetSelected(bool selected);

			// Returns all children count
			int GetChildCount() const;
		};

	protected:
		// -----------------------------------
		// Visible tree node widget cache info
		// -----------------------------------
		struct VisibleWidgetDef
		{
			void*     object;
			TreeNode* widget;
			int       position;

		public:

			bool operator==(const VisibleWidgetDef& other) const;
		};

	protected:
		RearrangeType mRearrangeType = RearrangeType::Enabled; // Current available rearrange type @SERIALIZABLE
		bool          mMultiSelectAvailable = true;            // Is multi selection available @SERIALIZABLE

		TreeNode* mNodeWidgetSample = nullptr; // Item sample @SERIALIZABLE
		float     mChildrenOffset = 10.0f;     // Children nodes offset from parent @SERIALIZABLE

		bool mIsNeedUpdateView = false;         // Is tree needs to be rebuild
		bool mIsNeedUdateLayout = false;        // Is layout needs to rebuild
		bool mIsNeedUpdateVisibleNodes = false; // In need to update visible nodes

		Vector<Node*> mAllNodes; // All expanded nodes definitions

		Vector<void*> mSelectedObjects; // Selected objects
		Vector<Node*> mSelectedNodes;   // Selected nodes definitions

		Vector<TreeNode*> mNodeWidgetsBuf; // Nodes widgets buffer
		Vector<Node*>     mNodesBuf;       // Nodes buffer

		Vector<Node*> mVisibleNodes;           // Visible nodes
		int           mMinVisibleNodeIdx = 0;  // Minimal visible node index
		int           mMaxVisibleNodeIdx = -1; // Maximum visible node index

		Vec2F mLastClickPos; // Last click position in scroll space (depends on scroll position)

		TreeNode* mHoveredItem = nullptr;   // Current hovered tree node item
		Sprite*   mHoverDrawable = nullptr; // Selection sprite @SERIALIZABLE
		Layout    mHoverLayout;             // Selection layout, result selection area depends on selected item @SERIALIZABLE						  										   
		RectF     mCurrentHoverRect;        // Current selection rectangle (for smoothing)
		RectF     mTargetHoverRect;         // Target selection rectangle (over selected item)	

		bool          mIsDraggingNodes = false;       // Is nodes moving by cursor
		TreeNode*     mFakeDragNode = nullptr;        // Dragging node
		Vec2F         mDragOffset;                    // Offset from cursor to dragging node's center
		TreeNode*     mInsertNodeCandidate = nullptr; // Insertion node candidate when dragging nodes
		Vector<void*> mBeforeDragSelectedItems;       // Before drag begin selection
		bool          mDragEnded = false;             // Is dragging ended and it needs to call EndDragging

		Vector<void*> mExpandedObjects; // Expanded objects

		ExpandState mExpandingNodeState = ExpandState::None; // Expanding node state
		int         mExpandingNodeIdx = -1;                  // Current expanding node index. -1 if no expanding node
		int         mExpandingNodeChildsCount = 0;           // Current expanding node children count
		float       mExpandingNodePosition = 0.0f;           // Current expanding node position in this local coordinates
		float       mExpandingNodeBottomPosition = 0.0f;     // Current expanding node position of bottom
		float       mExpandingNodeCurrCoef = 0.0f;           // Current expanding node coefficient of expanding
		float       mExpandingNodeCurrHeight = 0.0f;         // Current expanding node current height
		float       mExpandingNodeTargetHeight = 0.0f;       // Current expanding node target height
		float       mExpandNodeTime = 0.4f;                  // Node expanding time
		Curve       mExpandingNodeFunc = Curve::EaseInOut(); // Expanding easing node curve

		TreeNode* mExpandNodeCandidate = nullptr; // Expand node candidate when dragging
		float     mExpandInsertTime = -1.0f;      // Remaining time to expanding item under cursor when dragging nodes
		float     mPressedTime = 10.0f;           // Time from last item pressing

		float mNodeExpandTime = 2.0f;   // Node expand time when dragging actors @SERIALIZABLE
		float mNodeDragIntoZone = 0.3f; // Node inside dragging zone coefficient (0.5 is full node area) @SERIALIZABLE

		AnimationClip   mHighlighClip;              // Node highlight animation clip @SERIALIZABLE 
		AnimationPlayer mHighlightAnim;             // Node highlight animation
		Sprite*         mHighlightSprite = nullptr; // Node highlight sprite @SERIALIZABLE
		Layout          mHighlightLayout;           // Node highlight sprite layout @SERIALIZABLE
		Node*           mHighlighNode = nullptr;    // Hightlighing node
		void*           mHighlightObject;           // Highlight object
		
		Sprite* mZebraBackLine = nullptr; // Dark zebra line sprite. When it is null, no zebra back doesn't draw @SERIALIZABLE

		Vector<VisibleWidgetDef> mVisibleWidgetsCache; // Visible widgets cache

	protected:
		// It is called when widget was selected
		void OnFocused() override;

		// It is called when widget was deselected
		void OnUnfocused() override;

		// Draws zebra back with scroll offset
		void DrawZebraBack();

		// Returns object's parent
		virtual void* GetObjectParent(void* object);

		// Returns object's children
		virtual Vector<void*> GetObjectChilds(void* object);

		// Returns debugging string for object
		virtual String GetObjectDebug(void* object);

		// Sets nodeWidget data by object
		virtual void FillNodeDataByObject(TreeNode* nodeWidget, void* object);

		// Free node data
		virtual void FreeNodeData(TreeNode* nodeWidget, void* object);

		// It is called when tree node was double clicked
		virtual void OnNodeDblClick(TreeNode* nodeWidget);

		// It is called when tree node was clicked by right button
		virtual void OnNodeRBClick(TreeNode* nodeWidget);

		// It is called when list of selected objects was changed
		virtual void OnNodesSelectionChanged(Vector<void*> objects);

		// It is called when objects was dragged in new parent in position next of prevObject
		virtual void OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject);

// ISelectableDragableObjectsGroup implementation

		// Returns selected objects in group
		Vector<SelectableDragableObject*> GetSelectedDragObjects() const override;

		// Returns all objects in group 
		Vector<SelectableDragableObject*> GetAllObjects() const override;

		// Selects object
		void Select(SelectableDragableObject* object) override;

		// Selects object
		void Select(SelectableDragableObject* object, bool sendOnSelectionChanged);

		// Deselects object
		void Deselect(SelectableDragableObject* object) override;

		// Adds selectable object to group
		void AddSelectableObject(SelectableDragableObject* object) override;

		// Removes selectable object from group
		void RemoveSelectableObject(SelectableDragableObject* object) override;

		// It is called when selectable draggable object was released
		void OnSelectableObjectCursorReleased(SelectableDragableObject* object, const Input::Cursor& cursor) override;

		// It is called when selectable object was began to drag
		void OnSelectableObjectBeganDragging(SelectableDragableObject* object) override;

// -------------

		// Checks multiple selection nodes (when Shift key is down) and returns is someone was selected
		bool CheckMultipleSelection(const Vec2F& point);

		// It is called when items selected
		void OnSelectionChanged();

		// Updates highlight animation
		void UpdateHighlighting(float dt);

		// Checks and updates pressed node expanding
		void UpdatePressedNodeExpand(float dt);

		// Updates root nodes and their childs if need
		virtual void UpdateNodesStructure();

		// Inserts node to hierarchy
		int InsertNodes(Node* parentNode, int position, Vector<Node*>* newNodes = nullptr);

		// Removes node from hierarchy
		void RemoveNodes(Node* parentNode);

		// Creates node from object with parent
		Node* CreateNode(void* object, Node* parent);

		// Updates visible nodes (calculates range and initializes nodes)
		virtual void UpdateVisibleNodes();

		// Createw UITreeNode for visible node at position i
		void CreateVisibleNodeWidget(Node* node, int i);

		// Updates node view
		void UpdateNodeView(Node* node, TreeNode* widget, int idx);

		// Updates node widget layout
		void UpdateNodeWidgetLayout(Node* node, int idx);

		// Returns node index from mAllNodes by position in local coordinates
		int GetNodeIndex(float position) const;

		// Returns node position in local coordinates by index in mAllNodes array
		float GetNodePosition(int idx) const;

		// Expands node: creates his children and sets expanded state
		void ExpandNode(Node* node);

		// Collapses node: removes children from hierarchy and widgets if they're visible
		void CollapseNode(Node* node);

		// Starts expanding or collapse animation of node
		void StartExpandingAnimation(ExpandState direction, Node* node, int childrenCount);

		// Updates node expanding
		void UpdateNodeExpanding(float dt);

		// Calculates scroll area
		void CalculateScrollArea() override;

		// Moves scroll position and updates children widgets clipping and layout
		void MoveScrollPosition(const Vec2F& delta) override;

		// It is called when cursor pressed on this
		void OnCursorPressed(const Input::Cursor& cursor) override;

		// It is called when cursor moved on this (or moved outside when this was pressed)
		void OnCursorMoved(const Input::Cursor& cursor) override;

		// It is called when cursor released (only when cursor pressed this at previous time)
		void OnCursorReleased(const Input::Cursor& cursor) override;

		// It is called when right mouse button was released (only when right mouse button pressed this at previous time)
		void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

		// It is called when cursor pressing was broken (when scrolled scroll area or some other)
		void OnCursorPressBreak(const Input::Cursor& cursor) override;

		// It is called when cursor exits this object
		void OnCursorExit(const Input::Cursor& cursor) override;

		// It is called when scrolling
		void OnScrolled(float scroll) override;

		// Updates hover target rect and visibility
		void UpdateHover(TreeNode* itemUnderCursor);

		// Gets tree node from pool or creates new
		virtual TreeNode* CreateTreeNodeWidget();

// DragDropArea implementation

		// Begins dragging selected items
		void BeginDragging(TreeNode* node);

		// Ends dragging items
		void EndDragging(bool droppedToThis = false);

		// Updates dragging graphics
		void UpdateDraggingGraphics();

		// Updates insert states on nodes when dragging
		void UpdateDraggingInsertion();

		// Updates insertion animation when dragging
		void UpdateDraggingInsertionAnim(float dt);

		// It is called when some drag listeners was entered to this area
		void OnDragEnter(ISelectableDragableObjectsGroup* group) override;

		// It is called when some drag listeners was dragged above this area
		void OnDraggedAbove(ISelectableDragableObjectsGroup* group) override;

		// It is called when some drag listeners was exited from this area
		void OnDragExit(ISelectableDragableObjectsGroup* group) override;

		// It is called when some selectable listeners was dropped to this
		void OnDropped(ISelectableDragableObjectsGroup* group) override;

// ------------

		// Completion deserialization callback
		void OnDeserialized(const DataValue& node) override;

		friend class TreeNode;
	};

	// ------------
	// UI Tree node
	// ------------
	class TreeNode: public Widget, public SelectableDragableObject
	{
	public:
		// Default constructor
		TreeNode();

		// Copy-constructor
		TreeNode(const TreeNode& other);

		// Destructor
		~TreeNode();

		// Copy-operator
		TreeNode& operator=(const TreeNode& other);

		// Sets expanding
		void SetExpanded(bool expanded, bool forcible = false);

		// Is node expanded
		bool IsExpanded() const;

		// Expands node
		void Expand(bool forcible = false);

		// Collapses node
		void Collapse(bool forcible = false);

		// Returns object pointer
		void* GetObject() const;

		// Returns true if point is in this object
		bool IsUnderPoint(const Vec2F& point) override;

		// Sets selected state
		void SetSelectedState(bool state);

		// Sets focused state
		void SetFocusedState(bool state);

		// Returns create menu group in editor
		static String GetCreateMenuGroup();

		SERIALIZABLE(TreeNode);

	protected:
		Tree::Node* mNodeDef = nullptr;   // Node definition
		Tree*       mOwnerTree = nullptr; // Owner tree
		Button*     mExpandBtn = nullptr; // Node expanding button

		WidgetState* mSelectedState = nullptr; // Selected state cached

	protected:
		// It is called when widget state was added, caches selected state
		void OnStateAdded(WidgetState* state) override;

		// Updates expanding
		void UpdateTreeLayout(float dt);

		// It is called when cursor double clicked
		void OnCursorDblClicked(const Input::Cursor& cursor) override;

		// It is called when cursor enters this object, moving hover of tree to this
		void OnCursorEnter(const Input::Cursor& cursor) override;

		// It is called when cursor exits this object, moving hover of tree to this
		void OnCursorExit(const Input::Cursor& cursor) override;

		// It is called when started dragging
		void OnDragStart(const Input::Cursor& cursor) override;

		// It is called when dragged
		void OnDragged(const Input::Cursor& cursor, DragDropArea* area) override;

		// It is called when dragging completed
		void OnDragEnd(const Input::Cursor& cursor) override;

		// It is called when this was selected
		void OnSelected() override;

		// It is called when this was unselected
		void OnDeselected() override;

		friend class Tree;
	};
}

PRE_ENUM_META(o2::Tree::RearrangeType);

PRE_ENUM_META(o2::Tree::ExpandState);

CLASS_BASES_META(o2::Tree)
{
	BASE_CLASS(o2::ScrollArea);
	BASE_CLASS(o2::DragDropArea);
	BASE_CLASS(o2::ISelectableDragableObjectsGroup);
}
END_META;
CLASS_FIELDS_META(o2::Tree)
{
	FIELD().NAME(getObjectParentDelegate).PUBLIC();
	FIELD().NAME(getObjectChildrenDelegate).PUBLIC();
	FIELD().NAME(fillNodeDataByObjectDelegate).PUBLIC();
	FIELD().NAME(freeNodeDataDelegate).PUBLIC();
	FIELD().NAME(getDebugForObject).PUBLIC();
	FIELD().NAME(onNodeDoubleClicked).PUBLIC();
	FIELD().NAME(onNodeRightButtonClicked).PUBLIC();
	FIELD().NAME(onObjectsSelectionChanged).PUBLIC();
	FIELD().NAME(onDraggedObjects).PUBLIC();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(RearrangeType::Enabled).NAME(mRearrangeType).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMultiSelectAvailable).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(mNodeWidgetSample).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10.0f).NAME(mChildrenOffset).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mIsNeedUpdateView).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mIsNeedUdateLayout).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mIsNeedUpdateVisibleNodes).PROTECTED();
	FIELD().NAME(mAllNodes).PROTECTED();
	FIELD().NAME(mSelectedObjects).PROTECTED();
	FIELD().NAME(mSelectedNodes).PROTECTED();
	FIELD().NAME(mNodeWidgetsBuf).PROTECTED();
	FIELD().NAME(mNodesBuf).PROTECTED();
	FIELD().NAME(mVisibleNodes).PROTECTED();
	FIELD().DEFAULT_VALUE(0).NAME(mMinVisibleNodeIdx).PROTECTED();
	FIELD().DEFAULT_VALUE(-1).NAME(mMaxVisibleNodeIdx).PROTECTED();
	FIELD().NAME(mLastClickPos).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mHoveredItem).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(mHoverDrawable).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().NAME(mHoverLayout).PROTECTED();
	FIELD().NAME(mCurrentHoverRect).PROTECTED();
	FIELD().NAME(mTargetHoverRect).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mIsDraggingNodes).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mFakeDragNode).PROTECTED();
	FIELD().NAME(mDragOffset).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mInsertNodeCandidate).PROTECTED();
	FIELD().NAME(mBeforeDragSelectedItems).PROTECTED();
	FIELD().DEFAULT_VALUE(false).NAME(mDragEnded).PROTECTED();
	FIELD().NAME(mExpandedObjects).PROTECTED();
	FIELD().DEFAULT_VALUE(ExpandState::None).NAME(mExpandingNodeState).PROTECTED();
	FIELD().DEFAULT_VALUE(-1).NAME(mExpandingNodeIdx).PROTECTED();
	FIELD().DEFAULT_VALUE(0).NAME(mExpandingNodeChildsCount).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandingNodePosition).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeBottomPosition).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeCurrCoef).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeCurrHeight).PROTECTED();
	FIELD().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeTargetHeight).PROTECTED();
	FIELD().DEFAULT_VALUE(0.4f).NAME(mExpandNodeTime).PROTECTED();
	FIELD().DEFAULT_VALUE(Curve::EaseInOut()).NAME(mExpandingNodeFunc).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mExpandNodeCandidate).PROTECTED();
	FIELD().DEFAULT_VALUE(-1.0f).NAME(mExpandInsertTime).PROTECTED();
	FIELD().DEFAULT_VALUE(10.0f).NAME(mPressedTime).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(2.0f).NAME(mNodeExpandTime).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.3f).NAME(mNodeDragIntoZone).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().NAME(mHighlighClip).PROTECTED();
	FIELD().NAME(mHighlightAnim).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(mHighlightSprite).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightLayout).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mHighlighNode).PROTECTED();
	FIELD().NAME(mHighlightObject).PROTECTED();
	FIELD().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(mZebraBackLine).PROTECTED();
	FIELD().NAME(mVisibleWidgetsCache).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::Tree)
{

	PUBLIC_FUNCTION(void, OnObjectCreated, void*, void*);
	PUBLIC_FUNCTION(void, OnObjectRemoved, void*);
	PUBLIC_FUNCTION(void, OnObjectsChanged, const Vector<void*>&);
	PUBLIC_FUNCTION(void, Draw);
	PUBLIC_FUNCTION(void, Update, float);
	PUBLIC_FUNCTION(void, UpdateChildren, float);
	PUBLIC_FUNCTION(void, UpdateChildrenTransforms);
	PUBLIC_FUNCTION(void, UpdateNodesView, bool);
	PUBLIC_FUNCTION(TreeNode*, GetNode, void*);
	PUBLIC_FUNCTION(void, ExpandAll);
	PUBLIC_FUNCTION(void, CollapseAll);
	PUBLIC_FUNCTION(Vector<void*>, GetSelectedObjects);
	PUBLIC_FUNCTION(void, SetSelectedObjects, const Vector<void*>&);
	PUBLIC_FUNCTION(void, SelectObject, void*);
	PUBLIC_FUNCTION(void, SelectAndHighlightObject, void*);
	PUBLIC_FUNCTION(void, DeselectObject, void*);
	PUBLIC_FUNCTION(void, DeselectAllObjects);
	PUBLIC_FUNCTION(void, ScrollTo, void*);
	PUBLIC_FUNCTION(void, ScrollToAndHighlight, void*);
	PUBLIC_FUNCTION(void, ExpandParentObjects, void*);
	PUBLIC_FUNCTION(TreeNode*, GetTreeNodeUnderPoint, const Vec2F&);
	PUBLIC_FUNCTION(void, SetRearrangeType, RearrangeType);
	PUBLIC_FUNCTION(RearrangeType, GetRearrangeType);
	PUBLIC_FUNCTION(void, SetMultipleSelectionAvailable, bool);
	PUBLIC_FUNCTION(bool, IsMultiSelectionAvailable);
	PUBLIC_FUNCTION(TreeNode*, GetNodeSample);
	PUBLIC_FUNCTION(void, SetNodeSample, TreeNode*);
	PUBLIC_FUNCTION(Sprite*, GetHoverDrawable);
	PUBLIC_FUNCTION(void, SetHoverLayout, const Layout&);
	PUBLIC_FUNCTION(Sprite*, GetHighlightDrawable);
	PUBLIC_FUNCTION(void, SetHighlightAnimation, const AnimationClip&);
	PUBLIC_FUNCTION(void, SetHighlightLayout, const Layout&);
	PUBLIC_FUNCTION(void, SetZebraBackLine, Sprite*);
	PUBLIC_FUNCTION(Sprite*, GetZebraBackLine);
	PUBLIC_FUNCTION(void, SetNodeExpandTimer, float);
	PUBLIC_FUNCTION(float, GetNodeExpandTimer);
	PUBLIC_FUNCTION(void, SetChildsNodesOffset, float);
	PUBLIC_FUNCTION(float, GetChildsNodesOffset);
	PUBLIC_FUNCTION(bool, IsScrollable);
	PUBLIC_FUNCTION(bool, IsFocusable);
	PUBLIC_FUNCTION(bool, IsUnderPoint, const Vec2F&);
	PUBLIC_FUNCTION(void, UpdateSelfTransform);
	PUBLIC_FUNCTION(bool, IsInputTransparent);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuGroup);
	PROTECTED_FUNCTION(void, OnFocused);
	PROTECTED_FUNCTION(void, OnUnfocused);
	PROTECTED_FUNCTION(void, DrawZebraBack);
	PROTECTED_FUNCTION(void*, GetObjectParent, void*);
	PROTECTED_FUNCTION(Vector<void*>, GetObjectChilds, void*);
	PROTECTED_FUNCTION(String, GetObjectDebug, void*);
	PROTECTED_FUNCTION(void, FillNodeDataByObject, TreeNode*, void*);
	PROTECTED_FUNCTION(void, FreeNodeData, TreeNode*, void*);
	PROTECTED_FUNCTION(void, OnNodeDblClick, TreeNode*);
	PROTECTED_FUNCTION(void, OnNodeRBClick, TreeNode*);
	PROTECTED_FUNCTION(void, OnNodesSelectionChanged, Vector<void*>);
	PROTECTED_FUNCTION(void, OnDraggedObjects, Vector<void*>, void*, void*);
	PROTECTED_FUNCTION(Vector<SelectableDragableObject*>, GetSelectedDragObjects);
	PROTECTED_FUNCTION(Vector<SelectableDragableObject*>, GetAllObjects);
	PROTECTED_FUNCTION(void, Select, SelectableDragableObject*);
	PROTECTED_FUNCTION(void, Select, SelectableDragableObject*, bool);
	PROTECTED_FUNCTION(void, Deselect, SelectableDragableObject*);
	PROTECTED_FUNCTION(void, AddSelectableObject, SelectableDragableObject*);
	PROTECTED_FUNCTION(void, RemoveSelectableObject, SelectableDragableObject*);
	PROTECTED_FUNCTION(void, OnSelectableObjectCursorReleased, SelectableDragableObject*, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnSelectableObjectBeganDragging, SelectableDragableObject*);
	PROTECTED_FUNCTION(bool, CheckMultipleSelection, const Vec2F&);
	PROTECTED_FUNCTION(void, OnSelectionChanged);
	PROTECTED_FUNCTION(void, UpdateHighlighting, float);
	PROTECTED_FUNCTION(void, UpdatePressedNodeExpand, float);
	PROTECTED_FUNCTION(void, UpdateNodesStructure);
	PROTECTED_FUNCTION(int, InsertNodes, Node*, int, Vector<Node*>*);
	PROTECTED_FUNCTION(void, RemoveNodes, Node*);
	PROTECTED_FUNCTION(Node*, CreateNode, void*, Node*);
	PROTECTED_FUNCTION(void, UpdateVisibleNodes);
	PROTECTED_FUNCTION(void, CreateVisibleNodeWidget, Node*, int);
	PROTECTED_FUNCTION(void, UpdateNodeView, Node*, TreeNode*, int);
	PROTECTED_FUNCTION(void, UpdateNodeWidgetLayout, Node*, int);
	PROTECTED_FUNCTION(int, GetNodeIndex, float);
	PROTECTED_FUNCTION(float, GetNodePosition, int);
	PROTECTED_FUNCTION(void, ExpandNode, Node*);
	PROTECTED_FUNCTION(void, CollapseNode, Node*);
	PROTECTED_FUNCTION(void, StartExpandingAnimation, ExpandState, Node*, int);
	PROTECTED_FUNCTION(void, UpdateNodeExpanding, float);
	PROTECTED_FUNCTION(void, CalculateScrollArea);
	PROTECTED_FUNCTION(void, MoveScrollPosition, const Vec2F&);
	PROTECTED_FUNCTION(void, OnCursorPressed, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorMoved, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorReleased, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorRightMouseReleased, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorPressBreak, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorExit, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnScrolled, float);
	PROTECTED_FUNCTION(void, UpdateHover, TreeNode*);
	PROTECTED_FUNCTION(TreeNode*, CreateTreeNodeWidget);
	PROTECTED_FUNCTION(void, BeginDragging, TreeNode*);
	PROTECTED_FUNCTION(void, EndDragging, bool);
	PROTECTED_FUNCTION(void, UpdateDraggingGraphics);
	PROTECTED_FUNCTION(void, UpdateDraggingInsertion);
	PROTECTED_FUNCTION(void, UpdateDraggingInsertionAnim, float);
	PROTECTED_FUNCTION(void, OnDragEnter, ISelectableDragableObjectsGroup*);
	PROTECTED_FUNCTION(void, OnDraggedAbove, ISelectableDragableObjectsGroup*);
	PROTECTED_FUNCTION(void, OnDragExit, ISelectableDragableObjectsGroup*);
	PROTECTED_FUNCTION(void, OnDropped, ISelectableDragableObjectsGroup*);
	PROTECTED_FUNCTION(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::TreeNode)
{
	BASE_CLASS(o2::Widget);
	BASE_CLASS(o2::SelectableDragableObject);
}
END_META;
CLASS_FIELDS_META(o2::TreeNode)
{
	FIELD().DEFAULT_VALUE(nullptr).NAME(mNodeDef).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mOwnerTree).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mExpandBtn).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mSelectedState).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::TreeNode)
{

	PUBLIC_FUNCTION(void, SetExpanded, bool, bool);
	PUBLIC_FUNCTION(bool, IsExpanded);
	PUBLIC_FUNCTION(void, Expand, bool);
	PUBLIC_FUNCTION(void, Collapse, bool);
	PUBLIC_FUNCTION(void*, GetObject);
	PUBLIC_FUNCTION(bool, IsUnderPoint, const Vec2F&);
	PUBLIC_FUNCTION(void, SetSelectedState, bool);
	PUBLIC_FUNCTION(void, SetFocusedState, bool);
	PUBLIC_STATIC_FUNCTION(String, GetCreateMenuGroup);
	PROTECTED_FUNCTION(void, OnStateAdded, WidgetState*);
	PROTECTED_FUNCTION(void, UpdateTreeLayout, float);
	PROTECTED_FUNCTION(void, OnCursorDblClicked, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorEnter, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnCursorExit, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnDragStart, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnDragged, const Input::Cursor&, DragDropArea*);
	PROTECTED_FUNCTION(void, OnDragEnd, const Input::Cursor&);
	PROTECTED_FUNCTION(void, OnSelected);
	PROTECTED_FUNCTION(void, OnDeselected);
}
END_META;
