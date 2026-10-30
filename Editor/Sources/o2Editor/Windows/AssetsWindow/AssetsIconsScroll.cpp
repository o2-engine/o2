#include "o2Editor/stdafx.h"
#include "AssetsIconsScroll.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Application/VKCodes.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Utils/CommonTextures.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/StringUtils.h"
#include "o2Editor/Actions/Create.h"
#include "o2Editor/Actions/CreateAsset.h"
#include "o2Editor/Actions/CreateAssetFromScene.h"
#include "o2Editor/Actions/MoveAsset.h"
#include "o2Editor/Actions/RenameAsset.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"

namespace Editor
{
    namespace
    {
        // Fits the size into the area of a layout with one anchor point, keeping the aspect
        Layout FitIntoArea(const Layout& area, float width, float height)
        {
            Vec2F areaSize = area.offsetMax - area.offsetMin;
            Vec2F center = (area.offsetMin + area.offsetMax)*0.5f;
            float scale = Math::Min(areaSize.x/Math::Max(width, 1.0f), areaSize.y/Math::Max(height, 1.0f));
            Vec2F size(width*scale, height*scale);
            return Layout(area.anchorMin, area.anchorMax, center - size*0.5f, center + size*0.5f);
        }

        int CompareDates(const TimeStamp& a, const TimeStamp& b)
        {
            int av[] = { a.mYear, a.mMonth, a.mDay, a.mHour, a.mMinute, a.mSecond };
            int bv[] = { b.mYear, b.mMonth, b.mDay, b.mHour, b.mMinute, b.mSecond };
            for (int i = 0; i < 6; i++)
            {
                if (av[i] != bv[i])
                    return av[i] < bv[i] ? -1 : 1;
            }

            return 0;
        }
    }

    AssetsIconsScrollArea::AssetsIconsScrollArea(RefCounter* refCounter):
        GridLayoutScrollArea(refCounter)
    {
        mDragIcon = mmake<AssetIcon>();

        mHighlightSprite = mmake<Sprite>();
        mSelectionSprite = mmake<Sprite>();
        mHoverDrawable = mmake<Sprite>();

        mHighlightAnim->SetTarget(mHighlightSprite.Get());
    }

    AssetsIconsScrollArea::AssetsIconsScrollArea(RefCounter* refCounter, const AssetsIconsScrollArea& other) :
        GridLayoutScrollArea(refCounter, other), mHighlightSprite(other.mHighlightSprite->CloneAsRef<IRectDrawable>()),
        mHighlightLayout(other.mHighlightLayout), mHighlightAnim(other.mHighlightAnim),
        mSelectionSprite(other.mSelectionSprite->CloneAsRef<IRectDrawable>()),
        mHoverDrawable(other.mHoverDrawable->CloneAsRef<IRectDrawable>()), mHoverLayout(other.mHoverLayout)
    {
        PushEditorScopeOnStack scope;

        RemoveAllChildren();

        if (auto clonedHeader = FindInternalWidget("table header"))
            RemoveInternalWidget(clonedHeader);

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
    {
    }

    AssetsIconsScrollArea& AssetsIconsScrollArea::operator=(const AssetsIconsScrollArea& other)
    {
        ScrollArea::operator=(other);

        mDragIcon = other.mDragIcon->CloneAsRef<AssetIcon>();

        mContextMenu = FindChildByType<ContextMenu>();

        mHighlightLayout = other.mHighlightLayout;
        mHighlightSprite = other.mHighlightSprite->CloneAsRef<IRectDrawable>();
        mHighlighClip = other.mHighlighClip->CloneAsRef<AnimationClip>();
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);

        mSelectionSprite = other.mSelectionSprite->CloneAsRef<IRectDrawable>();
        mHoverDrawable = other.mHoverDrawable->CloneAsRef<IRectDrawable>();
        mHoverLayout = other.mHoverLayout;

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

        if (mTableHeader && mViewMode == ViewMode::Table)
            DrawTableLines(false);

        DrawInheritedDepthChildren();

        mHoverDrawable->Draw();

        if (mSelecting)
            mSelectionSprite->Draw();

        if (mHighlightSprite)
            mHighlightSprite->Draw();

        o2Render.DisableScissorTest();

        if (mTableHeader && mViewMode == ViewMode::Table)
        {
            mTableHeader->Draw();
            DrawTableLines(true);
        }

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

        const float rectLerpCoef = 10.0f;
        if (mCurrentHoverRect != mTargetHoverRect)
        {
            mCurrentHoverRect = Math::Lerp(mCurrentHoverRect, mTargetHoverRect, dt*rectLerpCoef);
            mHoverDrawable->SetRect(mCurrentHoverRect);
        }

        if (mHighlightAnim->IsPlaying())
        {
            if (mHighlightIcon)
            {
                mHighlightSprite->SetScale2D(Vec2F(1.0f, 1.0f));
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

        mFileInfos.Clear();
        SortAssetInfos();
        OnItemsUpdated(true);
    }

    void AssetsIconsScrollArea::ShowSearchResults(const Vector<Ref<AssetInfo>>& matches)
    {
        PushEditorScopeOnStack scope;

        DeselectAllAssets();
        mAssetInfos = matches;
        mFileInfos.Clear();
        SortAssetInfos();
        OnItemsUpdated(true);
        ResetScroll();
    }

    void AssetsIconsScrollArea::SortAssetInfos()
    {
        struct SortKey
        {
            int     typeSorting = 0;
            String  name;
            WString type;
            Int64   size = 0;
            TimeStamp date;
        };

        Map<Ref<AssetInfo>, SortKey> keys;
        for (auto& assetInfo : mAssetInfos)
        {
            SortKey key;
            key.typeSorting = assetInfo->meta->GetAssetType()->InvokeStatic<int>("GetEditorSorting");
            key.name = assetInfo->path.ToLowerCase();

            if (mSortBy == SortBy::Type)
                key.type = GetTypeText(*assetInfo);
            else if (mSortBy == SortBy::Size || mSortBy == SortBy::Modified)
            {
                auto& file = GetFileInfo(assetInfo);
                key.size = file.size;
                key.date = file.editDate;
            }

            keys[assetInfo] = key;
        }

        mAssetInfos.Sort([&](const Ref<AssetInfo>& a, const Ref<AssetInfo>& b)
        {
            auto& ak = keys[a];
            auto& bk = keys[b];
            if (ak.typeSorting != bk.typeSorting)
                return ak.typeSorting > bk.typeSorting;

            int order = 0;
            if (mSortBy == SortBy::Type && ak.type != bk.type)
                order = ak.type < bk.type ? -1 : 1;
            else if (mSortBy == SortBy::Size && ak.size != bk.size)
                order = ak.size < bk.size ? -1 : 1;
            else if (mSortBy == SortBy::Modified)
                order = CompareDates(ak.date, bk.date);

            if (order == 0 && ak.name != bk.name)
                order = ak.name < bk.name ? -1 : 1;

            if (order != 0)
                return mSortDescending ? order > 0 : order < 0;

            return a->meta->ID() < b->meta->ID();
        });
    }

    const FileInfo& AssetsIconsScrollArea::GetFileInfo(const Ref<AssetInfo>& info)
    {
        UID id = info->meta->ID();
        if (mFileInfos.ContainsKey(id))
            return mFileInfos[id];

        mFileInfos[id] = o2FileSystem.GetFileInfo(o2Assets.GetAssetsPath() + info->path);
        return mFileInfos[id];
    }

    const Vector<Ref<AssetInfo>>& AssetsIconsScrollArea::GetAssetInfos() const
    {
        return mAssetInfos;
    }

    const Vector<Ref<AssetIcon>>& AssetsIconsScrollArea::GetVisibleIcons() const
    {
        return mVisibleAssetIcons;
    }

    void AssetsIconsScrollArea::SetViewMode(ViewMode mode)
    {
        PushEditorScopeOnStack scope;

        if (!mBaseLayoutsStored)
        {
            mGridViewLayout = GetViewLayout();
            mGridClippingLayout = GetClippingLayout();
            mGridItemsSpacing = GetItemsSpacing();
            mBaseLayoutsStored = true;
        }

        mViewMode = mode;

        UpdateHover(nullptr);
        OnItemsUpdated(true);
        mItemsPool.Clear();
        mVisibleAssetIcons.Clear();

        const char* styles[] = { "standard", "list", "table" };
        SetItemSample(o2UI.CreateWidget<AssetIcon>(styles[(int)mode]));
        SetItemsSpacing(mode == ViewMode::Grid ? mGridItemsSpacing : Vec2F());

        Layout viewLayout = mGridViewLayout;
        Layout clippingLayout = mGridClippingLayout;
        if (mode == ViewMode::Table)
        {
            if (!mTableHeader)
                InitializeTableHeader();

            viewLayout.offsetMax.y -= tableHeaderHeight;
            clippingLayout.offsetMax.y -= tableHeaderHeight + 1.0f;
            UpdateTableHeader();
        }

        if (mTableHeader)
            mTableHeader->SetEnabledForcible(mode == ViewMode::Table);

        SetViewLayout(viewLayout);
        SetClippingLayout(clippingLayout);
        SetItemsFitWidth(mode != ViewMode::Grid);
        ResetScroll();
    }

    AssetsIconsScrollArea::ViewMode AssetsIconsScrollArea::GetViewMode() const
    {
        return mViewMode;
    }

    void AssetsIconsScrollArea::SetSort(SortBy sortBy, bool descending /*= false*/)
    {
        mSortBy = sortBy;
        mSortDescending = descending;

        SortAssetInfos();
        OnItemsUpdated(true);
        UpdateTableHeader();
    }

    AssetsIconsScrollArea::SortBy AssetsIconsScrollArea::GetSortBy() const
    {
        return mSortBy;
    }

    bool AssetsIconsScrollArea::IsSortDescending() const
    {
        return mSortDescending;
    }

    const Ref<Widget>& AssetsIconsScrollArea::GetTableHeader() const
    {
        return mTableHeader;
    }

    void AssetsIconsScrollArea::InitializeTableHeader()
    {
        mTableHeader = mmake<Widget>();
        mTableHeader->name = "table header";
        *mTableHeader->layout = WidgetLayout::HorStretch(VerAlign::Top, mGridViewLayout.offsetMin.x, -mGridViewLayout.offsetMax.x,
                                                         tableHeaderHeight, 2.0f);
        AddInternalWidget(mTableHeader);

        const char* captions[] = { "Name", "Type", "Size", "Modified" };

        mTableHeaderButtons.Clear();
        for (int column = 0; column < 4; column++)
        {
            auto button = o2UI.CreateWidget<Button>("asset table column");
            button->name = captions[column];
            button->SetCaption(captions[column]);
            *button->layout = AssetIcon::GetTableColumnLayout(column);
            button->onClick = [this, column]() {
                SortBy sortBy = (SortBy)column;
                SetSort(sortBy, sortBy == mSortBy ? !mSortDescending : false);
                if (onViewChanged)
                    onViewChanged();
            };

            // The size column keeps its numbers by the right edge, the title too
            if (column == (int)SortBy::Size)
            {
                button->GetLayer("caption")->layout = Layout::BothStretch(16, 0, 4, 0);
                button->GetLayerDrawable<Text>("caption")->SetHorAlign(HorAlign::Right);
                button->GetLayer("sortArrow")->layout = Layout::Based(BaseCorner::Left, Vec2F(16, 16), Vec2F(1, 0));
            }

            mTableHeader->AddChild(button);
            mTableHeaderButtons.Add(button);
        }
    }

    void AssetsIconsScrollArea::UpdateTableHeader()
    {
        for (int column = 0; column < mTableHeaderButtons.Count(); column++)
        {
            auto arrow = mTableHeaderButtons[column]->GetLayer("sortArrow");
            arrow->SetEnabled((int)mSortBy == column);
            arrow->GetDrawable()->SetAngle(mSortDescending ? 0.0f : Math::PI());
        }
    }

    void AssetsIconsScrollArea::DrawTableLines(bool header)
    {
        if (mTableHeaderButtons.IsEmpty())
            return;

        Color4 color = EditorTheme::Get().text;
        color.a = 45;

        RectF headerRect = mTableHeader->layout->GetWorldRect();
        float top = header ? headerRect.top : headerRect.bottom;
        float bottom = headerRect.bottom;
        if (!header)
        {
            bottom = mAbsoluteClipArea.bottom;
            if (!mChildWidgets.IsEmpty())
                bottom = Math::Max(bottom, mChildWidgets.Last()->layout->GetWorldRect().bottom);
        }

        for (int column = 1; column < mTableHeaderButtons.Count(); column++)
        {
            float x = Math::Round(mTableHeaderButtons[column]->layout->GetWorldRect().left - 4.0f);
            o2Render.DrawLine(Vec2F(x, top), Vec2F(x, bottom), color);
        }

        if (header)
            o2Render.DrawLine(Vec2F(headerRect.left, Math::Round(bottom)), Vec2F(headerRect.right, Math::Round(bottom)), color);
    }

    WString AssetsIconsScrollArea::GetTypeText(const AssetInfo& info)
    {
        auto type = info.meta->GetAssetType();
        if (!type)
            return WString();

        String name = type->GetName();
        int namespaceEnd = name.FindLast("::");
        if (namespaceEnd >= 0)
            name = name.SubStr(namespaceEnd + 2);

        if (name.EndsWith("Asset") && name.Length() > 5)
            name = name.SubStr(0, name.Length() - 5);

        return GetSmartName(name);
    }

    WString AssetsIconsScrollArea::GetSizeText(const AssetInfo& info, const FileInfo& file)
    {
        if (info.meta->GetAssetType() == &TypeOf(FolderAsset))
            return WString();

        double size = (double)file.size;
        if (size < 1024.0)
            return (String)(int)file.size + " B";

        if (size < 1024.0*1024.0)
            return String::Format("%.1f KB", size/1024.0);

        return String::Format("%.1f MB", size/(1024.0*1024.0));
    }

    WString AssetsIconsScrollArea::GetDateText(const FileInfo& file)
    {
        auto& d = file.editDate;
        return String::Format("%04d-%02d-%02d %02d:%02d", d.mYear, d.mMonth, d.mDay, d.mHour, d.mMinute);
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

        const AssetInfo& info = o2Assets.GetAssetInfo(id);
        if (!info.IsValid())
            return;

        auto assetInfo = Ref(const_cast<AssetInfo*>(&info));

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
            float itemHeight = mItemSample->layout->minHeight + mItemsSpacing.y;

            SetScroll(Vec2F(0.0f, idx/GetItemsInLine()*itemHeight));
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

    const Ref<ContextMenu>& AssetsIconsScrollArea::GetContextMenu() const
    {
        return mContextMenu;
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

        assetIcon->Show();

        auto iconLayer = assetIcon->layer["icon"];
        auto rasterIcon = [&]()
        {
            auto sprite = DynamicCast<Sprite>(iconLayer->GetDrawable());
            if (!sprite)
            {
                sprite = mmake<Sprite>();
                if (auto drawable = iconLayer->GetDrawable())
                    sprite->SetColor(drawable->GetColor());

                iconLayer->SetDrawable(sprite);
            }

            return sprite;
        };

        // Texture previews lay on the checked background, like the image viewers do; the
        // layer is created lazily and sits right below the icon layer
        auto previewBackLayer = assetIcon->FindLayer("previewBack");
        if (!previewBackLayer)
        {
            CommonTextures::Initialize(); // lazy, in case icons are built outside the editor app
            auto backSprite = mmake<Sprite>(CommonTextures::checkedBackground);
            backSprite->SetMode(SpriteMode::Tiled);
            previewBackLayer = assetIcon->AddLayer("previewBack", backSprite, iconLayer->layout,
                                                   iconLayer->GetDepth() - 0.1f);
        }

        auto setPreviewLayout = [&](float width, float height) {
            iconLayer->layout = FitIntoArea(assetIcon->GetPreviewLayout(), width, height);
            previewBackLayer->layout = iconLayer->layout;
            previewBackLayer->SetEnabled(true);
        };

        bool isAtlasWithPages = false;
        if (asset->meta->GetAssetType() == &TypeOf(AtlasAsset))
        {
            AssetRef<AtlasAsset> atlasAsset(asset->path);
            auto& pages = atlasAsset->GetPages();
            isAtlasWithPages = !pages.IsEmpty() && pages[0].GetTexture();

            if (isAtlasWithPages)
            {
                Vec2I pageSize = pages[0].GetTexture()->GetSize();
                setPreviewLayout((float)pageSize.x, (float)pageSize.y);

                auto iconSprite = rasterIcon();
                iconSprite->SetTexture(pages[0].GetTexture());
                iconSprite->SetTextureSrcRect(RectI(Vec2I(), pageSize));
                iconSprite->mode = SpriteMode::Default;
            }
        }

        AssetRef<VectorImageAsset> previewVectorAsset;
        if (asset->meta->GetAssetType() == &TypeOf(VectorImageAsset))
            previewVectorAsset = AssetRef<VectorImageAsset>(asset->path);

        bool isVectorImage = previewVectorAsset && previewVectorAsset->IsValid();
        auto vectorPreviewLayer = assetIcon->FindLayer("vectorPreview");
        if (isVectorImage && !vectorPreviewLayer)
        {
            vectorPreviewLayer = assetIcon->AddLayer("vectorPreview", mmake<VectorSprite>(), iconLayer->layout,
                                                     iconLayer->GetDepth() + 0.05f);
        }

        if (vectorPreviewLayer)
            vectorPreviewLayer->SetEnabled(isVectorImage);

        iconLayer->SetEnabled(!isVectorImage);

        if (asset->meta->GetAssetType() == &TypeOf(ImageAsset))
        {
            AssetRef<ImageAsset> previewSpriteAsset(asset->path);
            setPreviewLayout(previewSpriteAsset->width, previewSpriteAsset->height);

            auto iconSprite = rasterIcon();
            iconSprite->image = previewSpriteAsset;
            iconSprite->mode = SpriteMode::Default;
        }
        else if (isVectorImage)
        {
            setPreviewLayout(previewVectorAsset->width, previewVectorAsset->height);
            vectorPreviewLayer->layout = iconLayer->layout;

            auto vectorSprite = DynamicCast<VectorSprite>(vectorPreviewLayer->GetDrawable());
            vectorSprite->SetImageAsset(previewVectorAsset);
            vectorSprite->SetMode(SpriteMode::Default);
        }
        else if (!isAtlasWithPages)
        {
            auto icon = iconLayer->SetImage(asset->meta->GetAssetType()->InvokeStatic<String>("GetEditorIcon"));
            if (auto vectorIcon = DynamicCast<VectorSprite>(icon))
                vectorIcon->SetMode(SpriteMode::FixedAspect);
            else if (auto rasterIcon = DynamicCast<Sprite>(icon))
                rasterIcon->SetMode(SpriteMode::FixedAspect);
            iconLayer->layout = assetIcon->GetIconLayout();
            previewBackLayer->SetEnabled(false);
        }

        SetupMediaIcon(assetIcon, asset);
        assetIcon->SetAssetInfo(asset);

        if (mViewMode == ViewMode::Table)
        {
            auto& file = GetFileInfo(asset);
            assetIcon->SetColumns(GetTypeText(*asset), GetSizeText(*asset, file), GetDateText(file));
        }

        assetIcon->SetState("halfHide", mCuttingAssets.Contains([&](auto x) { return x.first == asset->meta->ID(); }));
        assetIcon->SetSelectionGroup(Ref(this));
        assetIcon->SetSelected(mSelectedAssets.Contains(Ref(asset)));
        assetIcon->SetDragOnlySelected(true);
        assetIcon->mOwner = Ref(this);
    }

    void AssetsIconsScrollArea::SetupMediaIcon(const Ref<AssetIcon>& icon, const Ref<AssetInfo>& info)
    {
        auto type = info->meta->GetAssetType();
        bool isVideo = type == &TypeOf(VideoAsset);
        bool grid = mViewMode == ViewMode::Grid;
        auto& theme = PipelineTheme::Get();
        auto iconLayer = icon->layer["icon"];

        Layout previewArea = icon->GetPreviewLayout();
        Vec2F center = (previewArea.offsetMin + previewArea.offsetMax)*0.5f;
        auto areaAround = [&](const Vec2F& size) {
            return Layout(previewArea.anchorMin, previewArea.anchorMax, center - size*0.5f, center + size*0.5f);
        };

        // A video shows its frame, wider than the previews of the images, with a play badge in the grid
        TextureRef frame = isVideo ? MediaPreview::GetVideoThumbnail(AssetRef<VideoAsset>(info->meta->ID())) : TextureRef();
        auto badge = icon->FindLayer("mediaBadge");
        if (frame)
        {
            Vec2I size = frame->GetSize();
            Vec2F areaSize = previewArea.offsetMax - previewArea.offsetMin;
            Layout frameLayout = FitIntoArea(areaAround(Vec2F(grid ? areaSize.x*1.5f : areaSize.x, areaSize.y)), (float)size.x, (float)size.y);
            iconLayer->layout = frameLayout;

            auto sprite = DynamicCast<Sprite>(iconLayer->GetDrawable());
            if (!sprite)
            {
                sprite = mmake<Sprite>();
                iconLayer->SetDrawable(sprite);
            }

            sprite->SetTexture(frame);
            sprite->SetTextureSrcRect(RectI(Vec2I(), size));
            sprite->SetMode(SpriteMode::Default);
            iconLayer->SetEnabled(true);

            if (auto back = icon->FindLayer("previewBack"))
                back->SetEnabled(false);

            if (grid)
            {
                // The badge has a light outline, seen over any frame
                if (!badge)
                {
                    auto outline = mmake<PipelineRoundedRect>();
                    outline->radius = 7.0f;
                    outline->roundBottom = true;
                    outline->color = Color4(255, 255, 255, 240);
                    badge = icon->AddLayer("mediaBadge", outline, Layout(), iconLayer->GetDepth() + 0.1f);

                    auto circle = mmake<PipelineRoundedRect>();
                    circle->radius = 5.5f;
                    circle->roundBottom = true;
                    circle->color = theme.accent;
                    auto fill = badge->AddChildLayer("fill", circle, Layout::BothStretch(1.5f, 1.5f, 1.5f, 1.5f));

                    auto glyph = mmake<VectorSprite>("ui/pipeline/al_play.svg");
                    glyph->color = theme.accentInk;
                    fill->AddChildLayer("glyph", glyph, Layout::BothStretch(3.0f, 2.5f, 2.0f, 2.5f));
                }

                Vec2F corner(frameLayout.offsetMax.x, frameLayout.offsetMin.y);
                badge->layout = Layout(frameLayout.anchorMin, frameLayout.anchorMax, corner + Vec2F(-15, 1), corner + Vec2F(-1, 15));
            }
        }

        if (badge)
            badge->SetEnabled(frame && grid);
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
        Focus();

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
        UpdateHover(nullptr);

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
            mDragIconSize = icon->layout->GetWorldRect().Size();

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
        mDragIcon->layout->SetRect(RectF(o2Input.GetCursorPos() - mDragIconSize*0.5f + mDragOffset,
                                         o2Input.GetCursorPos() + mDragIconSize*0.5f + mDragOffset));

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

            Vector<MoveAssetAction::Entry> entries;
            for (auto& sel : mSelectedAssets)
            {
                MoveAssetAction::Entry e;
                e.uid = sel->meta->ID();
                e.filename = o2FileSystem.GetPathWithoutDirectories(sel->path);
                e.originalParent = o2FileSystem.GetParentPath(sel->path);
                entries.Add(e);
            }
            DeselectAllAssets();

            if (!entries.IsEmpty())
            {
                auto action = mmake<MoveAssetAction>(entries, destPath);
                action->Redo();
                o2EditorSceneWindow.DoneAction(action);
            }
        }
    }

    void AssetsIconsScrollArea::OnDroppedFromSceneTree(const Ref<SceneHierarchyTree>& sceneTree)
    {
        String destPath = mCurrentPath;

        auto iconUnderCursor = GetIconUnderPoint(o2Input.GetCursorPos());
        if (iconUnderCursor && iconUnderCursor->GetAssetInfo()->meta->GetAssetType() == &TypeOf(FolderAsset))
            destPath = iconUnderCursor->GetAssetInfo()->path;

        Vector<SceneUID> actorIds;
        for (auto& object : sceneTree->GetSelectedObjects())
        {
            if (auto actor = DynamicCast<Actor>(object))
                actorIds.Add(actor->GetID());
        }

        if (actorIds.IsEmpty())
            return;

        auto action = mmake<CreateAssetFromSceneAction>(actorIds, destPath);
        action->Redo();
        o2EditorSceneWindow.DoneAction(action);

        Vector<String> newAssets;
        for (auto& e : action->entries)
            if (!e.createdPath.IsEmpty())
                newAssets.Add(e.createdPath);

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

        o2EditorSceneWindow.DoneAction(createAction);
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
    {
        if (!mIsDraggingIcons)
            UpdateHover(GetIconUnderPoint(cursor.position));
    }

    void AssetsIconsScrollArea::OnCursorExit(const Input::Cursor& cursor)
    {
        UpdateHover(nullptr);
    }

    void AssetsIconsScrollArea::UpdateHover(const Ref<AssetIcon>& icon)
    {
        mHoveredIcon = icon;
        if (icon)
            mTargetHoverRect = mHoverLayout.Calculate(icon->layout->worldRect);

        if (auto hoverState = state["hover"])
            hoverState->SetState(icon != nullptr);
        else
            mHoverDrawable->SetEnabled(icon != nullptr);
    }

    void AssetsIconsScrollArea::MoveScrollPosition(const Vec2F& delta)
    {
        GridLayoutScrollArea::MoveScrollPosition(delta);

        mTargetHoverRect += delta;
        mCurrentHoverRect += delta;
    }

    void AssetsIconsScrollArea::SetHoverDrawable(const Ref<IRectDrawable>& drawable)
    {
        mHoverDrawable = drawable;
    }

    const Ref<IRectDrawable>& AssetsIconsScrollArea::GetHoverDrawable() const
    {
        return mHoverDrawable;
    }

    void AssetsIconsScrollArea::SetHoverLayout(const Layout& layout)
    {
        mHoverLayout = layout;
    }

    const Ref<AssetIcon>& AssetsIconsScrollArea::GetHoveredIcon() const
    {
        return mHoveredIcon;
    }

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

        mContextMenu->AddItem("Show in folder", [&]() { OnContextShowInExplorerPressed(); });
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("New folder", [&]() { CreateAsset(&TypeOf(FolderAsset)); });

        InitializeCreateContext();

        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Copy", [&]() { OnContextCopyPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_C, VK_CTRL_CMD}));
        mContextMenu->AddItem("Cut", [&]() { OnContextCutPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_X, VK_CTRL_CMD}));
        mContextMenu->AddItem("Paste", [&]() { OnContextPastePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_V, VK_CTRL_CMD}));
        mContextMenu->AddItem("Delete", [&]() { OnContextDeletePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_DELETE}));
        mContextMenu->AddItem("Rename", [&]() { RenameSelectedAsset(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_F2}));

        onFocused = [&]() { mContextMenu->SetItemsMaxPriority(); };
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

        // Only the icons of the grid are renamed by a double click on the name; the rows open the asset
        auto assetNameLabel = icon->GetChildWidget("nameLabel");
        if (mViewMode == ViewMode::Grid && assetNameLabel && assetNameLabel->IsUnderPoint(o2Input.cursorPos))
            RenameAsset(icon);
        else
        {
            if (iconAssetInfo.meta->GetAssetType() == &TypeOf(FolderAsset))
                o2EditorAssets.OpenFolder(iconAssetInfo.path);
            else
                o2EditorAssets.OpenAssetInEditor(iconAssetInfo);
        }
    }

    void AssetsIconsScrollArea::RenameSelectedAsset()
    {
        if (mSelectedAssets.IsEmpty())
            return;

        auto info = mSelectedAssets.Last();
        int idx = mAssetInfos.IndexOf(info);
        if (idx < 0)
            return;

        ScrollTo((void*)&mAssetInfos[idx]);
        UpdateVisibleItems();

        if (auto icon = FindVisibleIcon(info))
            RenameAsset(icon);
    }

    void AssetsIconsScrollArea::RenameAsset(const Ref<AssetIcon>& icon)
    {
        AssetInfo iconAssetInfo = *icon->GetAssetInfo();
        {
            auto name = o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(iconAssetInfo.path));
            StartAssetRenaming(icon, name, [=](const String& name)
                               {
                                   String extension = o2FileSystem.GetFileExtension(iconAssetInfo.path);
                                   String newName = extension.IsEmpty() ? name : name + "." + extension;
                                   String originalName = o2FileSystem.GetPathWithoutDirectories(iconAssetInfo.path);

                                   auto action = mmake<RenameAssetAction>(iconAssetInfo.meta->ID(), originalName, newName);
                                   action->Redo();
                                   o2EditorSceneWindow.DoneAction(action);

                                   String parentPath = o2FileSystem.GetParentPath(iconAssetInfo.path);
                                   String newPath = parentPath.IsEmpty() ? newName : parentPath + "/" + newName;
                                   o2EditorAssets.SelectAsset(newPath);
                               });
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

    void AssetsIconsScrollArea::SetHighlightDrawable(const Ref<IRectDrawable>& drawable)
    {
        mHighlightSprite = drawable;
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
    }

    const Ref<IRectDrawable>& AssetsIconsScrollArea::GetHighlightDrawable() const
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

    void AssetsIconsScrollArea::SetSelectingDrawable(const Ref<IRectDrawable>& drawable)
    {
        mSelectionSprite = drawable;
    }

    const Ref<IRectDrawable>& AssetsIconsScrollArea::GetSelectingDrawable() const
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

    void AssetsIconsScrollArea::OnContextShowInExplorerPressed()
    {
        if (mSelectedAssets.Count() > 0)
            o2EditorAssets.OpenAsset(mSelectedAssets.Last()->meta->ID());
    }

    void AssetsIconsScrollArea::CreateAsset(const Type* assetType)
    {
        String newAssetName = "New " + GetSmartName(assetType->GetName()).ReplacedAll(": ", " ");

        auto extesions = assetType->InvokeStatic<Vector<String>>("GetFileExtensions");
        auto extension = !extesions.IsEmpty() ? extesions[0] : String("");
        String initialPath = mCurrentPath.IsEmpty() ? newAssetName : mCurrentPath + "/" + newAssetName;
        if (!extension.IsEmpty())
            initialPath += "." + extension;

        auto objectType = dynamic_cast<const ObjectType*>(assetType);

        mNewAsset = DynamicCast<Asset>(objectType->CreateSampleRef());
        mNewAsset->SetPath(initialPath);
        mAssetInfos.Add(mmake<AssetInfo>(mNewAsset->GetInfo()));

        SortAssetInfos();
        OnItemsUpdated(true);

        int newIdx = mAssetInfos.IndexOf([&](const Ref<AssetInfo>& x) { return x->path == initialPath; });
        if (newIdx < 0)
            return;

        Ref<AssetInfo> newAssetInfo = mAssetInfos[newIdx];
        ScrollTo((void*)&mAssetInfos[newIdx]);

        UpdateVisibleItems();
        auto icon = FindVisibleIcon(newAssetInfo);
        if (!icon)
            return;

        StartAssetRenaming(icon, newAssetName,
                           [&, extension, assetType](const String& name)
                           {
                               String fileName = extension.IsEmpty() ? name : name + "." + extension;
                               String path = (!mCurrentPath.IsEmpty() ? mCurrentPath + "/" + fileName : fileName);

                               mNewAsset = nullptr;

                               auto action = mmake<CreateAssetAction>(*assetType, mCurrentPath, fileName);
                               action->Redo();
                               o2EditorSceneWindow.DoneAction(action);

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

    void AssetsIconsScrollArea::InstantiateAssetIntoScene(const AssetInfo& assetInfo)
    {
        ForcePopEditorScopeOnStack scope;

        auto actor = InstantiateAsset(assetInfo);
        if (!actor)
            return;

        actor->name = o2FileSystem.GetFileNameWithoutExtension(
            o2FileSystem.GetPathWithoutDirectories(assetInfo.path));

        // The clone waits in the scene's added-entities queue; flush it so the
        // object is really on scene before registering the undo action
        o2Scene.UpdateAddedEntities();

        auto object = DynamicCast<SceneEditableObject>(actor);
        auto parent = object->GetEditableParent();
        auto parentChilds = parent ? parent->GetEditableChildren() : o2Scene.GetRootEditableObjects();
        int idx = parentChilds.IndexOf(object);
        auto prevObject = idx > 0 ? parentChilds[idx - 1] : nullptr;

        auto createAction = mmake<CreateAction>(Vector<Ref<SceneEditableObject>>{ object }, parent, prevObject);
        o2EditorSceneWindow.DoneAction(createAction);

        o2EditorTree.GetSceneTree()->UpdateNodesView(true);
        o2EditorTree.SetSelectedObjects({ object });
        o2EditorTree.HighlightObjectTreeNode(object);
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

ENUM_META(Editor::AssetsIconsScrollArea::ViewMode, Editor__AssetsIconsScrollArea__ViewMode)
{
    ENUM_ENTRY(Grid);
    ENUM_ENTRY(List);
    ENUM_ENTRY(Table);
}
END_ENUM_META;

ENUM_META(Editor::AssetsIconsScrollArea::SortBy, Editor__AssetsIconsScrollArea__SortBy)
{
    ENUM_ENTRY(Modified);
    ENUM_ENTRY(Name);
    ENUM_ENTRY(Size);
    ENUM_ENTRY(Type);
}
END_ENUM_META;

DECLARE_CLASS(Editor::AssetsIconsScrollArea, Editor__AssetsIconsScrollArea);
// --- END META ---
