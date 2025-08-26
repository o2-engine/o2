#pragma once

#include "o2/Utils/Singleton.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/IEditorWindow.h"

namespace o2
{
    class SceneEditableObject;
    class UIButtonGroup;
    class ContextMenu;
    class EditBox;
    class Toggle;
    class ToggleGroup;
}

using namespace o2;

// Editor actors tree access macros
#define o2EditorTree TreeWindow::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(SceneHierarchyTree);
    FORWARD_CLASS_REF(DrawOrderTree);

    // -----------------------------------------------------------------
    // Actors tree window. Shows scene hierarchy tree or draw order tree
    // -----------------------------------------------------------------
    class TreeWindow: public Singleton<TreeWindow>, public IEditorWindow
    {
    public:
        // Default constructor
        explicit TreeWindow(RefCounter* refCounter);

        // Copy-constructor
        TreeWindow(RefCounter* refCounter, const TreeWindow& other);

        // Destructor
        ~TreeWindow();

        // Returns actors tree widget
        const Ref<SceneHierarchyTree>& GetSceneTree() const;

        // Returns draw order tree widget
        const Ref<DrawOrderTree>& GetDrawOrderTree() const;

		// Called when scene was focused, updates context menu priority
		void OnSceneFocused();

        // Sets selected objects
        void SetSelectedObjects(const Vector<Ref<SceneEditableObject>>& objects);

        // Updates current tree
        void UpdateTreeView();

        // Restores expanded state from cache
        void RestoreExpandedFromCache();

        // Expands all actor's parents nodes in tree and highlights actor
        void HighlightObjectTreeNode(const Ref<SceneEditableObject>& targetObject);

        // Sets widgets layers visibility in hierarchy
        void SetWidgetsLayersVisible(bool visible);

        // Returns widgets layers visibility in hierarchy
        bool IsWidgetsLayersVisible() const;

        // Sets widgets internal children visibility in hierarchy
        void SetWidgetsInternalChildrenVisible(bool visible);

        // Returns widgets internal children visibility in hierarchy
        bool IsWidgetsInternalChildrenVisible() const;

        // Dynamic cast to RefCounterable via Singleton<TreeWindow>
        static Ref<RefCounterable> CastToRefCounterable(const Ref<TreeWindow>& ref);

        IOBJECT(TreeWindow);
        REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<TreeWindow>);

    protected:
        Ref<Toggle>  mListTreeToggle; // Toggle between list and tree views
        Ref<EditBox> mSearchEditBox;  // Search actors edit box

        Ref<SceneHierarchyTree> mSceneTree;     // Scene hierarchy tree
        Ref<DrawOrderTree>      mDrawOrderTree; // Draw order tree

        Ref<ContextMenu> mTreeContextMenu; // Context menu

        Vector<Ref<SceneEditableObject>> mSearchObjects;    // Array of searched objects
        bool                             mInSearch = false; // True when searching objects (mSearchEditBox isn't empty)

    protected:
        // Initializes window
        void InitializeWindow();

        // Initializes actors tree widget
        void InitializeSceneTree();

        // Initializes draw order tree widget
        void InitializeDrawOrderTree();

        // Initializes top panel with filter
        void InitializeTopPanel();

        // Initializes context menu 
        void InitializeContextMenu();

        // Gets all actor types and creates create sub menu
        void InitializeCreateMenu();

        // Creates creation menu items for UI style
        void InitUIStyleCreateMenu();

        // Creates creation menu for UI layers
        void InitUILayersCreateMenu();

        // Called after that all windows was created
        void PostInitializeWindow() override;

        // Called when search button pressed
        void OnSearchPressed();

        // Called when list-tree toggle changed value
        void OnListTreeToggled(bool value);

        // Called when search field was changed
        void OnSearchEdited(const WString& searchStr);

        // Searches actors
        void SearchObjectsRecursive(const Ref<SceneEditableObject>& object, const String& searchStr);

        // Called when tree node clicked by right button (For tree widget)
        void OnTreeRBPressed(const Ref<TreeNode>& node);

        template<typename _type>
        void CreateObject(const String& name);

        // Called when some object needs to be created and registers object insert action
        void OnCreateObject(const Ref<SceneEditableObject>& newObject);

        // Called when pressed "Create new" in context menu
        void OnContextCreateNewPressed();

        // Called when pressed "Create sprite" in context menu
        void OnContextCreateSprite();

        // Called when pressed "Create button" in context menu
        void OnContextCreateButton();

        // Called when pressed "Copy" in context menu
        void OnContextCopyPressed();

        // Called when pressed "Cut" in context menu
        void OnContextCutPressed();

        // Called when pressed "Paste" in context menu
        void OnContextPastePressed();

        // Called when pressed "Delete" in context menu
        void OnContextDeletePressed();

        // Called when pressed "Duplicate" in context menu
        void OnContextDuplicatePressed();

        // Called when pressed "Expand all" in context menu
        void OnContextExpandAllPressed();

        // Called when pressed "Collapse all" in context menu
        void OnContextCollapseAllPressed();

        // Called when pressed "Lock/unlock" in context menu
        void OnContextLockPressed();

        // Called when pressed "Enable/disable" in context menu
        void OnContextEnablePressed();

        // Called when view layers menu item toggled
        void OnViewLayersToggled(bool view);

        // Called when view internal children menu item toggled
        void OnViewInternalChildrenToggled(bool view);

        // Called when actor was created
        void OnActorCreated(const Ref<SceneEditableObject>& object);

        // Called when actor was destroyed
        void OnActorDestroyed(const Ref<SceneEditableObject>& object);

        friend class SceneEditWidget;
    };

    template<typename _type>
    void TreeWindow::CreateObject(const String& name)
    {
        ForcePopEditorScopeOnStack scope;

        auto object = mmake<_type>();
        object->SetName(name);
        OnCreateObject(object);
    }

}
// --- META ---

CLASS_BASES_META(Editor::TreeWindow)
{
    BASE_CLASS(o2::Singleton<TreeWindow>);
    BASE_CLASS(Editor::IEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::TreeWindow)
{
    FIELD().PROTECTED().NAME(mListTreeToggle);
    FIELD().PROTECTED().NAME(mSearchEditBox);
    FIELD().PROTECTED().NAME(mSceneTree);
    FIELD().PROTECTED().NAME(mDrawOrderTree);
    FIELD().PROTECTED().NAME(mTreeContextMenu);
    FIELD().PROTECTED().NAME(mSearchObjects);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mInSearch);
}
END_META;
CLASS_METHODS_META(Editor::TreeWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TreeWindow&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SceneHierarchyTree>&, GetSceneTree);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<DrawOrderTree>&, GetDrawOrderTree);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSceneFocused);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedObjects, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateTreeView);
    FUNCTION().PUBLIC().SIGNATURE(void, RestoreExpandedFromCache);
    FUNCTION().PUBLIC().SIGNATURE(void, HighlightObjectTreeNode, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidgetsLayersVisible, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsWidgetsLayersVisible);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWidgetsInternalChildrenVisible, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsWidgetsInternalChildrenVisible);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<TreeWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeSceneTree);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDrawOrderTree);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTopPanel);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitUIStyleCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitUILayersCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, PostInitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSearchPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnListTreeToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSearchEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, SearchObjectsRecursive, const Ref<SceneEditableObject>&, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTreeRBPressed, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCreateObject, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCreateNewPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCreateSprite);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCreateButton);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCopyPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCutPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextPastePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextDeletePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextDuplicatePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextExpandAllPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCollapseAllPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextLockPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextEnablePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnViewLayersToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnViewInternalChildrenToggled, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActorCreated, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActorDestroyed, const Ref<SceneEditableObject>&);
}
END_META;
// --- END META ---
