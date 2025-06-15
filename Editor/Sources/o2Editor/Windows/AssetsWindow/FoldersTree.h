#pragma once

#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Tree.h"

using namespace o2;

namespace o2
{
    class ContextMenu;
}

namespace Editor
{
    FORWARD_CLASS_REF(FoldersTree);

    // --------------------------
    // Assets folders tree widget
    // --------------------------
    class AssetsFoldersTree: public Widget, public KeyboardEventsListener
    {
    public:
        // Default constructor
        AssetsFoldersTree(RefCounter* refCounter);

        // Copy-constructor
        AssetsFoldersTree(RefCounter* refCounter, const AssetsFoldersTree& other);

        // Destructor
        ~AssetsFoldersTree();

        // Copy-operator
        AssetsFoldersTree& operator=(const AssetsFoldersTree& other);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AssetsFoldersTree);
        CLONEABLE_REF(AssetsFoldersTree);

    protected:
        Ref<Tree>        mFoldersTree; // Folders tree
        Ref<ContextMenu> mContextMenu; // Context menu
        String           mCurrentPath; // Current viewing path

        bool mOpengingFolderFromThis = false;

    protected:
        // Selects and expands folder
        void SelectAndExpandFolder(const String& path);

        // Rebuilds folders tree
        void UpdateView();

        // Initializes assets context menu
        void InitializeContext();

        // Returns folder parent (for folders tree)
        void* GetFoldersTreeNodeParent(void* object);

        // Returns folder childs (for folders tree)
        Vector<void*> GetFoldersTreeNodeChilds(void* object);

        // Setups tree node by folder (for folders tree)
        void SetupFoldersTreeNode(const Ref<TreeNode>& node, void* object);

        // Called when folder item double clicked (for folders tree)
        void OnFoldersTreeNodeDblClick(const Ref<TreeNode>& node);

        // Called when folder item clicked (for folders tree)
        void OnFoldersTreeSelect(Vector<void*> nodes);

        // Called when folders tree clicked by right button
        void OnFoldersTreeRightClick(const Ref<TreeNode>& node);

        // Called when context copy pressed
        void OnContextCopyPressed();

        // Called when context cut pressed
        void OnContextCutPressed();

        // Called when context paste pressed
        void OnContextPastePressed();

        // Called when context delete pressed
        void OnContextDeletePressed();

        // Called when context open pressed
        void OnContextOpenPressed();

        // Called when context show in explorer pressed
        void OnContextShowInExplorerPressed();

        // Called when context create folder pressed
        void OnContextCreateFolderPressed();

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;
        
        REF_COUNTERABLE_IMPL(Widget);

        friend class AssetsWindow;
    };

    class FoldersTree : public Tree
    {
    public:
        // Default constructor
        FoldersTree(RefCounter* refCounter);

        // Copy-constructor
        FoldersTree(RefCounter* refCounter, const FoldersTree& other);

        // Copy-constructor
        FoldersTree(const FoldersTree& other);

        SERIALIZABLE(FoldersTree);
        CLONEABLE_REF(FoldersTree);

    protected:
        // Updates visible nodes (calculates range and initializes nodes), enables editor mode
        void UpdateVisibleNodes() override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();
    };
}
// --- META ---

CLASS_BASES_META(Editor::AssetsFoldersTree)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AssetsFoldersTree)
{
    FIELD().PROTECTED().NAME(mFoldersTree);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().NAME(mCurrentPath);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOpengingFolderFromThis);
}
END_META;
CLASS_METHODS_META(Editor::AssetsFoldersTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetsFoldersTree&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, SelectAndExpandFolder, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContext);
    FUNCTION().PROTECTED().SIGNATURE(void*, GetFoldersTreeNodeParent, void*);
    FUNCTION().PROTECTED().SIGNATURE(Vector<void*>, GetFoldersTreeNodeChilds, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupFoldersTreeNode, const Ref<TreeNode>&, void*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFoldersTreeNodeDblClick, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFoldersTreeSelect, Vector<void*>);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFoldersTreeRightClick, const Ref<TreeNode>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCopyPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCutPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextPastePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextDeletePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextOpenPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextShowInExplorerPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContextCreateFolderPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
}
END_META;

CLASS_BASES_META(Editor::FoldersTree)
{
    BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::FoldersTree)
{
}
END_META;
CLASS_METHODS_META(Editor::FoldersTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const FoldersTree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const FoldersTree&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(String, GetCreateMenuCategory);
}
END_META;
// --- END META ---
