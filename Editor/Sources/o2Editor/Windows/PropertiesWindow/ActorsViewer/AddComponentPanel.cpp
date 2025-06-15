#include "o2Editor/stdafx.h"
#include "AddComponentPanel.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/ActorViewer.h"

namespace Editor
{
    AddComponentPanel::AddComponentPanel(RefCounter* refCounter, const Ref<ActorViewer>& viewer) :
        Widget(refCounter), mViewer(viewer)
    {
        AddLayer("border", mmake<Sprite>("ui/UI4_shadow_separator.png"), Layout::HorStretch(VerAlign::Top, -2, -2, 5, -4));

        mFilterBox = o2UI.CreateEditBox("actor head name");
        mAddButton = o2UI.CreateWidget<Button>("add component");
        mTree = o2UI.CreateWidget<ComponentsTree>();

        *mFilterBox->layout = WidgetLayout::HorStretch(VerAlign::Top, 10, 40, 20, 10);
        *mAddButton->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(40, 40), Vec2F());
        *mTree->layout = WidgetLayout::BothStretch(0, 0, 0, 40);

        AddChildren({ mFilterBox, mAddButton, mTree });

        mFilterBox->onChanged = [&](const WString& filter) { mTree->SetFilter(filter); };
        mAddButton->onClick = THIS_FUNC(OnAddPressed);

        mTree->onNodeDoubleClicked = THIS_FUNC(OnNodeDblClick);
        mTree->Refresh();
    }

    AddComponentPanel::AddComponentPanel(RefCounter* refCounter):
        Widget(refCounter)
    {}

    void AddComponentPanel::Draw()
    {
        Widget::Draw();
        CursorEventsArea::OnDrawn();
    }

    const Ref<EditBox>& AddComponentPanel::GetFilter() const
    {
        return mFilterBox;
    }

    const Ref<ComponentsTree>& AddComponentPanel::GetTree() const
    {
        return mTree;
    }

    bool AddComponentPanel::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    bool AddComponentPanel::IsInputTransparent() const
    {
        return true;
    }

    String AddComponentPanel::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    Ref<RefCounterable> AddComponentPanel::CastToRefCounterable(const Ref<AddComponentPanel>& ref)
    {
        return DynamicCast<Widget>(ref);
    }

    void AddComponentPanel::OnAddPressed()
    {
        if (mTree->GetSelectedObjects().IsEmpty())
            return;

        ComponentsTree::NodeData* node = (ComponentsTree::NodeData*)(mTree->GetSelectedObjects().Last());

        if (!node->type)
            return;

        const ObjectType* objType = dynamic_cast<const ObjectType*>(node->type);
        CreateComponent(objType);

        onCursorPressedOutside(*o2Input.GetCursor());
    }

    void AddComponentPanel::CreateComponent(const ObjectType* objType)
    {
        auto viewer = mViewer.Lock();

        for (auto& actor : viewer->mTargetActors)
        {
            auto comp = DynamicCast<Component>(objType->CreateSampleRef());
            actor->AddComponent(comp);
            comp->OnAddedFromEditor();
        }

        viewer->SetTargets(viewer->mTargetActors.Convert<IObject*>([](auto x) { return dynamic_cast<IObject*>(x); }));
        viewer->OnPropertiesEnabled();
    }

    void AddComponentPanel::OnNodeDblClick(const Ref<TreeNode>& nodeWidget)
    {
        if (!nodeWidget)
            return;

        auto node = DynamicCast<ComponentsTreeNode>(nodeWidget);
        if (!node->data->type)
            return;

        const ObjectType* objType = dynamic_cast<const ObjectType*>(node->data->type);
        CreateComponent(objType);
    }

    void AddComponentPanel::OnKeyReleased(const Input::Key& key)
    {
        if (key.keyCode != VK_RETURN)
            return; 

        if (mFilterBox->IsFocused() || mTree->IsFocused())
            OnAddPressed();
    }

    ComponentsTree::ComponentsTree(RefCounter* refCounter) :
        Tree(refCounter)
    {}

    ComponentsTree::ComponentsTree(RefCounter* refCounter, const ComponentsTree& other) :
        Tree(refCounter, other)
    {}

    ComponentsTree::ComponentsTree(const ComponentsTree& other):
        ComponentsTree(nullptr, other)
    {}

    ComponentsTree& ComponentsTree::operator=(const ComponentsTree& other)
    {
        Tree::operator=(other);
        return *this;
    }

    void ComponentsTree::Refresh()
    {
        mRoot = mmake<NodeData>();

        auto componentsTypes = TypeOf(Component).GetDerivedTypes();
        componentsTypes.Remove(&TypeOf(Component));

        String filterStrLower = mFilterStr.ToLowerCase();

        for (auto& type : componentsTypes)
        {
            if (!type->InvokeStatic<bool>("IsAvailableFromCreateMenu"))
                continue;

            String name = type->InvokeStatic<String>("GetName");
            if (name.IsEmpty())
                name = type->GetName();

            if (!filterStrLower.IsEmpty())
            {
                if (name.ToLowerCase().Contains(filterStrLower))
                    mRoot->AddChild(name, type)->icon = type->InvokeStatic<String>("GetIcon");
            }
            else
            {
                String category = type->InvokeStatic<String>("GetCategory");
                auto it = mRoot;
                while (!category.IsEmpty())
                {
                    int fnd = category.Find('/');
                    String subStr = category.SubStr(0, fnd);
                    category = fnd != -1 ? category.SubStr(fnd + 1) : String("");

                    auto nextIt = it->children.FindOrDefault([&](auto& x) { return x->name == subStr; });
                    if (!nextIt)
                        nextIt = it->AddChild(subStr, nullptr);

                    it = nextIt;
                }

                it->AddChild(name, type)->icon = type->InvokeStatic<String>("GetIcon");
            }
        }

        UpdateNodesStructure();
        UpdateVisibleNodes();
    }

    void ComponentsTree::SetFilter(const WString& filter)
    {
        mFilterStr = filter;
        Refresh();

        String lowerFilter = mFilterStr.ToLowerCase();
        int minDist = INT_MAX;
        Ref<NodeData> filtered;
        Function<void(const Ref<NodeData>&)> recursiveSearch = 
            [&lowerFilter, &recursiveSearch, &minDist, &filtered](const Ref<NodeData>& node)
        {
            if (node->type != nullptr)
            {
                int dist = node->name.ToLowerCase().Find(lowerFilter);
                if (dist >= 0 && dist < minDist)
                {
                    minDist = dist;
                    filtered = node;
                }
            }

            for (auto& child : node->children)
                recursiveSearch(child);
        };

        recursiveSearch(mRoot);

        if (filtered)
            SelectAndHighlightObject(filtered.Get());
    }

    String ComponentsTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ComponentsTree::UpdateVisibleNodes()
    {
        PushEditorScopeOnStack scope;
        Tree::UpdateVisibleNodes();
    }

    Ref<TreeNode> ComponentsTree::CreateTreeNodeWidget()
    {
        PushEditorScopeOnStack scope;
        return Tree::CreateTreeNodeWidget();
    }

    void* ComponentsTree::GetObjectParent(void* object)
    {
        auto node = (NodeData*)object;
        if (node->parent == mRoot.Get())
            return (NodeData*)nullptr;

        return node->parent.Lock().Get();
    }

    Vector<void*> ComponentsTree::GetObjectChilds(void* object)
    {
        if (object)
            return ((NodeData*)object)->children.Convert<void*>([](auto& x) { return x.Get(); });

        return mRoot->children.Convert<void*>([](auto& x) { return x.Get(); });
    }

    String ComponentsTree::GetObjectDebug(void* object)
    {
        if (object)
            return ((NodeData*)object)->path;

        return "";
    }

    void ComponentsTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        auto propertyNode = DynamicCast<ComponentsTreeNode>(nodeWidget);
        propertyNode->Setup(Ref((NodeData*)object), Ref(this));
    }

    void ComponentsTree::OnDeserialized(const DataValue& node)
    {
        Tree::OnDeserialized(node);
    }

    ComponentsTreeNode::ComponentsTreeNode(RefCounter* refCounter):
        TreeNode(refCounter)
    {
        InitializeControls();
    }

    ComponentsTreeNode::ComponentsTreeNode(RefCounter* refCounter, const ComponentsTreeNode& other) :
        TreeNode(refCounter, other)
    {
        InitializeControls();
    }

    ComponentsTreeNode& ComponentsTreeNode::operator=(const ComponentsTreeNode& other)
    {
        TreeNode::operator=(other);
        InitializeControls();
        return *this;
    }

    void ComponentsTreeNode::Setup(const Ref<ComponentsTree::NodeData>& data, const Ref<ComponentsTree>& tree)
    {
        mName->text = data->name;

        *mIcon = Sprite(data->icon);
        mIcon->enabled = data->type != nullptr;
        mIcon->color = Color4(44, 62, 80);

        this->data = data;
        mTree = tree;
    }

    String ComponentsTreeNode::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ComponentsTreeNode::OnDeserialized(const DataValue& node)
    {
        TreeNode::OnDeserialized(node);
        InitializeControls();
    }

    void ComponentsTreeNode::InitializeControls()
    {
        mName = GetLayerDrawable<Text>("name");
        mIcon = GetLayerDrawable<Sprite>("icon");
    }

    ComponentsTree::NodeData::~NodeData()
    {
        Clear();
    }

    void ComponentsTree::NodeData::Clear()
    {
        children.Clear();
    }

    Ref<ComponentsTree::NodeData> ComponentsTree::NodeData::AddChild(const String& name, const Type* type)
    {
        auto newChild = mmake<NodeData>();
        children.Add(newChild);

        newChild->name = name;
        newChild->path = path.IsEmpty() ? name : path + "/" + name;
        newChild->type = type;
        newChild->parent = Ref(this);

        return newChild;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ComponentsTreeNode>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AddComponentPanel>);
// --- META ---

DECLARE_CLASS(Editor::AddComponentPanel, Editor__AddComponentPanel);

DECLARE_CLASS(Editor::ComponentsTree, Editor__ComponentsTree);

DECLARE_CLASS(Editor::ComponentsTreeNode, Editor__ComponentsTreeNode);
// --- END META ---
