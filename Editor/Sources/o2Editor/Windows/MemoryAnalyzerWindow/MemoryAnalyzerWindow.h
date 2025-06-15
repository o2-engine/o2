#pragma once

#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Memory/MemoryAnalyzer.h"
#include "o2/Utils/Singleton.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"

namespace o2
{
    class Button;
    class EditBox;
    class Sprite;
    class Text;
    class Window;
}

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(MemoryAnalyzeTree);

    class MemoryAnalyzerWindow : public Singleton<MemoryAnalyzerWindow>
    {
    public:
        MemoryAnalyzerWindow(RefCounter* refCounter);
        ~MemoryAnalyzerWindow();

        static void Show(MemoryAnalyzer::MemoryNode* data);

    private:
        MemoryAnalyzer::MemoryNode* mData = nullptr;
        MemoryAnalyzer::MemoryNode* mSelected = nullptr;

        Ref<o2::Window> mWindow;

        Ref<Widget>            mTreeWidget;
        Ref<EditBox>           mFilter;
        Ref<MemoryAnalyzeTree> mMemoryTree;

        Ref<Widget>         mInspectorWidget;
        Ref<Label>          mObjectLabel;
        Ref<Label>          mAddressLabel;
        Ref<Label>          mSizeLabel;
        Ref<Button>         mReverseTreeButton;
        Ref<EditBox>        mStackText;
        Ref<VerticalLayout> mInspectorContainer;

        Ref<IObjectPropertiesViewer> mObjectPropertiesViewer;

        float mTreeViewWidth = 800.0f;    // Width of tree area. Changed by dragable separator
        float mMinTreeViewWidth = 250.0f; // Minimal tree width

        Ref<WidgetDragHandle> mTreeSeparatorHandle; // Tree separator handle. When it moves, it changes size of all dependent widgets

    private:
        // Initializes window and controls
        void InitializeWindow();

        // Initialized inspector widgets
        void InitializeInspector();

        // Initializes tree widget
        void InitializeTree();

        // Initializes separator handle view and events
        void InitializeSeparatorHandle();

        // Updates inspector by selected node
        void UpdateInspector(MemoryAnalyzer::MemoryNode* node);

        // Called when tree reverse button pressed
        void OnReversePressed();
    };

    class MemoryAnalyzeTree : public Tree
    {
    public:
        Function<void(MemoryAnalyzer::MemoryNode* data)> onSelectedNode;

    public:
        // Default constructor
        MemoryAnalyzeTree(RefCounter* refCounter);

        // Copy-constructor
        MemoryAnalyzeTree(RefCounter* refCounter, const MemoryAnalyzeTree& other);

        // Copy-constructor
        MemoryAnalyzeTree(const MemoryAnalyzeTree& other);

        // Copy operator
        MemoryAnalyzeTree& operator=(const MemoryAnalyzeTree& other);

        // Initializes data tree
        void Initialize(MemoryAnalyzer::MemoryNode* data, bool reversed);

        // Returns true if tree is reversed
        bool IsTreeReversed() const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(MemoryAnalyzeTree);
        CLONEABLE_REF(MemoryAnalyzeTree);

    private:
        WString mFilterStr; // Filtering string

        MemoryAnalyzer::MemoryNode* mRoot = nullptr; // Root data node

        bool mReversed = false; // Reversed tree view, from children to parents

    private:
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

        // Called when list of selected objects was changed
        void OnNodesSelectionChanged(Vector<void*> objects) override;

        friend class MemoryAnalyzeTreeNode;
    };

    class MemoryAnalyzeTreeNode : public TreeNode
    {
    public:
        // Default constructor
        MemoryAnalyzeTreeNode(RefCounter* refCounter);

        // Copy-constructor
        MemoryAnalyzeTreeNode(RefCounter* refCounter, const MemoryAnalyzeTreeNode& other);

        // Copy operator
        MemoryAnalyzeTreeNode& operator=(const MemoryAnalyzeTreeNode& other);

        // Initializes node by data
        void Setup(MemoryAnalyzer::MemoryNode* data, const Ref<MemoryAnalyzeTree>& tree);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(MemoryAnalyzeTreeNode);
        CLONEABLE_REF(MemoryAnalyzeTreeNode);

    private:
        Ref<Text> mName;
        Ref<Text> mAddress;
        Ref<Text> mSize;

        MemoryAnalyzer::MemoryNode* mData; // Data node pointer

        WeakRef<MemoryAnalyzeTree> mTree; // Owner tree

    private:
        // Called on deserialization, initializes controls
        void OnDeserialized(const DataValue& node) override;

        // initializes controls and widgets
        void InitializeControls();

        friend class MemoryAnalyzeTree;
    };
}
// --- META ---

CLASS_BASES_META(Editor::MemoryAnalyzeTree)
{
    BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::MemoryAnalyzeTree)
{
    FIELD().PUBLIC().NAME(onSelectedNode);
    FIELD().PRIVATE().NAME(mFilterStr);
    FIELD().PRIVATE().DEFAULT_VALUE(nullptr).NAME(mRoot);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mReversed);
}
END_META;
CLASS_METHODS_META(Editor::MemoryAnalyzeTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MemoryAnalyzeTree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const MemoryAnalyzeTree&);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize, MemoryAnalyzer::MemoryNode*, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsTreeReversed);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PRIVATE().SIGNATURE(Ref<TreeNode>, CreateTreeNodeWidget);
    FUNCTION().PRIVATE().SIGNATURE(void*, GetObjectParent, void*);
    FUNCTION().PRIVATE().SIGNATURE(Vector<void*>, GetObjectChilds, void*);
    FUNCTION().PRIVATE().SIGNATURE(String, GetObjectDebug, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, FillNodeDataByObject, const Ref<TreeNode>&, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, OnNodesSelectionChanged, Vector<void*>);
}
END_META;

CLASS_BASES_META(Editor::MemoryAnalyzeTreeNode)
{
    BASE_CLASS(o2::TreeNode);
}
END_META;
CLASS_FIELDS_META(Editor::MemoryAnalyzeTreeNode)
{
    FIELD().PRIVATE().NAME(mName);
    FIELD().PRIVATE().NAME(mAddress);
    FIELD().PRIVATE().NAME(mSize);
    FIELD().PRIVATE().NAME(mData);
    FIELD().PRIVATE().NAME(mTree);
}
END_META;
CLASS_METHODS_META(Editor::MemoryAnalyzeTreeNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MemoryAnalyzeTreeNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, MemoryAnalyzer::MemoryNode*, const Ref<MemoryAnalyzeTree>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
}
END_META;
// --- END META ---
