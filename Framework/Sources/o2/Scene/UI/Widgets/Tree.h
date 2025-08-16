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
        Function<void*(void*)>         getObjectParentDelegate;      // Getting objects' parent delegate
        Function<Vector<void*>(void*)> getObjectChildrenDelegate;    // Getting objects' childs count delegate

        Function<void(const Ref<TreeNode>&, void*)> fillNodeDataByObjectDelegate; // Setup tree node item delegate
        Function<void(const Ref<TreeNode>&, void*)> freeNodeDataDelegate;         // Free tree node data delegate

        Function<String(void*)> getDebugForObject; // Getting debug string for object delegate

        Function<void(const Ref<TreeNode>&)> onNodeDoubleClicked;      // Node double clicked event
        Function<void(const Ref<TreeNode>&)> onNodeRightButtonClicked; // Node right button click event

        Function<void(Vector<void*>)> onObjectsSelectionChanged; // Objects selected event

        Function<void(Vector<void*>, void*, void*)> onDraggedObjects; // Objects dragged event

        Function<void(void*)> onNodeExpanded;  // Node expanded event
        Function<void(void*)> onNodeCollapsed; // Node collapsed event

    public:
        // Default constructor
        explicit Tree(RefCounter* refCounter);

        // Copy-constructor
        Tree(RefCounter* refCounter, const Tree& other);

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
        Ref<TreeNode> GetNode(void* object);

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
        Ref<TreeNode> GetTreeNodeUnderPoint(const Vec2F& point);

        // Sets rearrange type
        void SetRearrangeType(RearrangeType type);

        // Returns available rearrange type 
        RearrangeType GetRearrangeType() const;

        // Sets available multi selection
        void SetMultipleSelectionAvailable(bool available);

        // Returns is multi selection available
        bool IsMultiSelectionAvailable() const;

        // Returns item sample
        const Ref<TreeNode>& GetNodeSample() const;

        // Sets item sample
        void SetNodeSample(const Ref<TreeNode>& sample);

        // Returns hover drawable
        const Ref<Sprite>& GetHoverDrawable() const;

        // Sets hover layout
        void SetHoverLayout(const Layout& layout);

        // Returns node highlight drawable
        const Ref<Sprite>& GetHighlightDrawable() const;

        // Sets highlight animation
        void SetHighlightAnimation(const Ref<AnimationClip>& animation);

        // Sets highlight layout
        void SetHighlightLayout(const Layout& layout);

        // Sets zebra back line sprite drawable. When it is null no zebra back isn't drawing
        void SetZebraBackLine(const Ref<Sprite>& sprite);

        // Return zebra back line sprite
        const Ref<Sprite>& GetZebraBackLine() const;

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
        CLONEABLE_REF(Tree);

    protected:
        // --------------------
        // Tree node definition
        // --------------------
        struct Node: public RefCounterable
        {
            String         id;
            void*          object;             // Pointer to object
            Ref<TreeNode>  widget;             // Node widget
            int            level = 0;          // Hierarchy depth level
            bool           isSelected = false; // Is node selected
            bool           isExpanded = false; // Is node expanded

            WeakRef<Node>     parent; // Parent node definition
            Vector<Ref<Node>> childs; // Children nodes definitions

            bool  inserting = false; // Node insertion flag
            float insertCoef = 0.0f; // Inserting coefficient (0...1)

        public:
            // Destructor
            virtual ~Node();

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
            void*         object;
            Ref<TreeNode> widget;
            int           position;

        public:

            bool operator==(const VisibleWidgetDef& other) const;
        };

    protected:
        RearrangeType mRearrangeType = RearrangeType::Enabled; // Current available rearrange type @SERIALIZABLE
        bool          mMultiSelectAvailable = true;            // Is multi selection available @SERIALIZABLE

        Ref<TreeNode> mNodeWidgetSample; // Item sample @SERIALIZABLE

        float mChildrenOffset = 10.0f; // Children nodes offset from parent @SERIALIZABLE

        bool mIsNeedUpdateView = false;         // Is tree needs to be rebuild
        bool mIsNeedUdateLayout = false;        // Is layout needs to rebuild
        bool mIsNeedUpdateVisibleNodes = false; // In need to update visible nodes

        Vector<Ref<Node>> mAllNodes; // All expanded nodes definitions

        Vector<void*>     mSelectedObjects; // Selected objects
        Vector<Ref<Node>> mSelectedNodes;   // Selected nodes definitions

        Vector<Ref<TreeNode>> mNodeWidgetsBuf; // Nodes widgets buffer
        Vector<Ref<Node>>     mNodesBuf;       // Nodes buffer

        Vector<Ref<Node>> mVisibleNodes;           // Visible nodes
        int               mMinVisibleNodeIdx = 0;  // Minimal visible node index
        int               mMaxVisibleNodeIdx = -1; // Maximum visible node index

        Vec2F mLastClickPos; // Last click position in scroll space (depends on scroll position)

        WeakRef<TreeNode> mHoveredItem;      // Current hovered tree node item
        Ref<Sprite>       mHoverDrawable;    // Selection sprite @SERIALIZABLE
        Layout            mHoverLayout;      // Selection layout, result selection area depends on selected item @SERIALIZABLE                                                                     
        RectF             mCurrentHoverRect; // Current selection rectangle (for smoothing)
        RectF             mTargetHoverRect;  // Target selection rectangle (over selected item)    

        bool          mIsDraggingNodes = false; // Is nodes moving by cursor
        Ref<TreeNode> mFakeDragNode;            // Dragging node
        Vec2F         mDragOffset;              // Offset from cursor to dragging node's center
        Ref<TreeNode> mInsertNodeCandidate;     // Insertion node candidate when dragging nodes
        Vector<void*> mBeforeDragSelectedItems; // Before drag begin selection
        bool          mDragEnded = false;       // Is dragging ended and it needs to call EndDragging

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

        Ref<TreeNode> mExpandNodeCandidate;      // Expand node candidate when dragging
        float         mExpandInsertTime = -1.0f; // Remaining time to expanding item under cursor when dragging nodes
        float         mPressedTime = 10.0f;      // Time from last item pressing

        float mNodeExpandTime = 2.0f;   // Node expand time when dragging actors @SERIALIZABLE
        float mNodeDragIntoZone = 0.3f; // Node inside dragging zone coefficient (0.5 is full node area) @SERIALIZABLE

        Ref<AnimationClip>   mHighlighClip;                             // Node highlight animation clip @SERIALIZABLE 
        Ref<AnimationPlayer> mHighlightAnim = mmake<AnimationPlayer>(); // Node highlight animation
        Ref<Sprite>          mHighlightSprite;                          // Node highlight sprite @SERIALIZABLE
        Layout               mHighlightLayout;                          // Node highlight sprite layout @SERIALIZABLE
        WeakRef<Node>        mHighlighNode = nullptr;                   // Hightlighing node
        void*                mHighlightObject;                          // Highlight object
        
        Ref<Sprite> mZebraBackLine; // Dark zebra line sprite. When it is null, no zebra back doesn't draw @SERIALIZABLE

        Vector<VisibleWidgetDef> mVisibleWidgetsCache; // Visible widgets cache

    protected:
        // Called when widget was selected
        void OnFocused() override;

        // Called when widget was deselected
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
        virtual void FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object);

        // Free node data
        virtual void FreeNodeData(const Ref<TreeNode>& nodeWidget, void* object);

        // Called when tree node was double clicked
        virtual void OnNodeDblClick(const Ref<TreeNode>& nodeWidget);

        // Called when tree node was clicked by right button
        virtual void OnNodeRBClick(const Ref<TreeNode>& nodeWidget);

        // Called when list of selected objects was changed
        virtual void OnNodesSelectionChanged(Vector<void*> objects);

        // Called when objects was dragged in new parent in position next of prevObject
        virtual void OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject);

        // Called when node was expanded
        virtual void OnNodeExpanded(void* object) {}

        // Called when node was collapsed
        virtual void OnNodeCollapsed(void* object) {}

// ISelectableDragableObjectsGroup implementation

        // Returns selected objects in group
        Vector<Ref<SelectableDragableObject>> GetSelectedDragObjects() const override;

        // Returns all objects in group 
        Vector<Ref<SelectableDragableObject>> GetAllObjects() const override;

        // Selects object
        void Select(const Ref<SelectableDragableObject>& object) override;

        // Selects object
        void Select(const Ref<SelectableDragableObject>& object, bool sendOnSelectionChanged);

        // Deselects object
        void Deselect(const Ref<SelectableDragableObject>& object) override;

        // Adds selectable object to group
        void AddSelectableObject(const Ref<SelectableDragableObject>& object) override;

        // Removes selectable object from group
        void RemoveSelectableObject(SelectableDragableObject* object) override;

        // Called when selectable draggable object was released
        void OnSelectableObjectCursorReleased(const Ref<SelectableDragableObject>& object, const Input::Cursor& cursor) override;

        // Called when selectable object was began to drag
        void OnSelectableObjectBeganDragging(const Ref<SelectableDragableObject>& object) override;

// -------------

        // Checks multiple selection nodes (when Shift key is down) and returns is someone was selected
        bool CheckMultipleSelection(const Vec2F& point);

        // Called when items selected
        void OnSelectionChanged();

        // Updates highlight animation
        void UpdateHighlighting(float dt);

        // Checks and updates pressed node expanding
        void UpdatePressedNodeExpand(float dt);

        // Updates root nodes and their childs if need
        virtual void UpdateNodesStructure();

        // Inserts node to hierarchy
        int InsertNodes(const Ref<Node>& parentNode, int position, Vector<Ref<Node>>* newNodes = nullptr);

        // Removes node from hierarchy
        void RemoveNodes(const Ref<Node>& parentNode);

        // Creates node from object with parent
        Ref<Node> CreateNode(void* object, const Ref<Node>& parent);

        // Updates visible nodes (calculates range and initializes nodes)
        virtual void UpdateVisibleNodes();

        // Creates UITreeNode for visible node at position i
        void CreateVisibleNodeWidget(const Ref<Node>& node, int i);

        // Updates node view
        void UpdateNodeView(const Ref<Node>& node, const Ref<TreeNode>& widget, int idx);

        // Updates node widget layout
        void UpdateNodeWidgetLayout(const Ref<Node>& node, int idx);

        // Returns node index from mAllNodes by position in local coordinates
        int GetNodeIndex(float position) const;

        // Returns node position in local coordinates by index in mAllNodes array
        float GetNodePosition(int idx) const;

        // Expands node: creates his children and sets expanded state
        void ExpandNode(const Ref<Node>& node);

        // Collapses node: removes children from hierarchy and widgets if they're visible
        void CollapseNode(const Ref<Node>& node);

        // Starts expanding or collapse animation of node
        void StartExpandingAnimation(ExpandState direction, const Ref<Node>& node, int childrenCount);

        // Updates node expanding
        void UpdateNodeExpanding(float dt);

        // Calculates scroll area
        void CalculateScrollArea() override;

        // Moves scroll position and updates children widgets clipping and layout
        void MoveScrollPosition(const Vec2F& delta) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Updates hover target rect and visibility
        void UpdateHover(const Ref<TreeNode>& itemUnderCursor);

        // Gets tree node from pool or creates new
        virtual Ref<TreeNode> CreateTreeNodeWidget();

// DragDropArea implementation

        // Begins dragging selected items
        void BeginDragging(const Ref<TreeNode>& node);

        // Ends dragging items
        void EndDragging(bool droppedToThis = false);

        // Updates dragging graphics
        void UpdateDraggingGraphics();

        // Updates insert states on nodes when dragging
        void UpdateDraggingInsertion();

        // Updates insertion animation when dragging
        void UpdateDraggingInsertionAnim(float dt);

        // Called when some drag listeners was entered to this area
        void OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was dragged above this area
        void OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was exited from this area
        void OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        REF_COUNTERABLE_IMPL(Widget);

        friend class TreeNode;
    };

    // ------------
    // UI Tree node
    // ------------
    class TreeNode: public Widget, public SelectableDragableObject
    {
    public:
        // Default constructor
        explicit TreeNode(RefCounter* refCounter);

        // Copy-constructor
        TreeNode(RefCounter* refCounter, const TreeNode& other);

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
        CLONEABLE_REF(TreeNode);

    protected:
        WeakRef<Tree::Node> mNodeDef;   // Node definition

        WeakRef<Tree> mOwnerTree; // Owner tree
        Ref<Button>   mExpandBtn; // Node expanding button

        Ref<WidgetState> mSelectedState; // Selected state cached

    protected:
        // Called when widget state was added, caches selected state
        void OnStateAdded(const Ref<WidgetState>& state) override;

        // Updates expanding
        void UpdateTreeLayout(float dt);

        // Called when cursor double clicked
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when cursor enters this object, moving hover of tree to this
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object, moving hover of tree to this
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when started dragging
        void OnDragStart(const Input::Cursor& cursor) override;

        // Called when dragged
        void OnDragged(const Input::Cursor& cursor, const Ref<DragDropArea>& area) override;

        // Called when dragging completed
        void OnDragEnd(const Input::Cursor& cursor) override;

        // Called when this was selected
        void OnSelected() override;

        // Called when this was unselected
        void OnDeselected() override;

        REF_COUNTERABLE_IMPL(Widget);

        friend class Tree;
    };
}
// --- META ---

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
    FIELD().PUBLIC().NAME(getObjectParentDelegate);
    FIELD().PUBLIC().NAME(getObjectChildrenDelegate);
    FIELD().PUBLIC().NAME(fillNodeDataByObjectDelegate);
    FIELD().PUBLIC().NAME(freeNodeDataDelegate);
    FIELD().PUBLIC().NAME(getDebugForObject);
    FIELD().PUBLIC().NAME(onNodeDoubleClicked);
    FIELD().PUBLIC().NAME(onNodeRightButtonClicked);
    FIELD().PUBLIC().NAME(onObjectsSelectionChanged);
    FIELD().PUBLIC().NAME(onDraggedObjects);
    FIELD().PUBLIC().NAME(onNodeExpanded);
    FIELD().PUBLIC().NAME(onNodeCollapsed);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(RearrangeType::Enabled).NAME(mRearrangeType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMultiSelectAvailable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mNodeWidgetSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10.0f).NAME(mChildrenOffset);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsNeedUpdateView);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsNeedUdateLayout);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsNeedUpdateVisibleNodes);
    FIELD().PROTECTED().NAME(mAllNodes);
    FIELD().PROTECTED().NAME(mSelectedObjects);
    FIELD().PROTECTED().NAME(mSelectedNodes);
    FIELD().PROTECTED().NAME(mNodeWidgetsBuf);
    FIELD().PROTECTED().NAME(mNodesBuf);
    FIELD().PROTECTED().NAME(mVisibleNodes);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mMinVisibleNodeIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mMaxVisibleNodeIdx);
    FIELD().PROTECTED().NAME(mLastClickPos);
    FIELD().PROTECTED().NAME(mHoveredItem);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverLayout);
    FIELD().PROTECTED().NAME(mCurrentHoverRect);
    FIELD().PROTECTED().NAME(mTargetHoverRect);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDraggingNodes);
    FIELD().PROTECTED().NAME(mFakeDragNode);
    FIELD().PROTECTED().NAME(mDragOffset);
    FIELD().PROTECTED().NAME(mInsertNodeCandidate);
    FIELD().PROTECTED().NAME(mBeforeDragSelectedItems);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDragEnded);
    FIELD().PROTECTED().NAME(mExpandedObjects);
    FIELD().PROTECTED().DEFAULT_VALUE(ExpandState::None).NAME(mExpandingNodeState);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mExpandingNodeIdx);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mExpandingNodeChildsCount);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandingNodePosition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeBottomPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeCurrCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeCurrHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mExpandingNodeTargetHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(0.4f).NAME(mExpandNodeTime);
    FIELD().PROTECTED().DEFAULT_VALUE(Curve::EaseInOut()).NAME(mExpandingNodeFunc);
    FIELD().PROTECTED().NAME(mExpandNodeCandidate);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mExpandInsertTime);
    FIELD().PROTECTED().DEFAULT_VALUE(10.0f).NAME(mPressedTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(2.0f).NAME(mNodeExpandTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.3f).NAME(mNodeDragIntoZone);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlighClip);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(mHighlightAnim);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightSprite);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHighlightLayout);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mHighlighNode);
    FIELD().PROTECTED().NAME(mHighlightObject);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mZebraBackLine);
    FIELD().PROTECTED().NAME(mVisibleWidgetsCache);
}
END_META;
CLASS_METHODS_META(o2::Tree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Tree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Tree&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectCreated, void*, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectRemoved, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectsChanged, const Vector<void*>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildren, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildrenTransforms);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateNodesView, bool);
    FUNCTION().PUBLIC().SIGNATURE(Ref<TreeNode>, GetNode, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, ExpandAll);
    FUNCTION().PUBLIC().SIGNATURE(void, CollapseAll);
    FUNCTION().PUBLIC().SIGNATURE(Vector<void*>, GetSelectedObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedObjects, const Vector<void*>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectObject, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAndHighlightObject, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectObject, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectAllObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, ScrollTo, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, ScrollToAndHighlight, void*);
    FUNCTION().PUBLIC().SIGNATURE(void, ExpandParentObjects, void*);
    FUNCTION().PUBLIC().SIGNATURE(Ref<TreeNode>, GetTreeNodeUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRearrangeType, RearrangeType);
    FUNCTION().PUBLIC().SIGNATURE(RearrangeType, GetRearrangeType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMultipleSelectionAvailable, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMultiSelectionAvailable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<TreeNode>&, GetNodeSample);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNodeSample, const Ref<TreeNode>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetHoverDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetHighlightDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHighlightLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetZebraBackLine, const Ref<Sprite>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetZebraBackLine);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNodeExpandTimer, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetNodeExpandTimer);
    FUNCTION().PUBLIC().SIGNATURE(void, SetChildsNodesOffset, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetChildsNodesOffset);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawZebraBack);
    FUNCTION().PROTECTED().SIGNATURE(void*, GetObjectParent, void*);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetObjectChilds, void*);
    FUNCTION().PROTECTED().SIGNATURE(String, GetObjectDebug, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, FillNodeDataByObject, const Ref<TreeNode>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeNodeData, const Ref<TreeNode>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeDblClick, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeRBClick, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodesSelectionChanged, Vector<void*>);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedObjects, Vector<void*>, void*, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeExpanded, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeCollapsed, void*);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<SelectableDragableObject>>, GetSelectedDragObjects);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<SelectableDragableObject>>, GetAllObjects);
    FUNCTION().PROTECTED().SIGNATURE(void, Select, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Select, const Ref<SelectableDragableObject>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, Deselect, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, AddSelectableObject, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveSelectableObject, SelectableDragableObject*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectableObjectCursorReleased, const Ref<SelectableDragableObject>&, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectableObjectBeganDragging, const Ref<SelectableDragableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(bool, CheckMultipleSelection, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHighlighting, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdatePressedNodeExpand, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateNodesStructure);
    FUNCTION().PROTECTED().SIGNATURE(int, InsertNodes, const Ref<Node>&, int, Vector<Ref<Node>>*);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveNodes, const Ref<Node>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Node>, CreateNode, void*, const Ref<Node>&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateVisibleNodeWidget, const Ref<Node>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateNodeView, const Ref<Node>&, const Ref<TreeNode>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateNodeWidgetLayout, const Ref<Node>&, int);
    FUNCTION().PROTECTED().SIGNATURE(int, GetNodeIndex, float);
    FUNCTION().PROTECTED().SIGNATURE(float, GetNodePosition, int);
    FUNCTION().PROTECTED().SIGNATURE(void, ExpandNode, const Ref<Node>&);
    FUNCTION().PROTECTED().SIGNATURE(void, CollapseNode, const Ref<Node>&);
    FUNCTION().PROTECTED().SIGNATURE(void, StartExpandingAnimation, ExpandState, const Ref<Node>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateNodeExpanding, float);
    FUNCTION().PROTECTED().SIGNATURE(void, CalculateScrollArea);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<TreeNode>, CreateTreeNodeWidget);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginDragging, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, EndDragging, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDraggingGraphics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDraggingInsertion);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDraggingInsertionAnim, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnter, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedAbove, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragExit, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
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
    FIELD().PROTECTED().NAME(mNodeDef);
    FIELD().PROTECTED().NAME(mOwnerTree);
    FIELD().PROTECTED().NAME(mExpandBtn);
    FIELD().PROTECTED().NAME(mSelectedState);
}
END_META;
CLASS_METHODS_META(o2::TreeNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TreeNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse, bool);
    FUNCTION().PUBLIC().SIGNATURE(void*, GetObject);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedState, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFocusedState, bool);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateAdded, const Ref<WidgetState>&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTreeLayout, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragStart, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragged, const Input::Cursor&, const Ref<DragDropArea>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnd, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeselected);
}
END_META;
// --- END META ---
