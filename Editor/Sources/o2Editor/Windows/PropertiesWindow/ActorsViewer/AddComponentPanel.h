#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Tree.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class Button;
}

namespace Editor
{
    FORWARD_CLASS_REF(ActorViewer);
    FORWARD_CLASS_REF(ComponentsTree);

    // --------------------------------------------------------------------------------
    // Add component to actor panel. Shows filter input, add button and components tree
    // --------------------------------------------------------------------------------
    class AddComponentPanel : public Widget, public CursorEventsArea, public KeyboardEventsListener
    {
    public:
        // Default constructor
        AddComponentPanel(RefCounter* refCounter);

        // Constructor
        AddComponentPanel(RefCounter* refCounter, const Ref<ActorViewer>& viewer);

        // Draws widget, calls OnDrawn for CursorEventsListener
        void Draw() override;

        // Returns filter widget
        const Ref<EditBox>& GetFilter() const;

        // Returns tree widget
        const Ref<ComponentsTree>& GetTree() const;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<AddComponentPanel>& ref);

        SERIALIZABLE(AddComponentPanel);
        CLONEABLE_REF(AddComponentPanel);

    private:
        WeakRef<ActorViewer> mViewer; // Owner actors viewer

        Ref<EditBox>        mFilterBox; // Components names filter edit box, updates list of component when edit
        Ref<Button>         mAddButton; // Add button
        Ref<ComponentsTree> mTree;      // Components tree

    private:
        // Called when add button pressed. Adds selected component to target actors from viewer
        void OnAddPressed();

        // Creates component by type
        void CreateComponent(const ObjectType* objType);

        // Called when tree node was double clicked
        void OnNodeDblClick(const Ref<TreeNode>& nodeWidget);

        // Called when key was released. When returns has pressed, component is creating
        void OnKeyReleased(const Input::Key& key) override;

        REF_COUNTERABLE_IMPL(Widget, CursorEventsArea);
    };
    
    // ---------------------------------------------------------------------
    // Components tree. Builds data tree by available components with filter
    // ---------------------------------------------------------------------
    class ComponentsTree : public Tree
    {
    public:
        struct NodeData: public RefCounterable
        {
            WeakRef<NodeData>     parent ;
            Vector<Ref<NodeData>> children;

            String name;
            String path;
            String icon = "ui/UI4_component_icon.png";

            const Type* type = nullptr;

        public:
            ~NodeData();

            void Clear();
            Ref<NodeData> AddChild(const String& name, const Type* type);
        };

    public:
        // Default constructor
        ComponentsTree(RefCounter* refCounter);

        // Copy-constructor
        ComponentsTree(RefCounter* refCounter, const ComponentsTree& other);

        // Copy-constructor
        ComponentsTree(const ComponentsTree& other);

        // Copy operator
        ComponentsTree& operator=(const ComponentsTree& other);

        // Refreshes components list
        void Refresh();

        // Sets filter and refreshes tree
        void SetFilter(const WString& filter);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ComponentsTree);
        CLONEABLE_REF(ComponentsTree);

    private:
        WString  mFilterStr; // Filtering string
        Ref<NodeData> mRoot; // Root properties data node

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

        void OnDeserialized(const DataValue& node) override;

        friend class ComponentsTreeNode;
    };

    // --------------------------------------------------------------------------
    // Components tree node. Can bea  group of components or the component itself
    // --------------------------------------------------------------------------
    class ComponentsTreeNode : public TreeNode
    {
    public:
        Ref<ComponentsTree::NodeData> data;

    public:
        // Default constructor
        ComponentsTreeNode(RefCounter* refCounter);

        // Copy-constructor
        ComponentsTreeNode(RefCounter* refCounter, const ComponentsTreeNode& other);

        // Copy operator
        ComponentsTreeNode& operator=(const ComponentsTreeNode& other);

        // Initializes node by data
        void Setup(const Ref<ComponentsTree::NodeData>& data, const Ref<ComponentsTree>& tree);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ComponentsTreeNode);
        CLONEABLE_REF(ComponentsTreeNode);

    private:
        Ref<Text>   mName;
        Ref<Sprite> mIcon;

        WeakRef<ComponentsTree> mTree;

    private:
        // Called on deserialization, initializes controls
        void OnDeserialized(const DataValue& node) override;

        // initializes controls and widgets
        void InitializeControls();

        friend class ComponentsTree;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AddComponentPanel)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorEventsArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AddComponentPanel)
{
    FIELD().PRIVATE().NAME(mViewer);
    FIELD().PRIVATE().NAME(mFilterBox);
    FIELD().PRIVATE().NAME(mAddButton);
    FIELD().PRIVATE().NAME(mTree);
}
END_META;
CLASS_METHODS_META(Editor::AddComponentPanel)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<ActorViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<EditBox>&, GetFilter);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ComponentsTree>&, GetTree);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<AddComponentPanel>&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnAddPressed);
    FUNCTION().PRIVATE().SIGNATURE(void, CreateComponent, const ObjectType*);
    FUNCTION().PRIVATE().SIGNATURE(void, OnNodeDblClick, const Ref<TreeNode>&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnKeyReleased, const Input::Key&);
}
END_META;

CLASS_BASES_META(Editor::ComponentsTree)
{
    BASE_CLASS(o2::Tree);
}
END_META;
CLASS_FIELDS_META(Editor::ComponentsTree)
{
    FIELD().PRIVATE().NAME(mFilterStr);
    FIELD().PRIVATE().NAME(mRoot);
}
END_META;
CLASS_METHODS_META(Editor::ComponentsTree)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ComponentsTree&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const ComponentsTree&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFilter, const WString&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateVisibleNodes);
    FUNCTION().PRIVATE().SIGNATURE(Ref<TreeNode>, CreateTreeNodeWidget);
    FUNCTION().PRIVATE().SIGNATURE(void*, GetObjectParent, void*);
    FUNCTION().PRIVATE().SIGNATURE(Vector<void*>, GetObjectChilds, void*);
    FUNCTION().PRIVATE().SIGNATURE(String, GetObjectDebug, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, FillNodeDataByObject, const Ref<TreeNode>&, void*);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(Editor::ComponentsTreeNode)
{
    BASE_CLASS(o2::TreeNode);
}
END_META;
CLASS_FIELDS_META(Editor::ComponentsTreeNode)
{
    FIELD().PUBLIC().NAME(data);
    FIELD().PRIVATE().NAME(mName);
    FIELD().PRIVATE().NAME(mIcon);
    FIELD().PRIVATE().NAME(mTree);
}
END_META;
CLASS_METHODS_META(Editor::ComponentsTreeNode)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ComponentsTreeNode&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const Ref<ComponentsTree::NodeData>&, const Ref<ComponentsTree>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeControls);
}
END_META;
// --- END META ---
