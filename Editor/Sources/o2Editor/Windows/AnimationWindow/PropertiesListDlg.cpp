#include "o2Editor/stdafx.h"
#include "PropertiesListDlg.h"

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/Attributes/AnimatableAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::PropertiesListDlg);

namespace Editor
{    
    PropertiesListDlg::PropertiesListDlg(RefCounter* refCounter):
        Singleton<PropertiesListDlg>(refCounter)
    {
        InitializeWindow();
    }

    PropertiesListDlg::~PropertiesListDlg()
    {}

    void PropertiesListDlg::Show(const Ref<AnimationClip>& animation, const Ref<Actor>& actor)
    {
        Instance().mPropertiesTree->Initialize(animation, actor);
        Instance().mFilter->SetText("");
        Instance().mWindow->ShowModal();
    }

    void PropertiesListDlg::InitializeWindow()
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Animation properties")));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, -2, 0, 17));
        mWindow->SetViewLayout(Layout::BothStretch(-2, -2, 0, 20));
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_tree_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));

        auto upPanel = mmake<Widget>();
        upPanel->name = "up panel";
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, -1);
        upPanel->AddLayer("back", mmake<Sprite>("ui/UI4_square_field.png"), Layout::BothStretch(-4, -4, -5, -5));

        auto searchButton = o2UI.CreateWidget<Button>("search");
        *searchButton->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(-1, 1));
        upPanel->AddChild(searchButton);

        mFilter = o2UI.CreateWidget<EditBox>("backless");
        *mFilter->layout = WidgetLayout::BothStretch(19, 2, 0, -4);
        mFilter->onChanged += [&](const WString& filter) { mPropertiesTree->SetFilter(filter); };
        upPanel->AddChild(mFilter);

        mWindow->AddChild(upPanel);

        mPropertiesTree = o2UI.CreateWidget<AnimationPropertiesTree>();
        *mPropertiesTree->layout = WidgetLayout::BothStretch(0, 0, 0, 17);
        mPropertiesTree->SetRearrangeType(Tree::RearrangeType::Disabled);

        mWindow->AddChild(mPropertiesTree);

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(400, 600);

        mWindow->GetBackCursorListener().onCursorReleased = [=](const Input::Cursor& c) { mWindow->Hide(); };
    }

    AnimationPropertiesTree::AnimationPropertiesTree(RefCounter* refCounter) :
        Tree(refCounter)
    {}

    AnimationPropertiesTree::AnimationPropertiesTree(RefCounter* refCounter, const AnimationPropertiesTree& other):
        Tree(refCounter, other)
    {}

    AnimationPropertiesTree::AnimationPropertiesTree(const AnimationPropertiesTree& other):
        AnimationPropertiesTree(nullptr, other)
    {}

    AnimationPropertiesTree& AnimationPropertiesTree::operator=(const AnimationPropertiesTree& other)
    {
        Tree::operator=(other);
        return *this;
    }

    void AnimationPropertiesTree::Initialize(const Ref<AnimationClip>& animation, const Ref<Actor>& actor)
    {
        mFilterStr = "";
        mRoot = mmake<NodeData>();
        mPassedObject.Clear();

        mAnimation = animation;
        mActor = actor;

        InitializeTreeNode(mRoot, actor.Get());
        UpdateNodesStructure();
        UpdateVisibleNodes();
    }

    void AnimationPropertiesTree::SetFilter(const WString& filter)
    {
        mFilterStr = filter.ToLowerCase();

        mRoot = mmake<NodeData>();
        mPassedObject.Clear();

        InitializeTreeNode(mRoot, mActor.Get());
        UpdateNodesStructure();
        UpdateVisibleNodes();
    }

    String AnimationPropertiesTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void AnimationPropertiesTree::InitializeTreeNode(const Ref<NodeData>& node, IObject* object)
    {
        if (!object)
            return;

        if (mPassedObject.Contains(object))
            return;

        mPassedObject.Add(object);

        auto objectType = dynamic_cast<const ObjectType*>(&object->GetType());
        auto rawObject = objectType->DynamicCastFromIObject(object);
        ProcessObject(rawObject, objectType, node);
    }

    void AnimationPropertiesTree::ProcessObject(void* object, const ObjectType* type, const Ref<NodeData>& node)
    {
        for (auto& field : type->GetFields())
        {
            bool isPublic = field.GetProtectionSection() == ProtectSection::Public;
            bool isEditorProperty = field.HasAttribute<EditorPropertyAttribute>();
            bool isIgnoreEditorProperty = field.HasAttribute<IgnoreEditorPropertyAttribute>();
            bool isAnimatable = field.HasAttribute<AnimatableAttribute>();

            if (isAnimatable || (isPublic && !isIgnoreEditorProperty) || isEditorProperty) 
                ProcessTreeNode(field.GetValuePtr(object), field.GetType(), field.GetName(), node);
        }

        for (auto& base : type->GetBaseTypes())
        {
            if (base.type->GetUsage() != Type::Usage::Object)
                continue;

            ProcessObject(base.dynamicCastUpFunc(object), dynamic_cast<const ObjectType*>(base.type), node);
        }
    }

    void AnimationPropertiesTree::ProcessTreeNode(void* object, const Type* type, const String& name, const Ref<NodeData>& node)
    {
        static Vector<const Type*> availableTypes({ &TypeOf(float), &TypeOf(Color4), &TypeOf(Vec2F), &TypeOf(bool) });

        if (type->GetUsage() == Type::Usage::Object && object)
        {
            auto fieldObjectType = dynamic_cast<const ObjectType*>(type);
            InitializeObjectTreeNode(fieldObjectType, object, name, node);
        }
        else if (type->GetUsage() == Type::Usage::Pointer && object)
        {
            auto pointerType = dynamic_cast<const PointerType*>(type);
            auto unpointedType = pointerType->GetBaseType();
            if (unpointedType->GetUsage() == Type::Usage::Object)
            {
                auto fieldObjectType = dynamic_cast<const ObjectType*>(unpointedType);
                InitializeObjectTreeNode(fieldObjectType, object, name, node);
            }
        }
        else if (type->GetUsage() == Type::Usage::Reference && object)
        {
            auto referenceType = dynamic_cast<const ReferenceType*>(type);
            auto unpointedType = referenceType->GetBaseType();
            if (unpointedType->GetUsage() == Type::Usage::Object)
            {
                auto fieldObjectType = dynamic_cast<const ObjectType*>(unpointedType);
                InitializeObjectTreeNode(fieldObjectType, referenceType->GetObjectRawPtr(object), name, node);
            }
        }
        else if (type->GetUsage() == Type::Usage::Property && object)
        {
            auto propertyType = dynamic_cast<const PropertyType*>(type);
            ProcessTreeNode(propertyType->GetValueAsPtr(object), propertyType->GetValueType(), name, node);
        }
        else if (type->GetUsage() == Type::Usage::StringAccessor && object)
        {
            auto accessorType = dynamic_cast<const StringPointerAccessorType*>(type);
            auto newNode = node->AddChild(name, type);
            auto allValues = accessorType->GetAllValues(object);

            auto valueType = accessorType->GetReturnType();
            if (valueType->GetUsage() == Type::Usage::Reference)
                valueType = dynamic_cast<const ReferenceType*>(valueType)->GetBaseType();

            for (auto& kv : allValues)
                ProcessTreeNode(kv.second, valueType, kv.first, newNode);

            if (newNode->children.empty()) {
                node->children.Remove(newNode);
            }
        }
        else if (availableTypes.Contains(type))
        {
            InitializePropertyNode(node, name, type);
        }
    }

    void AnimationPropertiesTree::InitializePropertyNode(const Ref<NodeData>& node, const String& name, const Type* type)
    {
        if (!mFilterStr.IsEmpty() && !name.ToLowerCase().Contains(mFilterStr))
            return;

        auto newNode = node->AddChild(name, type);
        newNode->used = mAnimation->ContainsTrack(newNode->path);
    }

    void AnimationPropertiesTree::InitializeObjectTreeNode(const ObjectType* fieldObjectType, void* object,
                                                           const String& name, const Ref<NodeData>& node)
    {
        auto fieldObject = fieldObjectType->DynamicCastToIObject(object);
        auto& realType = fieldObject->GetType();
        auto newNode = node->AddChild(name, &realType);
        InitializeTreeNode(newNode, fieldObject);

        if (newNode->children.IsEmpty())
            node->children.Remove(newNode);
    }

    void AnimationPropertiesTree::UpdateVisibleNodes()
    {
        PushEditorScopeOnStack scope;
        Tree::UpdateVisibleNodes();
    }

    Ref<TreeNode> AnimationPropertiesTree::CreateTreeNodeWidget()
    {
        PushEditorScopeOnStack scope;
        return Tree::CreateTreeNodeWidget();
    }

    void* AnimationPropertiesTree::GetObjectParent(void* object)
    {
        return ((NodeData*)object)->parent.Lock().Get();
    }

    Vector<void*> AnimationPropertiesTree::GetObjectChilds(void* object)
    {
        if (object)
            return ((NodeData*)object)->children.Convert<void*>([](auto& x) { return x.Get(); });

        return mRoot->children.Convert<void*>([](auto& x) { return x.Get(); });
    }

    String AnimationPropertiesTree::GetObjectDebug(void* object)
    {
        if (object)
            return ((NodeData*)object)->path;

        return "";
    }

    void AnimationPropertiesTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        auto propertyNode = DynamicCast<AnimationPropertiesTreeNode>(nodeWidget);
        propertyNode->Setup(Ref((NodeData*)object), Ref(this));
    }

    void AnimationPropertiesTree::OnNodeDblClick(const Ref<TreeNode>& nodeWidget)
    {
        if (!nodeWidget)
            return;

        auto propertyNode = DynamicCast<AnimationPropertiesTreeNode>(nodeWidget);
        auto propertyData = propertyNode->mData.Lock();
        if (propertyData->used)
            mAnimation->RemoveTrack(propertyData->path);
        else
            mAnimation->AddTrack(propertyData->path, *propertyData->type);
    }

    void AnimationPropertiesTree::OnNodesSelectionChanged(Vector<void*> objects)
    {

    }

    AnimationPropertiesTreeNode::AnimationPropertiesTreeNode(RefCounter* refCounter):
        TreeNode(refCounter)
    {
        InitializeControls();
    }

    AnimationPropertiesTreeNode::AnimationPropertiesTreeNode(RefCounter* refCounter, const AnimationPropertiesTreeNode& other) :
        TreeNode(refCounter, other)
    {
        InitializeControls();
    }

    AnimationPropertiesTreeNode& AnimationPropertiesTreeNode::operator=(const AnimationPropertiesTreeNode& other)
    {
        TreeNode::operator=(other);
        InitializeControls();
        return *this;
    }

    void AnimationPropertiesTreeNode::Setup(const Ref<AnimationPropertiesTree::NodeData>& data, const Ref<AnimationPropertiesTree>& tree)
    {
        static Map<const Type*, String> icons = 
        { 
            { (const Type*)&TypeOf(float), "ui/UI4_float_type.png" },
            { (const Type*)&TypeOf(Vec2F), "ui/UI4_vector_type.png" },
            { (const Type*)&TypeOf(Color4), "ui/UI4_color_type.png" },
            { (const Type*)&TypeOf(bool), "ui/UI4_bool_type.png" }
        };

        static String otherIcon = "ui/UI4_other_type.png";
        static String animIcon = "ui/UI4_anim_type.png";

        mName->text = data->name;

        bool ableToAddTrack = data->children.IsEmpty();

        String iconPath;
        if (!icons.TryGetValue(data->type, iconPath))
            iconPath = otherIcon;

        if (data->type->IsBasedOn(TypeOf(IAnimation)))
        {
            ableToAddTrack = true;
            iconPath = animIcon;
        }

        *mIcon = Sprite(iconPath);

        mData = data;
        mTree = tree;

        mAddButton->SetEnabledForcible(!data->used && ableToAddTrack);
        mRemoveButton->SetEnabledForcible(data->used && ableToAddTrack);
    }

    String AnimationPropertiesTreeNode::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void AnimationPropertiesTreeNode::OnDeserialized(const DataValue& node)
    {
        TreeNode::OnDeserialized(node);
        InitializeControls();
    }

    void AnimationPropertiesTreeNode::InitializeControls()
    {
        mName = GetLayerDrawable<Text>("name");
        mIcon = GetLayerDrawable<Sprite>("icon");


        mAddButton = GetChildByType<Button>("addButton");
        if (mAddButton)
        {
            mAddButton->onClick = [=]()
            {
                auto tree = mTree.Lock();
                auto data = mData.Lock();

                tree->mAnimation->AddTrack(data->path, *data->type);
                data->used = true;
                tree->OnObjectsChanged({ (void*)data.Get() });
            };
        }

        mRemoveButton = GetChildByType<Button>("removeButton");
        if (mRemoveButton)
        {
            mRemoveButton->onClick = [=]() {
                auto tree = mTree.Lock();
                auto data = mData.Lock();

                tree->mAnimation->RemoveTrack(data->path);
                data->used = false;
                tree->OnObjectsChanged({ (void*)data.Get() });
            };
        }
    }

    AnimationPropertiesTree::NodeData::~NodeData()
    {
        Clear();
    }

    void AnimationPropertiesTree::NodeData::Clear()
    {
        children.Clear();
    }

    Ref<AnimationPropertiesTree::NodeData> AnimationPropertiesTree::NodeData::AddChild(const String& name, const Type* type)
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

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationPropertiesTreeNode>);
// --- META ---

DECLARE_CLASS(Editor::AnimationPropertiesTree, Editor__AnimationPropertiesTree);

DECLARE_CLASS(Editor::AnimationPropertiesTreeNode, Editor__AnimationPropertiesTreeNode);
// --- END META ---
