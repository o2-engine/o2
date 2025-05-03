#include "o2Editor/stdafx.h"
#include "AssetsIconsScroll.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/GridLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/StringUtils.h"
#include "o2Editor/AssetsWindow/AssetIcon.h"
#include "o2Editor/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Core/Actions/Create.h"
#include "o2Editor/Core/EditorApplication.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/TreeWindow/SceneHierarchyTree.h"

namespace Editor
{
    AssetsIconsScrollArea::AssetsIconsScrollArea(RefCounter* refCounter):
        GridLayoutScrollArea(refCounter)
    {
        mDragIcon = mmake<AssetIcon>();

        mHighlightSprite = mmake<Sprite>();
        mSelectionSprite = mmake<Sprite>();

        mHighlightAnim->SetTarget(mHighlightSprite.Get());
    }

    AssetsIconsScrollArea::AssetsIconsScrollArea(RefCounter* refCounter, const AssetsIconsScrollArea& other) :
        GridLayoutScrollArea(refCounter, other), mHighlightSprite(other.mHighlightSprite->CloneAsRef<Sprite>()),
        mHighlightLayout(other.mHighlightLayout), mHighlightAnim(other.mHighlightAnim),
        mSelectionSprite(other.mSelectionSprite->CloneAsRef<Sprite>())
    {
        PushEditorScopeOnStack scope;

        RemoveAllChildren();

        mDragIcon = o2UI.CreateWidget<AssetIcon>();

        if (other.mHighlighClip)
            mHighlighClip = other.mHighlighClip->CloneAsRef<AnimationClip>();

        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);

        RetargetStatesAnimations();
        SetLayoutDirty();
        InitializeContext();
    }

    AssetsIconsScrollArea::AssetsIconsScrollArea(const AssetsIconsScrollArea& other) :
        AssetsIconsScrollArea(nullptr, other)
    {}

    AssetsIconsScrollArea::~AssetsIconsScrollArea()
    {}

    AssetsIconsScrollArea& AssetsIconsScrollArea::operator=(const AssetsIconsScrollArea& other)
    {
        ScrollArea::operator=(other);

        mDragIcon = other.mDragIcon->CloneAsRef<AssetIcon>();

        mContextMenu = FindChildByType<ContextMenu>();

        mHighlightLayout = other.mHighlightLayout;
        mHighlightSprite = other.mHighlightSprite->CloneAsRef<Sprite>();
        mHighlighClip = other.mHighlighClip->CloneAsRef<AnimationClip>();
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);

        mSelectionSprite = other.mSelectionSprite->CloneAsRef<Sprite>();

        RetargetStatesAnimations();
        SetLayoutDirty();
        return *this;
    }

    void AssetsIconsScrollArea::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        for (auto& child : mChildrenInheritedDepth)
            child->Draw();

        if (mSelecting)
            mSelectionSprite->Draw();

        if (mHighlightSprite)
            mHighlightSprite->Draw();

        o2Render.DisableScissorTest();

        GridLayoutScrollArea::OnDrawn();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        if (mOwnHorScrollBar)
            mHorScrollBar->Draw();

        if (mOwnVerScrollBar)
            mVerScrollBar->Draw();

        if (mIsDraggingIcons && mInstantiatedSceneDragObjects.Count() == 0)
            o2UI.DrawWidgetAtTop(mDragIcon);

        if (IsUIDebugEnabled() || o2Input.IsKeyDown(VK_F1))
            DrawDebugFrame();
    }

    void AssetsIconsScrollArea::Update(float dt)
    {
        ScrollArea::Update(dt);

        if (mHighlightAnim->IsPlaying())
        {
            if (mHighlightIcon)
            {
                mHighlightSprite->SetScale(Vec2F(1.0f, 1.0f));
                mHighlightSprite->SetRect(mHighlightLayout.Calculate(mHighlightIcon->layout->worldRect));
            }

            mHighlightAnim->Update(dt);

            if (!mHighlightAnim->IsPlaying())
                mHighlightIcon = nullptr;
        }

        if (mDragEnded)
            EndDragging();
    }

    void AssetsIconsScrollArea::SetViewingPath(const String& path)
    {
        if (path == mCurrentPath)
            return;

        mCurrentPath = path;
        UpdateAssetsByCurrentPath();
        ResetScroll();
    }

    const String& AssetsIconsScrollArea::GetViewingPath() const
    {
        return mCurrentPath;
    }

    void AssetsIconsScrollArea::UpdateAssetsByCurrentPath()
    {
        PushEditorScopeOnStack scope;

        Focus();
        DeselectAllAssets();

        if (mCurrentPath != "")
        {
            AssetRef<FolderAsset> ref(mCurrentPath);
            mAssetInfos = ref->GetInfo().GetChildren();
        }
        else
            mAssetInfos = o2Assets.GetAssetsTree().rootAssets;

        SortAssetInfos();
        OnItemsUpdated(true);
    }

    void AssetsIconsScrollArea::SortAssetInfos()
    {
        Map<Ref<AssetInfo>, Pair<String, int>> sortingCache;
        for (auto& assetInfo : mAssetInfos)
        {
            sortingCache[assetInfo] = { assetInfo->path.ToLowerCase(), assetInfo->meta->GetAssetType()->InvokeStatic<int>("GetEditorSorting") };
        }

        mAssetInfos.Sort([&](const Ref<AssetInfo>& a, const Ref<AssetInfo>& b) { return sortingCache[a].second < sortingCache[b].second; });
    }

    bool AssetsIconsScrollArea::IsFocusable() const
    {
        return true;
    }

    void AssetsIconsScrollArea::UpdateCuttingAssets()
    {
        for (auto& child : mChildWidgets)
        {
            auto icon = DynamicCast<AssetIcon>(child);
            icon->SetState("halfHide", o2EditorAssets.mCuttingAssets.Contains([=](auto x) { return x.first == icon->GetAssetInfo()->meta->ID(); }));
        }
    }

    void AssetsIconsScrollArea::HighlightAsset(const UID& id)
    {
        String assetPath = o2Assets.GetAssetPath(id);
        String assetFolder = o2FileSystem.ExtractPathStr(assetPath);

        SetViewingPath(assetFolder);

        auto assetInfo = mAssetInfos.FindOrDefault([&](auto& x) { return x->meta->ID() == id; });
        ScrollTo((void*)&assetInfo);

        auto icon = DynamicCast<AssetIcon>(mChildWidgets.FindOrDefault([=](const Ref<Widget>& x) {
            return DynamicCast<AssetIcon>(x)->GetAssetInfo() == assetInfo; 
        }));

        if (!icon)
            return;

        mHighlightIcon = icon;
        mHighlightAnim->RewindAndPlay();
    }

    void AssetsIconsScrollArea::SelectAsset(const UID& id, bool scroll /*= true*/)
    {
        bool selectionChanged = false;

        String assetPath = o2Assets.GetAssetPath(id);
        String assetFolder = o2FileSystem.ExtractPathStr(assetPath);

        SetViewingPath(assetFolder);

        auto assetInfo = Ref(const_cast<AssetInfo*>(&o2Assets.GetAssetInfo(id)));
        if (!assetInfo->IsValid())
            return;

        if (!mSelectedAssets.Contains(assetInfo))
        {
            if (auto assetIcon = FindVisibleIcon(assetInfo))
                assetIcon->SetSelected(true);

            mSelectedAssets.Add(assetInfo);

            selectionChanged = true;
        }

        if (scroll)
        {
            int idx = mAssetInfos.IndexOf(assetInfo);
            float itemHeight = mItemSample->layout->minHeight;
            int itemsInLine = Math::Max(1, Math::FloorToInt(mAbsoluteViewArea.Width() / mItemSample->layout->GetMinWidth()));

            SetScroll(Vec2F(0.0f, idx / itemsInLine * itemHeight));
        }

        if (selectionChanged)
            OnAssetsSelected();
    }

    void AssetsIconsScrollArea::DeselectAllAssets()
    {
        for (auto& icon : mVisibleAssetIcons)
            icon->SetSelected(false);

        mSelectedAssets.Clear();

        OnAssetsSelected();
    }

    const Vector<Ref<AssetInfo>>& AssetsIconsScrollArea::GetSelectedAssets() const
    {
        return mSelectedAssets;
    }

    void AssetsIconsScrollArea::OnAssetsSelected()
    {
        auto lastSelectedPreloadedAssets = mSelectedPreloadedAssets;
        for (auto& asset : lastSelectedPreloadedAssets)
        {
            if (!mSelectedAssets.Contains([&](const Ref<AssetInfo>& x) {
                return x->meta->ID() == asset->GetUID(); }))
            {
                mSelectedPreloadedAssets.Remove(asset);
            }
        }

        for (auto& icon : mSelectedAssets)
        {
            if (mSelectedPreloadedAssets.Contains([&](const AssetRef<Asset>& x) { return x->GetUID() == icon->meta->ID(); }))
                continue;

            AssetRef<Asset> iconAsset = o2Assets.GetAssetRef(icon->meta->ID());
            mSelectedPreloadedAssets.Add(iconAsset);
        }

        onAssetsSelected(mSelectedAssets.Convert<String>([](const Ref<AssetInfo>& x) { return x->path; }));

        if (PropertiesWindow::IsSingletonInitialzed())
        {
            mChangePropertiesTargetsFromThis = true;

            Vector<IObject*> targets;

            if (mSelectedPreloadedAssets.All([](const AssetRef<Asset>& x) { return x->GetType() == TypeOf(ActorAsset); }))
            {
                targets = mSelectedPreloadedAssets.Convert<IObject*>([](const AssetRef<Asset>& x) { return DynamicCast<ActorAsset>(x.GetRef())->GetActor().Get(); });
            }
            else if (mSelectedPreloadedAssets.All([](const AssetRef<Asset>& x) { return x->GetType() == TypeOf(FolderAsset); }))
            {
                targets.Clear();
            }
            else
            {
                targets = mSelectedPreloadedAssets.Convert<IObject*>([](const AssetRef<Asset>& x) { return const_cast<Asset*>(x.Get()); });
            }

            if (!targets.IsEmpty())
                o2EditorPropertiesWindow.SetTargets(targets, THIS_FUNC(OnAssetsPropertiesChanged));

            mChangePropertiesTargetsFromThis = false;
        }
    }

    void AssetsIconsScrollArea::UpdateSelfTransform()
    {
        Vec2F localPressPoint = mPressedPoint - GetChildrenWorldRect().LeftBottom();

        GridLayoutScrollArea::UpdateSelfTransform();

        mPressedPoint = localPressPoint + GetChildrenWorldRect().LeftBottom();

        if (mSelecting)
            UpdateSelection(*o2Input.GetCursor(0));
    }

    bool AssetsIconsScrollArea::IsScrollable() const
    {
        return ScrollArea::IsScrollable();
    }

    bool AssetsIconsScrollArea::IsInputTransparent() const
    {
        return ScrollArea::IsInputTransparent();
    }

    String AssetsIconsScrollArea::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    int AssetsIconsScrollArea::GetItemsCount() const
    {
        return mAssetInfos.Count();
    }

    Vector<void*> AssetsIconsScrollArea::GetItemsRange(int start, int end) const
    {
        Vector<void*> result;
        for (int i = start; i < end; i++)
            result.Add((void*)&mAssetInfos[i]);

        return result;
    }

    void AssetsIconsScrollArea::SetupItemWidget(const Ref<Widget>& widget, void* item)
    {
        auto asset = *(Ref<AssetInfo>*)item;
        auto assetIcon = DynamicCast<AssetIcon>(widget);

        auto iconLayer = assetIcon->layer["icon"];
        auto iconSprite = DynamicCast<Sprite>(iconLayer->GetDrawable());

        if (asset->meta->GetAssetType() == &TypeOf(ImageAsset))
        {
            AssetRef<ImageAsset> previewSpriteAsset(asset->path);
            float previewMaxSize = 30;

            if (previewSpriteAsset->width > previewSpriteAsset->height)
            {
                float cf = previewSpriteAsset->height / previewSpriteAsset->width;
                iconLayer->layout = Layout::Based(BaseCorner::Center, Vec2F(previewMaxSize, previewMaxSize * cf),
                                                  Vec2F(0, 10));
            }
            else
            {
                float cf = previewSpriteAsset->width / previewSpriteAsset->height;
                iconLayer->layout = Layout::Based(BaseCorner::Center, Vec2F(previewMaxSize * cf, previewMaxSize),
                                                  Vec2F(0, 10));
            }

            iconSprite->image = previewSpriteAsset;
            iconSprite->mode = SpriteMode::Default;
        }
        else
        {
            iconSprite->imageName = asset->meta->GetAssetType()->InvokeStatic<String>("GetEditorIcon");
            iconSprite->mode = SpriteMode::FixedAspect;
            iconLayer->layout = Layout::Based(BaseCorner::Center, Vec2F(40, 40), Vec2F(0, 10));
        }

        assetIcon->SetAssetInfo(asset);
        assetIcon->SetState("halfHide", mCuttingAssets.Contains([&](auto x) { return x.first == asset->meta->ID(); }));
        assetIcon->SetSelectionGroup(Ref(this));
        assetIcon->SetSelected(mSelectedAssets.Contains(Ref(asset)));
        assetIcon->SetDragOnlySelected(true);
        assetIcon->mOwner = Ref(this);
    }

    void AssetsIconsScrollArea::UpdateVisibleItems()
    {
        PushEditorScopeOnStack scope;
        GridLayoutScrollArea::UpdateVisibleItems();

        mVisibleAssetIcons = DynamicCastVector<AssetIcon>(mChildWidgets);
    }

    void AssetsIconsScrollArea::OnFocused()
    {
        for (auto& icon : mChildWidgets)
            icon->SetState("focused", true);

        Widget::OnFocused();
    }

    void AssetsIconsScrollArea::OnUnfocused()
    {
        for (auto& icon : mChildWidgets)
            icon->SetState("focused", false);

        Widget::OnUnfocused();
    }

    void AssetsIconsScrollArea::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (auto icon = GetIconUnderPoint(cursor.position))
        {
            if (icon->IsSelected())
                return;
        }

        mPressedPoint = cursor.position;

        BeginSelecting();
    }

    void AssetsIconsScrollArea::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (cursor.delta == Vec2F())
            return;

        if (mSelecting)
            UpdateSelection(cursor);
    }

    void AssetsIconsScrollArea::BeginSelecting()
    {
        mSelecting = true;

        if (!o2Input.IsKeyDown(VK_CONTROL))
            DeselectAllAssets();

        o2UI.FocusWidget(Ref(this));

        RectF selectionRect(mPressedPoint, mPressedPoint);
        mSelectionSprite->SetRect(selectionRect);

        mCurrentSelectingInfos.Clear();
    }

    void AssetsIconsScrollArea::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mSelecting)
            CompleteSelecting();
        else if (!mIsDraggingIcons)
        {
            if (!o2Input.IsKeyDown(VK_CONTROL))
                DeselectAllAssets();
        }
    }

    void AssetsIconsScrollArea::BeginDragging(const Ref<AssetIcon>& icon)
    {
        mIsDraggingIcons = true;
        mDragEnded = false;

        Ref<AssetIcon> dragIcon;
        for (auto& sel : mSelectedAssets)
        {
            if (auto icon = FindVisibleIcon(sel))
            {
                icon->Hide();
                dragIcon = icon;
            }
        }

        if (dragIcon)
        {
            *mDragIcon = *dragIcon;
            mDragIcon->DragDropArea::SetInteractable(false);
            mDragIcon->CursorAreaEventsListener::SetInteractable(false);
            mDragIcon->SetEnabled(true);
            mDragIcon->RemoveFromScene();
            mDragOffset = icon->layout->worldCenter - o2Input.GetCursorPos();

            if (mSelectedAssets.Count() > 1)
                mDragIcon->assetName = (String)mSelectedAssets.Count() + " items";
        }
    }

    void AssetsIconsScrollArea::EndDragging(bool droppedToThis /*= false*/)
    {
        for (auto& sel : mSelectedAssets)
        {
            if (auto icon = FindVisibleIcon(sel))
                icon->Show();
        }

        mIsDraggingIcons = false;
    }

    void AssetsIconsScrollArea::UpdateDraggingGraphics()
    {
        mDragIcon->SetEditableParent(nullptr);
        mDragIcon->layout->SetRect(RectF(o2Input.GetCursorPos() - mAssetIconSize * 0.5f + mDragOffset,
                                         o2Input.GetCursorPos() + mAssetIconSize * 0.5f + mDragOffset));

        mDragIcon->UpdateSelfTransform();
        mDragIcon->UpdateChildrenTransforms();
    }

    void AssetsIconsScrollArea::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {}

    void AssetsIconsScrollArea::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
    {}

    void AssetsIconsScrollArea::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {}

    void AssetsIconsScrollArea::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        if (DynamicCast<AssetsIconsScrollArea>(group))
            OnDroppedFromThis();
        else if (auto sceneTree = DynamicCast<SceneHierarchyTree>(group))
            OnDroppedFromSceneTree(sceneTree);
    }

    void AssetsIconsScrollArea::OnDroppedFromThis()
    {
        for (auto& sel : mSelectedAssets)
        {
            if (auto icon = FindVisibleIcon(sel))
                icon->Show();
        }

        auto iconUnderCursor = GetIconUnderPoint(o2Input.GetCursorPos());
        if (iconUnderCursor && iconUnderCursor->GetAssetInfo()->meta->GetAssetType() == &TypeOf(FolderAsset))
        {
            String destPath = iconUnderCursor->GetAssetInfo()->path;
            auto assetsInfos = mSelectedAssets.Convert<UID>([](const Ref<AssetInfo>& x) { return x->meta->ID(); });
            DeselectAllAssets();

            o2Assets.MoveAssets(assetsInfos, destPath);
            o2Assets.RebuildAssets();
        }
    }

    void AssetsIconsScrollArea::OnDroppedFromSceneTree(const Ref<SceneHierarchyTree>& sceneTree)
    {
        String destPath = mCurrentPath;

        auto iconUnderCursor = GetIconUnderPoint(o2Input.GetCursorPos());
        if (iconUnderCursor && iconUnderCursor->GetAssetInfo()->meta->GetAssetType() == &TypeOf(FolderAsset))
            destPath = iconUnderCursor->GetAssetInfo()->path;

        Vector<String> newAssets;
        for (auto& object : sceneTree->GetSelectedObjects())
        {
            if (auto actor = DynamicCast<Actor>(object))
            {
                AssetRef<ActorAsset> newAsset = actor->MakePrototype();
                String path = destPath.IsEmpty() ? newAsset->GetActor()->name + String(".proto") : destPath + "/" +
                    newAsset->GetActor()->name + String(".proto");

                String uniquePath = o2Assets.MakeUniqueAssetName(path);
                newAsset->Save(uniquePath);

                newAssets.Add(uniquePath);
            }
        }

        o2Assets.RebuildAssets();
        o2EditorAssets.OpenFolder(destPath);
        o2EditorAssets.SelectAssets(newAssets);
    }

    void AssetsIconsScrollArea::RegObjectsCreationAction()
    {
        if (mInstantiatedSceneDragObjects.IsEmpty())
            return;

        auto firstInstObject = mInstantiatedSceneDragObjects[0];
        auto parent = firstInstObject->GetEditableParent();
        auto parentChilds = parent ? parent->GetEditableChildren() : o2Scene.GetRootEditableObjects();
        int idx = parentChilds.IndexOf(firstInstObject);
        auto prevActor = idx > 0 ? parentChilds[idx - 1] : nullptr;

        auto createAction = mmake<CreateAction>(mInstantiatedSceneDragObjects, parent, prevActor);

        o2EditorApplication.DoneAction(createAction);
    }

    void AssetsIconsScrollArea::InstantiateDraggingAssets()
    {
        ForcePopEditorScopeOnStack scope;

        for (auto& sel : mSelectedAssets)
        {
            auto actor = InstantiateAsset(*sel);
            if (actor)
            {
                actor->name = o2FileSystem.GetPathWithoutDirectories(o2FileSystem.GetFileNameWithoutExtension(sel->path));
                mInstantiatedSceneDragObjects.Add(actor);
            }
        }
    }

    void AssetsIconsScrollArea::ClearInstantiatedDraggingAssets()
    {
        for (auto& object : mInstantiatedSceneDragObjects)
            object->Destroy();

        mInstantiatedSceneDragObjects.Clear();
    }

    void AssetsIconsScrollArea::OnCursorPressBreak(const Input::Cursor& cursor)
    {}

    void AssetsIconsScrollArea::UpdateSelection(const Input::Cursor& cursor)
    {
        RectF selectionRect(cursor.position, mPressedPoint);
        mSelectionSprite->SetRect(selectionRect);

        for (auto& info : mCurrentSelectingInfos)
        {
            if (auto icon = FindVisibleIcon(info))
                icon->SetSelected(false);
        }

        mCurrentSelectingInfos.Clear();

        for (auto& child : mChildWidgets)
        {
            if (child->layout->GetWorldRect().IsIntersects(selectionRect))
            {
                auto icon = DynamicCast<AssetIcon>(child);
                icon->SetSelected(true);
                mCurrentSelectingInfos.Add(icon->GetAssetInfo());
            }
        }
    }

    void AssetsIconsScrollArea::CompleteSelecting()
    {
        mSelectedAssets.Add(mCurrentSelectingInfos);
        mCurrentSelectingInfos.Clear();
        mSelecting = false;

        OnAssetsSelected();
    }

    void AssetsIconsScrollArea::OnCursorMoved(const Input::Cursor& cursor)
    {}

    void AssetsIconsScrollArea::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        auto iconUnderCursor = GetIconUnderPoint(cursor.position);
        if (iconUnderCursor)
        {
            if (!mSelectedAssets.Contains(iconUnderCursor->GetAssetInfo()))
            {
                DeselectAllAssets();
                iconUnderCursor->SetSelected(true);
                mCurrentSelectingInfos.Add(iconUnderCursor->GetAssetInfo());
            }
        }

        o2UI.FocusWidget(Ref(this));
        mContextMenu->Show();
    }

    void AssetsIconsScrollArea::OnScrolled(float scroll)
    {
        ScrollArea::OnScrolled(scroll);
    }

    void AssetsIconsScrollArea::OnKeyReleased(const Input::Key& key)
    {
        if (mIsFocused && key == VK_BACK)
        {
            if (mCurrentPath.CountOf("/") > 0)
                o2EditorAssets.OpenFolder(o2FileSystem.GetParentPath(mCurrentPath));
            else
                o2EditorAssets.OpenFolder("");
        }
    }

    void AssetsIconsScrollArea::InitializeContext()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        mContextMenu->AddItem("Open", [&]() { OnContextOpenPressed(); });
        mContextMenu->AddItem("Show in folder", [&]() { OnContextShowInExplorerPressed(); });
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("New folder", [&]() { CreateAsset(&TypeOf(FolderAsset)); });

        InitializeCreateContext();

        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Copy", [&]() { OnContextCopyPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('C', true));
        mContextMenu->AddItem("Cut", [&]() { OnContextCutPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('X', true));
        mContextMenu->AddItem("Paste", [&]() { OnContextPastePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('V', true));
        mContextMenu->AddItem("Delete", [&]() { OnContextDeletePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys(VK_DELETE));

        onFocused = [&]() { mContextMenu->SetItemsMaxPriority(); };
        onUnfocused = [&]() {
            mContextMenu->SetItemsMinPriority(); };

        AddInternalWidget(mContextMenu);
    }

    void AssetsIconsScrollArea::InitializeCreateContext()
    {
        auto types = TypeOf(Asset).GetDerivedTypes();

        for (auto& type : types)
        {
            if (!type->InvokeStatic<bool>("IsAvailableToCreateFromEditor"))
                continue;

            if (type == &TypeOf(FolderAsset))
                continue;

            mContextMenu->AddItem("Create/" + GetSmartName(type->GetName()),
                                  [=]() { CreateAsset(type); });
        }
    }

    Ref<AssetIcon> AssetsIconsScrollArea::GetAssetIconFromPool(const String& style)
    {
        if (!mIconsPool.ContainsKey(style))
            mIconsPool.Add(style, Vector<Ref<AssetIcon>>());

        int poolResizeStep = 10;

        if (mIconsPool[style].Count() == 0)
        {
            for (int i = 0; i < poolResizeStep; i++)
            {
                auto sample = o2UI.CreateWidget<AssetIcon>(style);
                mIconsPool[style].Add(sample);
            }
        }

        auto sample = mIconsPool[style].PopBack();
        return sample;
    }

    void AssetsIconsScrollArea::FreeAssetIconToPool(const Ref<AssetIcon>& icon)
    {
        if (mIconsPool.ContainsKey(icon->name))
            mIconsPool[icon->name].Add(icon);
    }

    Ref<AssetIcon> AssetsIconsScrollArea::FindVisibleIcon(const Ref<AssetInfo>& info)
    {
        return mVisibleAssetIcons.FindOrDefault([=](auto& x) { return x->GetAssetInfo() == info; });
    }

    void AssetsIconsScrollArea::OnAssetDblClick(const Ref<AssetIcon>& icon)
    {
        AssetInfo iconAssetInfo = *icon->GetAssetInfo();
        auto assetNameLabel = icon->GetChildWidget("nameLabel");
        if (assetNameLabel && assetNameLabel->IsUnderPoint(o2Input.cursorPos))
        {
            auto name = o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(iconAssetInfo.path));
            StartAssetRenaming(icon, name, [=](const String& name)
                               {
                                   String extension = o2FileSystem.GetFileExtension(iconAssetInfo.path);
                                   String newName = extension.IsEmpty() ? name : name + "." + extension;
                                   o2Assets.RenameAsset(iconAssetInfo.meta->ID(), newName);
                                   o2Assets.RebuildAssets();

                                   String parentPath = o2FileSystem.GetParentPath(iconAssetInfo.path);
                                   String newPath = parentPath.IsEmpty() ? newName : parentPath + "/" + newName;
                                   o2EditorAssets.SelectAsset(newPath);
                               });
        }
        else
        {
            if (iconAssetInfo.meta->GetAssetType() == &TypeOf(FolderAsset))
                o2EditorAssets.OpenFolder(iconAssetInfo.path);
            else
                o2EditorAssets.OpenAndEditAsset(iconAssetInfo.meta->ID());
        }
    }

    Ref<AssetIcon> AssetsIconsScrollArea::GetIconUnderPoint(const Vec2F& point) const
    {
        for (auto& child : mChildWidgets)
        {
            if (child->layout->IsPointInside(point))
            {
                auto icon = DynamicCast<AssetIcon>(child);
                return icon;
            }
        }

        return nullptr;
    }

    const Ref<Sprite>& AssetsIconsScrollArea::GetHighlightDrawable() const
    {
        return mHighlightSprite;
    }

    void AssetsIconsScrollArea::SetHighlightAnimation(const Ref<AnimationClip>& animation)
    {
        mHighlighClip = animation;
        mHighlightAnim->SetClip(mHighlighClip);
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
    }

    void AssetsIconsScrollArea::SetHighlightLayout(const Layout& layout)
    {
        mHighlightLayout = layout;
    }

    const Ref<Sprite>& AssetsIconsScrollArea::GetSelectingDrawable() const
    {
        return mSelectionSprite;
    }

    bool AssetsIconsScrollArea::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

#undef CopyFile

    void AssetsIconsScrollArea::OnContextCopyPressed()
    {
        o2EditorAssets.CopyAssets(
            mSelectedAssets.Convert<String>([](const Ref<AssetInfo>& x) { return x->path; }));
    }

    void AssetsIconsScrollArea::OnContextCutPressed()
    {
        o2EditorAssets.CutAssets(
            mSelectedAssets.Convert<String>([](const Ref<AssetInfo>& x) { return x->path; }));
    }

    void AssetsIconsScrollArea::OnContextPastePressed()
    {
        o2EditorAssets.PasteAssets(mCurrentPath);
        UpdateAssetsByCurrentPath();
    }


    void AssetsIconsScrollArea::OnContextDeletePressed()
    {
        o2EditorPropertiesWindow.ResetTargets();

        mSelectedPreloadedAssets.Clear();

        o2EditorAssets.DeleteAssets(
            mSelectedAssets.Convert<String>([](const Ref<AssetInfo>& x) { return x->path; }));
    }

    void AssetsIconsScrollArea::OnContextOpenPressed()
    {
        if (mSelectedAssets.Count() > 0)
            o2EditorAssets.OpenAndEditAsset(mSelectedAssets.Last()->meta->ID());
    }

    void AssetsIconsScrollArea::OnContextShowInExplorerPressed()
    {
        if (mSelectedAssets.Count() > 0)
            o2EditorAssets.OpenAsset(mSelectedAssets.Last()->meta->ID());
    }

    void AssetsIconsScrollArea::CreateAsset(const Type* assetType)
    {
        String newAssetName = "New " + GetSmartName(assetType->GetName());

        auto objectType = dynamic_cast<const ObjectType*>(assetType);

        mNewAsset = DynamicCast<Asset>(objectType->CreateSampleRef());
        mNewAsset->SetPath(mCurrentPath + "/" + newAssetName);
        mAssetInfos.Add(mmake<AssetInfo>(mNewAsset->GetInfo()));

        SortAssetInfos();
        OnItemsUpdated(true);
        ScrollTo((void*)&mNewAsset->GetInfo());

        auto icon = DynamicCast<AssetIcon>(mChildWidgets.FindOrDefault([=](const Ref<Widget>& x) {
            return DynamicCast<AssetIcon>(x)->GetAssetInfo() == &mNewAsset->GetInfo(); 
        }));

        if (!icon)
            return;

        auto extesions = assetType->InvokeStatic<Vector<String>>("GetFileExtensions");
        auto extension = !extesions.IsEmpty() ? extesions[0] : String("");

        StartAssetRenaming(icon, newAssetName,
                           [&, extension](const String& name)
                           {
                               String path = (!mCurrentPath.IsEmpty() ? mCurrentPath + "/" + name : name);
                               if (!extension.IsEmpty())
                                   path += "." + extension;

                               mNewAsset->Save(path);

                               o2Assets.RebuildAssets();
                               o2EditorAssets.SelectAsset(path);
                           });
    }

    Ref<Actor> AssetsIconsScrollArea::InstantiateAsset(const AssetRef<ImageAsset>& asset)
    {
        auto actor = mmake<Actor>();
        auto comp = mmake<ImageComponent>(asset);
        actor->AddComponent(comp);
        comp->FitActorByImage();
        return actor;
    }

    Ref<Actor> AssetsIconsScrollArea::InstantiateAsset(const AssetRef<ActorAsset>& asset)
    {
        return asset->GetActor()->CloneAsRef<Actor>();
    }

    void AssetsIconsScrollArea::OnAssetsPropertiesChanged()
    {
        for (auto& asset : mSelectedPreloadedAssets)
            asset->SetDirty();

        CheckPreloadedAssetsSaving();
    }

    void AssetsIconsScrollArea::CheckPreloadedAssetsSaving()
    {
        if (mChangePropertiesTargetsFromThis)
            return;

        for (auto& asset : mSelectedPreloadedAssets)
            asset->Save();

        mSelectedPreloadedAssets.Clear();
    }

    void AssetsIconsScrollArea::Select(const Ref<SelectableDragableObject>& object, bool sendOnSelectionChanged)
    {
        auto icon = DynamicCast<AssetIcon>(object);
        auto info = icon->GetAssetInfo();

        if (!mSelectedAssets.Contains(info))
        {
            icon->SetSelected(true);
            mSelectedAssets.Add(info);

            if (sendOnSelectionChanged)
                OnAssetsSelected();
        }
    }

    void AssetsIconsScrollArea::Deselect(const Ref<SelectableDragableObject>& object)
    {
        auto icon = DynamicCast<AssetIcon>(object);
        auto info = icon->GetAssetInfo();

        if (mSelectedAssets.Contains(info))
        {
            icon->SetSelected(false);
            mSelectedAssets.Remove(info);

            OnAssetsSelected();
        }
    }

    void AssetsIconsScrollArea::AddSelectableObject(const Ref<SelectableDragableObject>& object)
    {}

    void AssetsIconsScrollArea::RemoveSelectableObject(SelectableDragableObject* object)
    {}

    void AssetsIconsScrollArea::OnSelectableObjectCursorReleased(const Ref<SelectableDragableObject>& object, const Input::Cursor& cursor)
    {
        //         if ((mPressedPoint - cursor.position).Length() > 5.0f)
        //             return;

        if (!o2Input.IsKeyDown(VK_CONTROL) && !o2Input.IsKeyDown(VK_SHIFT))
            DeselectAllAssets();

        if (o2Input.IsKeyDown(VK_SHIFT) && !mSelectedAssets.IsEmpty())
        {
            auto selectIcon = DynamicCast<AssetIcon>(object);
            auto selectInfo = selectIcon->GetAssetInfo();
            int iconUnderCursorIdx = mAssetInfos.IndexOf(selectInfo);
            int lastSelectedIdx = mAssetInfos.IndexOf(mSelectedAssets.Last());

            int begin = Math::Min(iconUnderCursorIdx, lastSelectedIdx);
            int end = Math::Max(iconUnderCursorIdx, lastSelectedIdx);

            for (int i = begin; i <= end; i++)
            {
                auto info = mAssetInfos[i];
                if (mSelectedAssets.Contains(info))
                    continue;

                if (auto icon = FindVisibleIcon(info))
                    Select(icon);
                else
                    mSelectedAssets.Add(info);
            }
        }
        else
        {
            if (object->IsSelected())
                Deselect(object);
            else
                Select(object);
        }
    }

    void AssetsIconsScrollArea::OnSelectableObjectBeganDragging(const Ref<SelectableDragableObject>& object)
    {
        if (mSelecting)
        {
            Select(object, false);
            mSelecting = false;
        }

        if (!o2Input.IsKeyDown(VK_CONTROL) && !mSelectedAssets.Contains(DynamicCast<AssetIcon>(object)->GetAssetInfo()))
        {
            for (auto& info : mSelectedAssets)
            {
                if (auto icon = FindVisibleIcon(info))
                    icon->SetSelected(false);
            }

            mSelectedAssets.Clear();
        }

        if (!object->IsSelected())
            Select(object, false);
    }

    Ref<Actor> AssetsIconsScrollArea::InstantiateAsset(const AssetInfo& assetInfo)
    {
        if (assetInfo.meta->GetAssetType() == &TypeOf(ImageAsset))
            return InstantiateAsset(AssetRef<ImageAsset>(assetInfo.meta->ID()));
        else if (assetInfo.meta->GetAssetType() == &TypeOf(ActorAsset))
            return InstantiateAsset(AssetRef<ActorAsset>(assetInfo.meta->ID()));

        return nullptr;
    }

    Vector<Ref<SelectableDragableObject>> AssetsIconsScrollArea::GetSelectedDragObjects() const
    {
        return DynamicCastVector<SelectableDragableObject>(mSelectedAssets);
    }

    Vector<Ref<SelectableDragableObject>> AssetsIconsScrollArea::GetAllObjects() const
    {
        return DynamicCastVector<SelectableDragableObject>(mChildWidgets);
    }

    void AssetsIconsScrollArea::Select(const Ref<SelectableDragableObject>& object)
    {
        Select(object, true);
    }

    void AssetsIconsScrollArea::StartAssetRenaming(const Ref<AssetIcon>& icon, const String& name,
                                                   const Function<void(const String&)>& onCompleted)
    {
        icon->SetState("edit", true);

        auto editBox = icon->GetChildByType<EditBox>("nameEditBox");
        editBox->text = name;

        editBox->SelectAll();
        editBox->Focus();
        editBox->ResetScroll();

        editBox->onChangeCompleted = [=](const WString& text) {
            icon->SetState("edit", false);
            onCompleted(text);
        };
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AssetsIconsScrollArea>);
// --- META ---

DECLARE_CLASS(Editor::AssetsIconsScrollArea, Editor__AssetsIconsScrollArea);
// --- END META ---
