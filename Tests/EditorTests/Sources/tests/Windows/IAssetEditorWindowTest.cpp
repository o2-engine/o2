#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/IAssetEditorWindow.h"

#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    // Concrete IAssetEditorWindow used by every test below. It picks DataAsset as the
    // editing type (so CreateAssetInstance/CreateFileExtensionMap have something real
    // to reflect on), counts virtual hook calls, and lets each test toggle whether
    // Initialize() should auto-create a starting asset.
    class TestAssetEditorWindow : public IAssetEditorWindow
    {
    public:
        explicit TestAssetEditorWindow(RefCounter* refCounter):
            IAssetEditorWindow(refCounter)
        {}

        const Type& GetAssetType() const override { return TypeOf(DataAsset); }

        bool createAtStartup = false;
        bool IsCreateNewAssetAtStartupEnabled() const override { return createAtStartup; }

        int onStartEditingAsset = 0;
        int onCompletedEditingAsset = 0;
        int onStartEditingComponent = 0;
        int onCompletedEditingComponent = 0;
        int onAssetSaved = 0;
        int onComponentPreviewEnabled = 0;
        int onComponentPreviewDisabled = 0;

        void OnStartEditingAsset() override         { ++onStartEditingAsset; }
        void OnCompletedEditingAsset() override     { ++onCompletedEditingAsset; }
        void OnStartEditingComponent() override     { ++onStartEditingComponent; }
        void OnCompletedEditingComponent() override { ++onCompletedEditingComponent; }
        void OnAssetSaved() override                { ++onAssetSaved; }
        void OnComponentPreviewEnabled() override   { ++onComponentPreviewEnabled; }
        void OnComponentPreviewDisabled() override  { ++onComponentPreviewDisabled; }
    };

    Ref<TestAssetEditorWindow> MakeWindow()
    {
        // IEditorWindow's ctor adds the dockable to EditorUIRoot, which only makes sense
        // inside an editor scope. The fixture spins up EditorUIRoot once; we still need
        // to push the scope per-construction so the widget tree is built as editor UI.
        PushEditorScopeOnStack scope;
        return mmake<TestAssetEditorWindow>();
    }

    AssetRef<DataAsset> MakeDataAsset(const String& path = "")
    {
        AssetRef<DataAsset> asset(mmake<DataAsset>());
        if (!path.IsEmpty())
            asset->SetPath(path);
        return asset;
    }
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_ConstructBuildsUiPanels)
{
    auto wnd = MakeWindow();

    // Dockable host plus the upper button row are created during InitializeWindow.
    ASSERT_NE(wnd->GetWindow(), nullptr);
    auto dock = wnd->GetWindow();
    ASSERT_TRUE(dock->FindChildByTypeAndName<HorizontalLayout>("up panel") != nullptr);
    auto upPanel = dock->FindChildByTypeAndName<HorizontalLayout>("up panel");
    EXPECT_TRUE(upPanel->FindChildByTypeAndName<HorizontalLayout>("buttons panel") != nullptr);

    auto buttons = upPanel->FindChildByTypeAndName<HorizontalLayout>("buttons panel");
    EXPECT_TRUE(buttons->FindChildByTypeAndName<Button>("new asset button") != nullptr);
    EXPECT_TRUE(buttons->FindChildByTypeAndName<Button>("open asset button") != nullptr);
    EXPECT_TRUE(buttons->FindChildByTypeAndName<Button>("save button") != nullptr);
    EXPECT_TRUE(buttons->FindChildByTypeAndName<Button>("save as button") != nullptr);
    EXPECT_TRUE(buttons->FindChildByTypeAndName<Button>("revert button") != nullptr);
    EXPECT_TRUE(buttons->FindChildByType<Toggle>() != nullptr);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_GetAssetTypeFollowsOverride)
{
    auto wnd = MakeWindow();
    EXPECT_EQ(&wnd->GetAssetType(), &TypeOf(DataAsset));
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_NoEditingAssetByDefault)
{
    auto wnd = MakeWindow();
    EXPECT_EQ(wnd->GetEditingAsset(), nullptr);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_EditAssetSetsCurrentAndFiresHook)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("Some/Path.json");

    wnd->EditAsset(asset);

    EXPECT_EQ(wnd->GetEditingAsset(), asset);
    EXPECT_EQ(wnd->onStartEditingAsset, 1);
    EXPECT_EQ(wnd->onCompletedEditingAsset, 0); // nothing was previously open
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_EditAssetNullCreatesInstance)
{
    auto wnd = MakeWindow();

    wnd->EditAsset(AssetRef<Asset>(nullptr));

    auto editing = wnd->GetEditingAsset();
    ASSERT_NE(editing, nullptr);
    // CreateAssetInstance() defaults the path to "Unnamed".
    EXPECT_EQ(editing->GetPath(), "Unnamed");
    EXPECT_EQ(wnd->onStartEditingAsset, 1);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_RebindCallsCompletedThenStart)
{
    auto wnd = MakeWindow();
    auto a = MakeDataAsset("a.json");
    auto b = MakeDataAsset("b.json");

    wnd->EditAsset(a);
    wnd->EditAsset(b);

    EXPECT_EQ(wnd->GetEditingAsset(), b);
    EXPECT_EQ(wnd->onStartEditingAsset, 2);
    EXPECT_EQ(wnd->onCompletedEditingAsset, 1);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_OnAssetChangedMarksAssetDirty)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("p.json");

    wnd->EditAsset(asset);
    ASSERT_FALSE(asset->IsDirty());

    wnd->OnAssetChanged();
    EXPECT_TRUE(asset->IsDirty());
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_OnAssetChangedNoOpWithoutAsset)
{
    auto wnd = MakeWindow();
    // Just must not crash / dereference the empty WeakRef.
    wnd->OnAssetChanged();
    EXPECT_EQ(wnd->GetEditingAsset(), nullptr);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_InitializeCreatesAssetWhenEnabled)
{
    auto wnd = MakeWindow();
    wnd->createAtStartup = true;

    wnd->Initialize();

    auto editing = wnd->GetEditingAsset();
    ASSERT_NE(editing, nullptr);
    EXPECT_EQ(editing->GetPath(), "Unnamed");
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_InitializeSkipsAssetWhenDisabled)
{
    auto wnd = MakeWindow();
    wnd->createAtStartup = false;

    wnd->Initialize();

    EXPECT_EQ(wnd->GetEditingAsset(), nullptr);
    EXPECT_EQ(wnd->onStartEditingAsset, 0);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_UpdateDisablesSaveButtonWhenClean)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("p.json");
    wnd->EditAsset(asset);
    ASSERT_FALSE(asset->IsDirty());

    auto saveButton = wnd->GetWindow()->FindChildByTypeAndName<Button>("save button");
    ASSERT_NE(saveButton, nullptr);

    wnd->Update(0.0f);
    EXPECT_FALSE(saveButton->IsInteractable());
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_UpdateEnablesSaveButtonWhenDirty)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("p.json");
    wnd->EditAsset(asset);

    wnd->OnAssetChanged();
    ASSERT_TRUE(asset->IsDirty());

    auto saveButton = wnd->GetWindow()->FindChildByTypeAndName<Button>("save button");
    ASSERT_NE(saveButton, nullptr);

    wnd->Update(0.0f);
    EXPECT_TRUE(saveButton->IsInteractable());
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_UpdateWithoutAssetDisablesSaveButton)
{
    auto wnd = MakeWindow();

    auto saveButton = wnd->GetWindow()->FindChildByTypeAndName<Button>("save button");
    ASSERT_NE(saveButton, nullptr);

    wnd->Update(0.0f);
    EXPECT_FALSE(saveButton->IsInteractable());
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_RevertNoOpWhenNothingToRevert)
{
    auto wnd = MakeWindow();
    // No asset bound — must just bail out instead of calling Reload() on a null Lock().
    wnd->RevertEditingAsset();
    EXPECT_EQ(wnd->GetEditingAsset(), nullptr);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_SaveNoOpWhenAssetClean)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("clean.json");
    wnd->EditAsset(asset);
    ASSERT_FALSE(asset->IsDirty());

    wnd->SaveEditingAsset();
    // Clean asset → Save() / OnAssetSaved must NOT be triggered.
    EXPECT_EQ(wnd->onAssetSaved, 0);
}

TEST_F(EditorWindowsFixture, IAssetEditorWindow_PreviewToggleNoOpWithoutComponent)
{
    auto wnd = MakeWindow();
    auto asset = MakeDataAsset("p.json");
    wnd->EditAsset(asset);

    // No component → IsComponentPreviewAvailable() returns false; SetComponentPreview
    // should bail out without firing the enable hook.
    wnd->SetComponentPreview(true);
    EXPECT_EQ(wnd->onComponentPreviewEnabled, 0);
}

// NOTE: window-title behaviour (UpdateWindowTitle → mWindow->caption = ...) only
// shows up visibly when the dockable's "back/caption" layer is present, which
// requires UI styles to be loaded — that's not available in the headless editor
// runner. Cover that path in the Rendered tier if/when an editor-window
// rendered fixture exists.
