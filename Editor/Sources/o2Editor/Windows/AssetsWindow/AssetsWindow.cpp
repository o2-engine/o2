#include "o2Editor/stdafx.h"
#include "AssetsWindow.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/AssetsTree.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/GridLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/FoldersTree.h"
#include "o2Editor/EditorConfig.h"

DECLARE_SINGLETON(Editor::AssetsWindow);

namespace Editor
{
    AssetsWindow::AssetsWindow(RefCounter* refCounter):
        Singleton<AssetsWindow>(refCounter), IEditorWindow(refCounter)
    {
        if (Application::IsSingletonInitialzed() && o2Application.IsReady())
            InitializeWindow();
    }

    AssetsWindow::~AssetsWindow()
    {}

    void AssetsWindow::InitializeWindow()
    {
        o2Assets.onAssetsRebuilt += THIS_FUNC(OnAssetsRebuilt);

        mWindow->caption = "Assets";
        mWindow->name = "assets window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_folder_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(-2, 0, 0, 18));
        mWindow->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 18));

        InitializeUpPanel();
        InitializeDownPanel();
        InitializeFoldersTree();
        InitializeFoldersTreeVisibleState();
        InitializeFoldersTreeSeparator();

        OpenFolder("");
    }

    void AssetsWindow::InitializeFoldersTreeSeparator()
    {
        auto separatorLayer = mFoldersTree->FindLayer("separator");

        mSeparatorHandle = mmake<CursorEventsArea>();
        mSeparatorHandle->isUnderPoint = [=](const Vec2F& point) {
            RectF rt = separatorLayer->GetDrawable()->GetRect();
            rt.left -= 2; rt.right += 2;
            return rt.IsInside(point);
        };

        mSeparatorCoef = 0.0f; // o2EditorConfig.GetProjectUserData()["layout/assetsWindow/separator_coef"];

        if (mSeparatorCoef < FLT_EPSILON)
            mSeparatorCoef = 0.2f;

        mFoldersTree->layout->anchorRight = mSeparatorCoef;
        mAssetsGridScroll->layout->anchorLeft = mSeparatorCoef;

        mSeparatorHandle->onMoved = [&](const Input::Cursor& cursor) {
            float anchorDelta = cursor.delta.x / mWindow->layout->width;
            mFoldersTree->layout->anchorRight += anchorDelta;
            mAssetsGridScroll->layout->anchorLeft += anchorDelta;

//             auto& userData = o2EditorConfig.GetProjectUserData();
//             userData["layout/assetsWindow/separator_coef"].Set(mFoldersTree->layout->GetAnchorRight());
        };

        mAssetsGridScroll->onDraw += [&]() { mSeparatorHandle->OnDrawn(); };

        mSeparatorHandle->cursorType = CursorType::SizeWE;
    }

    void AssetsWindow::InitializeFoldersTreeVisibleState()
    {
        mFoldersTreeShowCoef = 1.0f;
        mFoldersTreeVisible = true;

        mFoldersTreeShowAnim->SetTarget(this);
        mFoldersTreeShowAnim->SetClip(AnimationClip::EaseInOut("mFoldersTreeShowCoef", 0.0f, 1.0f, 0.4f));
        mFoldersTreeShowAnim->GoToEnd();

        mFoldersTreeShowAnim->onUpdate = [&](float dt) {
            mFoldersTree->layout->anchorRight = mSeparatorCoef*mFoldersTreeShowCoef;
            mAssetsGridScroll->layout->anchorLeft = mSeparatorCoef*mFoldersTreeShowCoef;

            float disableCoef = 0.2f;
            if (mFoldersTreeShowCoef < disableCoef && mFoldersTree->IsEnabled())
                mFoldersTree->Hide();

            if (mFoldersTreeShowCoef >= disableCoef && !mFoldersTree->IsEnabled())
                mFoldersTree->Show();
        };
    }

    void AssetsWindow::InitializeFoldersTree()
    {
        mFoldersTree = mmake<AssetsFoldersTree>();
        *mFoldersTree->layout = WidgetLayout(0.0f, 1.0f, 0.5f, 0.0f, 5.0f, -18.0f, 0.0f, 18.0f);

        mFoldersTree->AddLayer("separator", mmake<Sprite>("ui/UI4_Ver_separator.png"),
                               Layout::VerStretch(HorAlign::Right, -2, 0, 5, 0));
        
        mFoldersTree->UpdateView();
        mWindow->AddChild(mFoldersTree);

        // assets scroll & grid
        mAssetsGridScroll = o2UI.CreateWidget<AssetsIconsScrollArea>();
        *mAssetsGridScroll->layout = WidgetLayout(0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -18.0f, 0.0f, 18.0f);
        mWindow->AddChild(mAssetsGridScroll);

        mAssetsGridScroll->onAssetsSelected = [&](const Vector<String>& assetsPaths) {
            if (!assetsPaths.IsEmpty())
                mSelectedAssetPathLabel->text = assetsPaths.Last();
            else
                mSelectedAssetPathLabel->text = mAssetsGridScroll->GetViewingPath();
        };
    }

    void AssetsWindow::InitializeDownPanel()
    {
        auto downPanel = mmake<Widget>();
        downPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_down_back.png"),
                            Layout::BothStretch(-4, -5, -4, -5));
        *downPanel->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 20, 0);

        auto showTreeBtn = o2UI.CreateWidget<Button>("menu tree");
        *showTreeBtn->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(0, 0));
        showTreeBtn->onClick += THIS_FUNC(OnShowTreePressed);
        downPanel->AddChild(showTreeBtn);

        mSelectedAssetPathLabel = o2UI.CreateWidget<Label>();
        *mSelectedAssetPathLabel->layout = WidgetLayout::BothStretch(20, 0, 0, 0);
        mSelectedAssetPathLabel->text = "o2/Assets/Folder/Image.png";
        mSelectedAssetPathLabel->horOverflow = Label::HorOverflow::Dots;
        mSelectedAssetPathLabel->horAlign = HorAlign::Left;
        downPanel->AddChild(mSelectedAssetPathLabel);

        mWindow->AddChild(downPanel);
    }

    void AssetsWindow::InitializeUpPanel()
    {
        auto upPanel = mmake<Widget>();
        upPanel->name = "up panel";
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        upPanel->AddLayer("back", mmake<Sprite>("ui/UI4_square_field.png"), Layout::BothStretch(-4, -4, -5, -5));

        auto searchButton = o2UI.CreateWidget<Button>("search");
        *searchButton->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(1, 1));
        upPanel->AddChild(searchButton);

        mFilterButton = o2UI.CreateWidget<Button>("menu filter");
        *mFilterButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0));
        mFilterButton->onClick += THIS_FUNC(OnMenuFilterPressed);
        upPanel->AddChild(mFilterButton);

        mSearchEditBox = o2UI.CreateWidget<EditBox>("backless");
        *mSearchEditBox->layout = WidgetLayout::BothStretch(19, 2, 21, -2);
        mSearchEditBox->onChanged += THIS_FUNC(OnSearchEdited);
        upPanel->AddChild(mSearchEditBox);

        mWindow->AddChild(upPanel);
    }

    void AssetsWindow::Update(float dt)
    {
        IEditorWindow::Update(dt);
        mFoldersTreeShowAnim->Update(dt);
    }

    void AssetsWindow::SelectAsset(const UID& id)
    {
        String assetPath = o2Assets.GetAssetPath(id);
        String folder = o2FileSystem.GetParentPath(assetPath);

        if (GetOpenedFolderPath() != folder)
            OpenFolder(folder);

        mAssetsGridScroll->SelectAsset(id);
    }

    void AssetsWindow::SelectAsset(const String& path)
    {
        const UID& id = o2Assets.GetAssetId(path);
        String folder = o2FileSystem.GetParentPath(path);

        if (GetOpenedFolderPath() != folder)
            OpenFolder(folder);

        mAssetsGridScroll->SelectAsset(id);
    }

    void AssetsWindow::SelectAsset(const Vector<UID>& ids)
    {
        for (auto& id : ids)
            SelectAsset(id);
    }

    void AssetsWindow::SelectAssets(const Vector<String>& paths)
    {
        for (auto& path : paths)
            SelectAsset(path);
    }

    void AssetsWindow::OpenAsset(const UID& id)
    {
        OpenAsset(o2Assets.GetAssetPath(id));
    }

    void AssetsWindow::OpenAsset(const String& path)
    {
        String fullPath = o2FileSystem.GetParentPath(o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + path);
#if defined PLATFORM_WINDOWS
        ShellExecute(NULL, "explore", fullPath, NULL, NULL, SW_SHOWNORMAL);
#endif
    }

    void AssetsWindow::OpenAndEditAsset(const UID& id)
    {
        OpenAndEditAsset(o2Assets.GetAssetPath(id));
    }

    void AssetsWindow::OpenAndEditAsset(const String& path)
    {
        String fullPath = o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + path;
#if defined PLATFORM_WINDOWS
        ShellExecute(NULL, "edit", fullPath, NULL, NULL, SW_SHOWNORMAL);
#endif
    }

    void AssetsWindow::DeselectAssets()
    {
        mAssetsGridScroll->DeselectAllAssets();
    }

    const Vector<Ref<AssetInfo>>& AssetsWindow::GetSelectedAssets() const
    {
        return mAssetsGridScroll->GetSelectedAssets();
    }

    String AssetsWindow::GetOpenedFolderPath() const
    {
        return mAssetsGridScroll->GetViewingPath();
    }

    void AssetsWindow::OpenFolder(const String& path)
    {
        mAssetsGridScroll->SetViewingPath(path);
        mFoldersTree->SelectAndExpandFolder(path);
        mSelectedAssetPathLabel->text = path;
    }

    void AssetsWindow::ShowAssetIcon(const UID& id)
    {
        String assetPath = o2Assets.GetAssetPath(id);
        String folder = o2FileSystem.GetParentPath(assetPath);

        ShowAssetIcon(folder, id);
    }

    void AssetsWindow::ShowAssetIcon(const String& path)
    {
        UID assetId = o2Assets.GetAssetId(path);
        String folder = o2FileSystem.GetParentPath(path);

        ShowAssetIcon(folder, assetId);
    }

    void AssetsWindow::ShowAssetIcon(const String& folder, UID assetId)
    {
        if (GetOpenedFolderPath() != folder)
            OpenFolder(folder);

        mAssetsGridScroll->HighlightAsset(assetId);
    }

    void AssetsWindow::CopyAssets(const Vector<String>& assetsPaths)
    {
        mCuttingAssets.Clear();
        mAssetsGridScroll->UpdateCuttingAssets();

        Vector<WString> paths;
        for (auto& path : assetsPaths)
            paths.Add(o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + path);

        Clipboard::CopyFiles(paths);
    }

    void AssetsWindow::CopyAssetFolder(const String& src, const String& dst)
    {
        o2FileSystem.FolderCreate(dst);

        FolderInfo info = o2FileSystem.GetFolderInfo(src);
        info.ClampPathNames();

        for (auto& file : info.files)
        {
            if (!file.path.EndsWith(".meta"))
                o2FileSystem.FileCopy(src + "/" + file.path, dst + "/" + file.path);
        }

        for (auto& folder : info.folders)
            CopyAssetFolder(src + "/" + folder.path, dst + "/" + folder.path);
    }

    void AssetsWindow::CutAssets(const Vector<String>& assetsPaths)
    {
        mCuttingAssets.Clear();

        Vector<WString> paths;
        for (auto& path : assetsPaths)
        {
            String fullPath = o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + path;
            mCuttingAssets.Add(Pair<UID, String>(o2Assets.GetAssetId(path), fullPath));
            paths.Add(fullPath);
        }

        Clipboard::CopyFiles(paths);

        mAssetsGridScroll->UpdateCuttingAssets();
    }

    void AssetsWindow::PasteAssets(const String& targetPath)
    {
        Vector<WString> paths = Clipboard::GetCopyFiles();
        for (auto& path : paths)
        {
            String fileName = o2FileSystem.GetPathWithoutDirectories(path);
            String extension = o2FileSystem.GetFileExtension(fileName);
            String fileNameWithoutExt = o2FileSystem.GetFileNameWithoutExtension(fileName);

            bool isFolder = extension.IsEmpty();

            String copyFileName = o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + targetPath + "/" + fileName;
            bool endsAsCopy = fileNameWithoutExt.EndsWith("copy");
            int i = 0;
            while (o2FileSystem.IsFileExist(copyFileName))
            {
                copyFileName = o2Application.GetBinPath() + "/" + o2Assets.GetAssetsPath() + targetPath + "/" +
                    fileNameWithoutExt;

                if (!endsAsCopy)
                    copyFileName += " copy";

                if (i > 0)
                    copyFileName += (String)(i + 1) + "." + extension;

                if (!isFolder)
                    copyFileName += "." + extension;

                i++;
            }

            if (mCuttingAssets.Contains([&](auto x) { return x.second == (String)path; }))
            {
                o2FileSystem.FileMove(path, copyFileName);
                o2FileSystem.FileMove(path + ".meta", copyFileName + ".meta");
            }
            else
            {
                if (!isFolder)
                    o2FileSystem.FileCopy(path, copyFileName);
                else
                    CopyAssetFolder(path, copyFileName);
            }
        }

        mCuttingAssets.Clear();
        mAssetsGridScroll->UpdateCuttingAssets();

        o2Assets.RebuildAssets();
    }

    void AssetsWindow::DeleteAssets(const Vector<String>& assetsPaths)
    {
        mCuttingAssets.Clear();
        mAssetsGridScroll->UpdateCuttingAssets();

        for (auto& path : assetsPaths)
            o2Assets.RemoveAsset(path);

        o2Assets.RebuildAssets();
    }

    Ref<Sprite> AssetsWindow::GetAssetIconSprite(const AssetRef<Asset>& asset)
    {
        const Type& type = asset->GetType();

        if (type == TypeOf(ImageAsset))
            return mmake<Sprite>(asset->GetPath());
        else if (type == TypeOf(ActorAsset))
            return mmake<Sprite>("ui/UI4_actor_icon.png");
        else if (type == TypeOf(FolderAsset))
            return mmake<Sprite>("ui/UI4_big_folder_icon.png");
        else if (type == TypeOf(DataAsset))
            return mmake<Sprite>("ui/UI4_big_text_file_icon.png");
        else if (type == TypeOf(AnimationAsset))
            return mmake<Sprite>("ui/UI4_anim_file_icon.png"); 

        return mmake<Sprite>("ui/UI4_big_file_icon.png"); 
    }

    Ref<RefCounterable> AssetsWindow::CastToRefCounterable(const Ref<AssetsWindow>& ref)
    {
        return DynamicCast<Singleton<AssetsWindow>>(ref);
    }

    void AssetsWindow::OnSearchEdited(const WString& search)
    {}

    void AssetsWindow::OnMenuFilterPressed()
    {}

    void AssetsWindow::OnShowTreePressed()
    {
        mFoldersTreeVisible = !mFoldersTreeVisible;

        if (mFoldersTreeVisible)
            mFoldersTreeShowAnim->PlayForward();
        else
            mFoldersTreeShowAnim->PlayBack();
    }

    void AssetsWindow::OnAssetsRebuilt(const Vector<UID>& changedAssets)
    {
        mFoldersTree->UpdateView();
        mFoldersTree->SelectAndExpandFolder(mAssetsGridScroll->GetViewingPath());

        mAssetsGridScroll->UpdateAssetsByCurrentPath();
    }
}
// --- META ---

DECLARE_CLASS(Editor::AssetsWindow, Editor__AssetsWindow);
// --- END META ---
