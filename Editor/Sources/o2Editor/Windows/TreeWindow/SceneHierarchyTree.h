#pragma once

#include "o2/Scene/UI/Widgets/Tree.h"

namespace o2
{
    class Actor;
    class SceneEditableObject;
    class Button;
    class EditBox;
    class Toggle;
    class ToggleGroup;
}

using namespace o2;

namespace Editor
{
    // -----------------
    // Scene tree widget
    // -----------------
    class SceneHierarchyTree : public Tree
    {
    public:
        Function<void(const Vector<Ref<SceneEditableObject>>&)> onObjectsSelectionChanged; // Callback on objects selection changed

    public:
        // Default constructor
        SceneHierarchyTree(RefCounter* refCounter);

        // Copy-constructor
        SceneHierarchyTree(RefCounter* refCounter, const SceneHierarchyTree& other);

        // Copy-constructor
        SceneHierarchyTree(const SceneHierarchyTree& other);

        // Destructor
        ~SceneHierarchyTree();

        // Copy-operator
        SceneHierarchyTree& operator=(const SceneHierarchyTree& other);

        // Attaches to scene events
        void AttachToSceneEvents();

        // Deattaches from scene events
        void DeattachFromSceneEvents();

        // Returns ui node for object
        Ref<TreeNode> GetNode(const Ref<SceneEditableObject>& object);

        // Returns selected objects vector
        Vector<Ref<SceneEditableObject>> GetSelectedObjects() const;

        // Sets selected objects
        void SetSelectedObjects(const Vector<Ref<SceneEditableObject>>& objects);

        // Selects object
        void SelectObject(const Ref<SceneEditableObject>& object);

        // Selects object
        void SelectAndHighlightObject(const Ref<SceneEditableObject>& object);

        // Scrolls view to object and highlights
        void ScrollToAndHighlight(const Ref<SceneEditableObject>& object);

        // Deselects object
        void DeselectObject(const Ref<SceneEditableObject>& object);

        // Deselects all objects
        void DeselectAllObjects();

        // Scrolls view to object's tree node
        void ScrollTo(const Ref<SceneEditableObject>& object);

        // Restores expanded state from cache
        void RestoreExpandedFromCache();

        // Sets watching editor UI state
        void SetEditorWatching(bool watching);

        // Returns is watching editor UI state
        bool IsEditorWatching() const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(SceneHierarchyTree);
        CLONEABLE_REF(SceneHierarchyTree);

    protected:
        Ref<ToggleGroup> mEnableTogglesGroup;    // Enable objects toggles group
        Ref<ToggleGroup> mLockTogglesGroup;         // Lock objects toggles group
        bool         mAttachedToSceneEvents; // Is tree attached to scene events

        Vector<SceneUID> mExpandedActorsCache; // Expanded actors cache, used to restore expanded state after scene reload

        bool mWatchEditor = false;

    protected:
        // Initializes widget logic
        void Initialize();

        // Updates visible nodes (calculates range and initializes nodes), enables editor mode
        void UpdateVisibleNodes() override;

        // Gets tree node from pool or creates new, enables editor mode
        Ref<TreeNode> CreateTreeNodeWidget() override;

        // Returns object's parent
        void* GetObjectParent(void* object) override;

        // Returns object's children
        Vector<void*> GetObjectChilds(void* object) override;

        // Returns debugging string for object
        String GetObjectDebug(void* object) override;

        // Sets nodeWidget data by object
        void FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object) override;

        // Called when tree node was double clicked
        void OnNodeDblClick(const Ref<TreeNode>& nodeWidget) override;

        // Called when objects was dragged in new parent in position next of prevObject
        void OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject) override;

        // Called when object was created
        void OnObjectCreated(const Ref<SceneEditableObject>& object);

        // Called when object was destroyed
        void OnObjectDestroing(const Ref<SceneEditableObject>& object);

        // Called when some objects were changed
        void OnObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects);

        // Called when object was changed
        void OnObjectChanged(const Ref<SceneEditableObject>& object);

        // Called when enable objects toggle group released
        void EnableObjectsGroupReleased(bool value);

        // Called when lock objects toggle group released
        void LockObjectsGroupReleased(bool value);

        // Called when list of selected objects was changed
        void OnNodesSelectionChanged(Vector<void*> objects) override;

        // Called when some drag listeners was entered to this area
        void OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was exited from this area
        void OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was dragged above this area
        void OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when node was expanded
        void OnNodeExpanded(void* object) override;

        // Called when node was collapsed
        void OnNodeCollapsed(void* object) override;

        friend class SceneHierarchyTreeNode;
    };

    // ---------------
    // Scene tree node
    // ---------------
    class SceneHierarchyTreeNode : public TreeNode
    {
    public:
        // Default constructor
        SceneHierarchyTreeNode(RefCounter* refCounter);

        // Copy-constructor
        SceneHierarchyTreeNode(RefCounter* refCounter, const SceneHierarchyTreeNode& other);

        // Copy operator
        SceneHierarchyTreeNode& operator=(const SceneHierarchyTreeNode& other);

        // Sets object and updates content
        void SetSceneObject(const Ref<SceneEditableObject>& object);

        // Enables edit name edit box
        void EnableEditName();

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(SceneHierarchyTreeNode);
        CLONEABLE_REF(SceneHierarchyTreeNode);

    protected:
        Ref<SceneEditableObject> mTargetObject; // Target object

        Ref<Toggle>      mLockToggle;              // Lock toggle
        Ref<WidgetState> mLockToggleLockedState;   // Lock toggle locked state
        Ref<WidgetState> mLockToggleHalfHideState; // Lock toggle half hide state

        Ref<Toggle> mEnableToggle; // Enable toggle

        Ref<Button>      mLinkBtn;              // View link button
        Ref<WidgetState> mLinkBtnHalfHideState; // View link button half hide state

        Ref<Text>        mNameDrawable; // Object name drawable
        Ref<EditBox>     mNameEditBox;  // Object's name edit box
        Ref<WidgetState> mEditState;    // Object's name edit state

    protected:
        // Called on deserialization, initializes controls
        void OnDeserialized(const DataValue& node) override;

        // initializes controls and widgets
        void InitializeControls();

        // Called when lock toggle was clicked and changes target object's lock state
        void OnLockClicked();

        // Called when enable toggle was clicked and changes target object's enable state
        void OnEnableCkicked();

        // Called when object name edit box changed
        void OnObjectNameChanged(const WString& text);

        friend class SceneHierarchyTree;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneHierarchyTree)
{
    BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::SceneHierarchyTree)
{
    FIELD().PUBLIC().NAME(onObjectsSelectionChanged);
    FIELD().PROTECTED().NAME(mEnableTogglesGroup);
    FIELD().PROTECTED().NAME(mLockTogglesGroup);
    FIELD().PROTECTED().NAME(mAttachedToSceneEvents);
    FIELD().PROTECTED().NAME(mExpandedActorsCache);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mWatchEditor);
}
END_META;
CLASS_METHODS_META(Editor::SceneHierarchyTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneHierarchyTree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const SceneHierarchyTree&);
    FUNCTION().PUBLIC().SIGNATURE(void, AttachToSceneEvents);
    FUNCTION().PUBLIC().SIGNATURE(void, DeattachFromSceneEvents);
    FUNCTION().PUBLIC().SIGNATURE(Ref<TreeNode>, GetNode, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<SceneEditableObject>>, GetSelectedObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedObjects, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectObject, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAndHighlightObject, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ScrollToAndHighlight, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectObject, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeselectAllObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, ScrollTo, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RestoreExpandedFromCache);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditorWatching, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditorWatching);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, Initialize);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PROTECTED().SIGNATURE(Ref<TreeNode>, CreateTreeNodeWidget);
    FUNCTION().PROTECTED().SIGNATURE(void*, GetObjectParent, void*);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetObjectChilds, void*);
    FUNCTION().PROTECTED().SIGNATURE(String, GetObjectDebug, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, FillNodeDataByObject, const Ref<TreeNode>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeDblClick, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedObjects, Vector<void*>, void*, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectCreated, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectDestroing, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectChanged, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, EnableObjectsGroupReleased, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, LockObjectsGroupReleased, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodesSelectionChanged, Vector<void*>);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnter, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragExit, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedAbove, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeExpanded, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNodeCollapsed, void*);
}
END_META;

CLASS_BASES_META(Editor::SceneHierarchyTreeNode)
{
    BASE_CLASS(o2::TreeNode);
}
END_META;
CLASS_FIELDS_META(Editor::SceneHierarchyTreeNode)
{
    FIELD().PROTECTED().NAME(mTargetObject);
    FIELD().PROTECTED().NAME(mLockToggle);
    FIELD().PROTECTED().NAME(mLockToggleLockedState);
    FIELD().PROTECTED().NAME(mLockToggleHalfHideState);
    FIELD().PROTECTED().NAME(mEnableToggle);
    FIELD().PROTECTED().NAME(mLinkBtn);
    FIELD().PROTECTED().NAME(mLinkBtnHalfHideState);
    FIELD().PROTECTED().NAME(mNameDrawable);
    FIELD().PROTECTED().NAME(mNameEditBox);
    FIELD().PROTECTED().NAME(mEditState);
}
END_META;
CLASS_METHODS_META(Editor::SceneHierarchyTreeNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneHierarchyTreeNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSceneObject, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, EnableEditName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLockClicked);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnableCkicked);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectNameChanged, const WString&);
}
END_META;
// --- END META ---
