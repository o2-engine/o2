#include "o2Editor/stdafx.h"
#include "MemoryAnalyzerWindow.h"

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/Attributes/AnimatableAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::MemoryAnalyzerWindow);

namespace Editor
{
    MemoryAnalyzerWindow::MemoryAnalyzerWindow(RefCounter* refCounter) :
        Singleton<MemoryAnalyzerWindow>(refCounter)
    {
        InitializeWindow();
    }

    MemoryAnalyzerWindow::~MemoryAnalyzerWindow()
    {}

    void MemoryAnalyzerWindow::Show(MemoryAnalyzer::MemoryNode* data)
    {
        auto& self = Instance();

        if (self.mData)
            delete self.mData;

        self.mData = data;
        self.mMemoryTree->Initialize(data, false);
        self.mWindow->ShowModal();
    }

    void MemoryAnalyzerWindow::InitializeWindow()
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Memory analyzer")));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, -2, 0, 19));
        mWindow->SetViewLayout(Layout::BothStretch(-2, -2, 0, 20));
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_tree_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));

        InitializeTree();
        InitializeInspector();
        InitializeSeparatorHandle();

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(1500, 900);
    }

    void MemoryAnalyzerWindow::InitializeTree()
    {
        mTreeWidget = mmake<Widget>();
        mTreeWidget->name = "tree widget";
        *mTreeWidget->layout = WidgetLayout::VerStretch(HorAlign::Left, 0, 0, mTreeViewWidth, 0);
        mWindow->AddChild(mTreeWidget);

        auto upPanel = mmake<Widget>();
        upPanel->name = "up panel";
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, -1);
        upPanel->AddLayer("back", mmake<Sprite>("ui/UI4_square_field.png"), Layout::BothStretch(-4, -4, -5, -5));

        auto searchButton = o2UI.CreateWidget<Button>("search");
        *searchButton->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(-1, 1));
        upPanel->AddChild(searchButton);

        mFilter = o2UI.CreateWidget<EditBox>("backless");
        *mFilter->layout = WidgetLayout::BothStretch(19, 2, 0, -4);
        mFilter->onChanged += [&](const WString& filter) {};
        upPanel->AddChild(mFilter);

        mTreeWidget->AddChild(upPanel);

        mMemoryTree = o2UI.CreateWidget<MemoryAnalyzeTree>();
        *mMemoryTree->layout = WidgetLayout::BothStretch(0, 0, 0, 17);
        mMemoryTree->SetRearrangeType(Tree::RearrangeType::Disabled);
        mMemoryTree->onSelectedNode = [&](MemoryAnalyzer::MemoryNode* data) { UpdateInspector(data); };

        mTreeWidget->AddChild(mMemoryTree);
    }

    void MemoryAnalyzerWindow::InitializeInspector()
    {
        mInspectorWidget = mmake<Widget>();
        mInspectorWidget->name = "inspector widget";
        *mInspectorWidget->layout = WidgetLayout::BothStretch(mTreeViewWidth, 0, 0, 0);
        mWindow->AddChild(mInspectorWidget);

        auto container = mmake<Widget>();
        container->name = "container";
        *container->layout = WidgetLayout::BothStretch(5, 5, 5, 5);
        mInspectorWidget->AddChild(container);

        auto upPanel = mmake<HorizontalLayout>();
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        upPanel->expandHeight = true;
        upPanel->expandWidth = true;

        mObjectLabel = o2UI.CreateLabel("Object");
        mObjectLabel->horAlign = HorAlign::Left;
        upPanel->AddChild(mObjectLabel);

        mAddressLabel = o2UI.CreateLabel("Address");
        mAddressLabel->layout->maxWidth = 170;
        upPanel->AddChild(mAddressLabel);

        mSizeLabel = o2UI.CreateLabel("Size");
        mSizeLabel->layout->maxWidth = 100;
        upPanel->AddChild(mSizeLabel);

        mReverseTreeButton = o2UI.CreateButton("Reverse tree");
        mReverseTreeButton->layout->maxWidth = 100;
        mReverseTreeButton->onClick = [&]() { OnReversePressed(); };
        upPanel->AddChild(mReverseTreeButton);

        container->AddChild(upPanel);

        auto inspectorScroll = o2UI.CreateWidget<ScrollArea>("backless");
        *inspectorScroll->layout = WidgetLayout::BothStretch(0, 0, -5, 20);
        container->AddChild(inspectorScroll);

        auto inspectorLayout = mmake<VerticalLayout>();
        *inspectorLayout->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        inspectorLayout->expandHeight = false;
        inspectorLayout->expandWidth = true;
        inspectorLayout->fitByChildren = true;
        inspectorLayout->baseCorner = BaseCorner::LeftTop;
        inspectorLayout->spacing = 5;
        inspectorScroll->AddChild(inspectorLayout);

        auto stackSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
        stackSpoiler->name = "Stack spoiler";
        stackSpoiler->caption = "Stack";
        *stackSpoiler->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        stackSpoiler->expandHeight = false;
        stackSpoiler->expandWidth = true;
        stackSpoiler->fitByChildren = true;
        stackSpoiler->baseCorner = BaseCorner::LeftTop;
        stackSpoiler->Expand();
        inspectorLayout->AddChild(stackSpoiler);

        mStackText = o2UI.CreateWidget<EditBox>("backless multiline");
        *mStackText->layout = WidgetLayout::BothStretch();
        mStackText->layout->minHeight = 500;
        stackSpoiler->AddChild(mStackText);

        mInspectorContainer = mmake<VerticalLayout>();
        *mInspectorContainer->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        mInspectorContainer->expandHeight = false;
        mInspectorContainer->expandWidth = true;
        mInspectorContainer->fitByChildren = true;
        mInspectorContainer->baseCorner = BaseCorner::LeftTop;
        mInspectorContainer->borderRight = 15;
        inspectorLayout->AddChild(mInspectorContainer);
    }

    void MemoryAnalyzerWindow::InitializeSeparatorHandle()
    {
        mTreeSeparatorHandle = mmake<WidgetDragHandle>(mmake<Sprite>("ui/UI4_Ver_separator.png"));
        mTreeSeparatorHandle->GetRegularDrawable()->pivot = Vec2F(0.5f, 0.5f);
        mTreeSeparatorHandle->GetRegularDrawable()->szPivot = Vec2F(4, mTreeSeparatorHandle->GetRegularDrawable()->szPivot.Get().y);

        mTreeSeparatorHandle->onChangedPos = [&](const Vec2F& point) {
            mTreeViewWidth = Math::Max(point.x, mMinTreeViewWidth);
            mTreeWidget->layout->right = mTreeViewWidth;
            mInspectorWidget->layout->left = mTreeViewWidth;
            };

        mTreeSeparatorHandle->checkPositionFunc = [&](const Vec2F& point) {
            return Vec2F(Math::Max(point.x, mMinTreeViewWidth), mWindow->layout->GetHeight()*0.5f);
            };

        mTreeSeparatorHandle->onLayoutUpdated = [&]() {
            mTreeSeparatorHandle->SetDrawablesSize(Vec2F(5.0f, mWindow->layout->GetHeight() + 50.0f));
            mTreeSeparatorHandle->SetPosition(Vec2F(mTreeViewWidth, mWindow->layout->GetHeight()*0.5f));
            };

        mTreeSeparatorHandle->cursorType = CursorType::SizeWE;

        mWindow->AddChild(mTreeSeparatorHandle);
    }

    String GetNodeName(MemoryAnalyzer::MemoryNode* node)
    {
        String result;

        String type = node->type;
        type.ReplaceAll("class ", "");
        type.ReplaceAll("> >", ">>");

        auto& name = node->name;

        if (name.empty())
            result = type;
        else if (type.IsEmpty())
            result = (String)name;
        else
            result = type + " " + (String)name;

//         if (node->object)
//             result += String("(") + (String)node->object->createIndex + String(")");

        return result;
    }

    String GetNodeAddress(MemoryAnalyzer::MemoryNode* node)
    {
        return String::Format("%p", node->memory);
    }

    String GetNodeSize(MemoryAnalyzer::MemoryNode* node)
    {
        String size;

        auto summarySize = node->leakedSize != 0 ? node->leakedSize : node->summarySize;
        if (summarySize > 1024 * 1024)
            size = String::Format("%.2f Mb", summarySize / (1024.0 * 1024.0));
        else if (summarySize > 1024)
            size = String::Format("%.2f Kb", summarySize / 1024.0);
        else
            size = String::Format("%d b", summarySize);

        return size;
    }

    Color4 GetNodeColor(MemoryAnalyzer::MemoryNode* node)
    {
        if (node->leakedSize > 0)
            return Color4(235, 125, 139);
        else
            return Color4(96, 125, 139);
    }

    void MemoryAnalyzerWindow::UpdateInspector(MemoryAnalyzer::MemoryNode* node)
    {
        mSelected = node;

        mObjectLabel->text = GetNodeName(node);
        mObjectLabel->color = GetNodeColor(node);

        mAddressLabel->text = GetNodeAddress(node);
        mSizeLabel->text = GetNodeSize(node);

        if (node->object)
            mStackText->text = (String)boost::stacktrace::to_string(node->object->trace);

        if (mObjectPropertiesViewer)
            o2EditorProperties.FreeObjectViewer(mObjectPropertiesViewer);

        if (node->iobject)
        {
            bool prevPrivateVisible = o2EditorProperties.IsPrivateFieldsVisible();

            o2EditorProperties.SetPrivateFieldsVisible(true);
            mObjectPropertiesViewer = o2EditorProperties.CreateObjectViewer(&node->iobject->GetType(), "");
            mObjectPropertiesViewer->Refresh({ { node->iobject, nullptr } });

            auto viewerWidget = mObjectPropertiesViewer->GetSpoiler();
            *viewerWidget->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
            viewerWidget->caption = "Properties";
            viewerWidget->Expand();

            mInspectorContainer->AddChild(viewerWidget);

            o2EditorProperties.SetPrivateFieldsVisible(prevPrivateVisible);
        }
    }

    void MemoryAnalyzerWindow::OnReversePressed()
    {
        if (mMemoryTree->IsTreeReversed())
            mMemoryTree->Initialize(mData, false);
        else
            mMemoryTree->Initialize(mSelected, true);
    }

    MemoryAnalyzeTree::MemoryAnalyzeTree(RefCounter* refCounter) :
        Tree(refCounter)
    {}

    MemoryAnalyzeTree::MemoryAnalyzeTree(RefCounter* refCounter, const MemoryAnalyzeTree& other) :
        Tree(refCounter, other)
    {}

    MemoryAnalyzeTree::MemoryAnalyzeTree(const MemoryAnalyzeTree& other) :
        MemoryAnalyzeTree(nullptr, other)
    {}

    MemoryAnalyzeTree& MemoryAnalyzeTree::operator=(const MemoryAnalyzeTree& other)
    {
        Tree::operator=(other);
        return *this;
    }

    void MemoryAnalyzeTree::Initialize(MemoryAnalyzer::MemoryNode* data, bool reversed)
    {
        mFilterStr = "";
        mRoot = data;
        mReversed = reversed;

        UpdateNodesStructure();
        UpdateVisibleNodes();
    }

    bool MemoryAnalyzeTree::IsTreeReversed() const
    {
        return mReversed;
    }

    String MemoryAnalyzeTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void MemoryAnalyzeTree::UpdateVisibleNodes()
    {
        PushEditorScopeOnStack scope;
        Tree::UpdateVisibleNodes();
    }

    Ref<TreeNode> MemoryAnalyzeTree::CreateTreeNodeWidget()
    {
        PushEditorScopeOnStack scope;
        return Tree::CreateTreeNodeWidget();
    }

    void* MemoryAnalyzeTree::GetObjectParent(void* object)
    {
        if (mReversed)
            return ((MemoryAnalyzer::MemoryNode*)object)->children[0];
        else
            return ((MemoryAnalyzer::MemoryNode*)object)->mainParent;
    }

    Vector<void*> MemoryAnalyzeTree::GetObjectChilds(void* object)
    {
        Vector<void*> result;
        if (object)
        {
            if (!mReversed)
            {
                for (auto& child : ((MemoryAnalyzer::MemoryNode*)object)->children)
                    result.push_back(child);
            }
            else
            {
                for (auto& parent : ((MemoryAnalyzer::MemoryNode*)object)->parents)
                    result.push_back(parent);
            }
        }
        else
        {
            if (!mReversed)
            {
                for (auto& child : mRoot->children)
                    result.push_back(child);
            }
            else
            {
                for (auto& parent : mRoot->parents)
                    result.push_back(parent);
            }
        }

        return result;
    }

    String MemoryAnalyzeTree::GetObjectDebug(void* object)
    {
        return "unknown";
    }

    void MemoryAnalyzeTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        auto propertyNode = DynamicCast<MemoryAnalyzeTreeNode>(nodeWidget);
        propertyNode->Setup((MemoryAnalyzer::MemoryNode*)object, Ref(this));
    }

    void MemoryAnalyzeTree::OnNodesSelectionChanged(Vector<void*> objects)
    {
        onSelectedNode((MemoryAnalyzer::MemoryNode*)objects[0]);
    }

    MemoryAnalyzeTreeNode::MemoryAnalyzeTreeNode(RefCounter* refCounter) :
        TreeNode(refCounter)
    {
        InitializeControls();
    }

    MemoryAnalyzeTreeNode::MemoryAnalyzeTreeNode(RefCounter* refCounter, const MemoryAnalyzeTreeNode& other) :
        TreeNode(refCounter, other)
    {
        InitializeControls();
    }

    MemoryAnalyzeTreeNode& MemoryAnalyzeTreeNode::operator=(const MemoryAnalyzeTreeNode& other)
    {
        TreeNode::operator=(other);
        InitializeControls();
        return *this;
    }

    void MemoryAnalyzeTreeNode::Setup(MemoryAnalyzer::MemoryNode* data, const Ref<MemoryAnalyzeTree>& tree)
    {
        mName->text = GetNodeName(data);
        mName->color = GetNodeColor(data);

        mAddress->text = GetNodeAddress(data);
        mSize->text = GetNodeSize(data);

        mData = data;
        mTree = tree;
    }

    String MemoryAnalyzeTreeNode::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void MemoryAnalyzeTreeNode::OnDeserialized(const DataValue& node)
    {
        TreeNode::OnDeserialized(node);
        InitializeControls();
    }

    void MemoryAnalyzeTreeNode::InitializeControls()
    {
        mName = GetLayerDrawable<Text>("root/name");
        mAddress = GetLayerDrawable<Text>("root/address");
        mSize = GetLayerDrawable<Text>("root/size");

        auto tree = mTree.Lock();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::MemoryAnalyzeTreeNode>);
// --- META ---

DECLARE_CLASS(Editor::MemoryAnalyzeTree, Editor__MemoryAnalyzeTree);

DECLARE_CLASS(Editor::MemoryAnalyzeTreeNode, Editor__MemoryAnalyzeTreeNode);
// --- END META ---
