#include "o2Editor/stdafx.h"
#include "FoldersTree.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/RenameAsset.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2/Utils/Editor/EditorScope.h"

namespace Editor
{
    AssetsFoldersTree::AssetsFoldersTree(RefCounter* refCounter) :
        Widget(refCounter)
    {
        if (!UIManager::IsSingletonInitialzed())
            return;

        mFoldersTree = o2UI.CreateWidget<Tree>("folders");
        *mFoldersTree->layout = WidgetLayout::BothStretch();

        mFoldersTree->SetRearrangeType(Tree::RearrangeType::OnlyReparent);
        mFoldersTree->SetMultipleSelectionAvailable(false);

        mFoldersTree->getObjectParentDelegate = THIS_FUNC(GetFoldersTreeNodeParent);
        mFoldersTree->getObjectChildrenDelegate = THIS_FUNC(GetFoldersTreeNodeChilds);
        mFoldersTree->fillNodeDataByObjectDelegate = THIS_FUNC(SetupFoldersTreeNode);
        mFoldersTree->onNodeDoubleClicked = THIS_FUNC(OnFoldersTreeNodeDblClick);
        mFoldersTree->onObjectsSelectionChanged = THIS_FUNC(OnFoldersTreeSelect);
        mFoldersTree->onNodeRightButtonClicked = THIS_FUNC(OnFoldersTreeRightClick);
        mFoldersTree->UpdateNodesView();

        AddChild(mFoldersTree);

        InitializeContext();
    }

    AssetsFoldersTree::AssetsFoldersTree(RefCounter* refCounter, const AssetsFoldersTree& other) :
        Widget(refCounter, other)
    {
        mFoldersTree = FindChildByType<Tree>();
        RemoveChild(FindChildByType<ContextMenu>());

        InitializeContext();

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    AssetsFoldersTree::~AssetsFoldersTree()
    {}

    String AssetsFoldersTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    AssetsFoldersTree& AssetsFoldersTree::operator=(const AssetsFoldersTree& other)
    {
        Widget::operator=(other);

        mFoldersTree = FindChildByType<Tree>();
        RemoveChild(FindChildByType<ContextMenu>());

        InitializeContext();

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void AssetsFoldersTree::SelectAndExpandFolder(const String& path)
    {
        mOpengingFolderFromThis = true;

        mFoldersTree->SelectAndHighlightObject(o2Assets.GetAssetsTree().Find(path).Get());
        mCurrentPath = path;

        mOpengingFolderFromThis = false;
    }

    void AssetsFoldersTree::UpdateView()
    {
        mFoldersTree->UpdateNodesView();
    }

    void AssetsFoldersTree::InitializeContext()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        mContextMenu->AddItem("Open", [&]() { OnContextOpenPressed(); });
        mContextMenu->AddItem("Show in folder", [&]() { OnContextShowInExplorerPressed(); });
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("New folder", [&]() { OnContextCreateFolderPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_N, VK_CTRL_CMD}));
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Copy", [&]() { OnContextCopyPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_C, VK_CTRL_CMD}));
        mContextMenu->AddItem("Cut", [&]() { OnContextCutPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_X, VK_CTRL_CMD}));
        mContextMenu->AddItem("Paste", [&]() { OnContextPastePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_V, VK_CTRL_CMD}));
        mContextMenu->AddItem("Delete", [&]() { OnContextDeletePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_DELETE}));

        mFoldersTree->onFocused = [&]() { mContextMenu->SetItemsMaxPriority(); };

        AddChild(mContextMenu);
    }

    void* AssetsFoldersTree::GetFoldersTreeNodeParent(void* object)
    {
        if (!object)
            return nullptr;

        AssetInfo* assetTreeNode = (AssetInfo*)(void*)object;
        return (void*)(assetTreeNode->parent.Lock().Get());
    }

    Vector<void*> AssetsFoldersTree::GetFoldersTreeNodeChilds(void* object)
    {
        AssetInfo* assetTreeNode = (AssetInfo*)object;

        auto& infos = assetTreeNode ? assetTreeNode->GetChildren() : o2Assets.GetAssetsTree().rootAssets;

        return infos
            .FindAll([](const Ref<AssetInfo>& x) { return x->meta->GetAssetType() == &TypeOf(FolderAsset); })
            .Sorted([](const Ref<AssetInfo>& a, const Ref<AssetInfo>& b) { return a->path < b->path; })
            .Convert<void*>([](const Ref<AssetInfo>& x) { return (void*)x.Get(); });
    }

    void AssetsFoldersTree::SetupFoldersTreeNode(const Ref<TreeNode>& node, void* object)
    {
        AssetInfo* assetTreeNode = (AssetInfo*)(void*)object;
        String pathName = o2FileSystem.GetPathWithoutDirectories(assetTreeNode->path);

        node->name = pathName;

        auto nameLayer = node->layer["name"];
        if (nameLayer)
            DynamicCast<Text>(nameLayer->GetDrawable())->text = pathName;
    }

    void AssetsFoldersTree::OnFoldersTreeNodeDblClick(const Ref<TreeNode>& node)
    {
        auto assetTreeNode = Ref((AssetInfo*)(void*)node->GetObject());
        String pathName = o2FileSystem.GetPathWithoutDirectories(assetTreeNode->path);

        node->SetState("edit", true);

        auto editBox = node->GetChildByType<EditBox>("nameEditBox");
        editBox->text = (String)pathName;
        editBox->SelectAll();
        editBox->Widget::Focus();
        editBox->ResetScroll();

        editBox->onChangeCompleted = [=](const WString& text) {
            String originalName = o2FileSystem.GetPathWithoutDirectories(assetTreeNode->path);
            String newName = text;
            String newPathAsset = o2FileSystem.GetParentPath(assetTreeNode->path) + "/" + newName;

            auto action = mmake<RenameAssetAction>(assetTreeNode->meta->ID(), originalName, newName);
            action->Redo();
            o2EditorSceneWindow.DoneAction(action);

            node->SetState("edit", false);

            o2EditorAssets.OpenFolder(newPathAsset.Trimed(" /\\"));
        };
    }

    void AssetsFoldersTree::OnFoldersTreeSelect(Vector<void*> nodes)
    {
        if (mOpengingFolderFromThis)
            return;

        mOpengingFolderFromThis = true;

        if (nodes.Count() > 0)
        {
            AssetInfo* assetTreeNode = (AssetInfo*)nodes.Last();
            mCurrentPath = assetTreeNode->path;
            o2EditorAssets.mAssetsGridScroll->SetViewingPath(mCurrentPath);
        }
        else
            o2EditorAssets.OpenFolder("");

        mOpengingFolderFromThis = false;
    }

    void AssetsFoldersTree::OnFoldersTreeRightClick(const Ref<TreeNode>& node)
    {
        o2UI.FocusWidget(Ref(this));
        mContextMenu->Show();
    }

    void AssetsFoldersTree::OnContextCopyPressed()
    {
        if (!mCurrentPath.IsEmpty())
            o2EditorAssets.CopyAssets({ mCurrentPath });
    }

    void AssetsFoldersTree::OnContextCutPressed()
    {
        if (!mCurrentPath.IsEmpty())
            o2EditorAssets.CutAssets({ mCurrentPath });
    }

    void AssetsFoldersTree::OnContextPastePressed()
    {
        o2EditorAssets.PasteAssets(mCurrentPath);
    }

    void AssetsFoldersTree::OnContextDeletePressed()
    {
        if (!mCurrentPath.IsEmpty())
        {
            auto deletingPath = mCurrentPath;

            o2EditorAssets.OpenFolder("");
            o2EditorAssets.DeleteAssets({ deletingPath });
        }
    }

    void AssetsFoldersTree::OnContextOpenPressed()
    {
        o2EditorAssets.OpenAsset(mCurrentPath);
    }

    void AssetsFoldersTree::OnContextShowInExplorerPressed()
    {
        o2EditorAssets.OpenAsset(mCurrentPath);
    }

    void AssetsFoldersTree::OnContextCreateFolderPressed()
    {
        o2EditorAssets.mAssetsGridScroll->CreateAsset(&TypeOf(FolderAsset));
    }

    void AssetsFoldersTree::OnKeyReleased(const Input::Key& key)
    {
        if (o2UI.GetFocusedWidget() == mFoldersTree && key == VK_BACK)
        {
            if (mCurrentPath.CountOf("/") > 0)
                o2EditorAssets.OpenFolder(o2FileSystem.GetParentPath(mCurrentPath));
            else
                o2EditorAssets.OpenFolder("");
        }
    }

    FoldersTree::FoldersTree(RefCounter* refCounter):
        Tree(refCounter)
    {}

    FoldersTree::FoldersTree(RefCounter* refCounter, const FoldersTree& other):
        Tree(refCounter, other)
    {}

    FoldersTree::FoldersTree(const FoldersTree& other):
        FoldersTree(nullptr, other)
    {}

    void FoldersTree::UpdateVisibleNodes()
    {
        PushEditorScopeOnStack scope;
        Tree::UpdateNodesStructure();
    }

    String FoldersTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::FoldersTree>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AssetsFoldersTree>);
// --- META ---

DECLARE_CLASS(Editor::AssetsFoldersTree, Editor__AssetsFoldersTree);

DECLARE_CLASS(Editor::FoldersTree, Editor__FoldersTree);
// --- END META ---
