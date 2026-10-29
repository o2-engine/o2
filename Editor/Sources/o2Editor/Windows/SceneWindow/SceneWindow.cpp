#include "o2Editor/stdafx.h"
#include "SceneWindow.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2Editor/Windows/SceneWindow/GizmosPopup.h"
#include "o2Editor/Windows/SceneWindow/LayersPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/EditorConfig.h"
#include "o2/Render/VectorSprite.h"

DECLARE_SINGLETON(Editor::SceneWindow);

namespace Editor
{
    SceneWindow::SceneWindow(RefCounter* refCounter) :
        Singleton<SceneWindow>(refCounter), IAssetEditorWindow(refCounter)
    {
        InitializeWindow();
    }

    SceneWindow::SceneWindow(RefCounter* refCounter, const SceneWindow& other) :
        Singleton<SceneWindow>(refCounter), IAssetEditorWindow(refCounter, other)
    {
        InitializeWindow();
    }

    SceneWindow::~SceneWindow()
    {}

    Ref<RefCounterable> SceneWindow::CastToRefCounterable(const Ref<SceneWindow>& ref)
    {
        return DynamicCast<Singleton<SceneWindow>>(ref);
    }

    void SceneWindow::InitializeWindow()
    {
        IAssetEditorWindow::InitializeWindow();

        mWindow->caption = "Scene";
        mWindow->name = "scene window";
        mWindow->SetIcon(mmake<VectorSprite>("ui/UI4_scene_icon.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-2, 2)));

        mEditWidget = mmake<SceneEditScreen>();
        *mEditWidget->layout = WidgetLayout::BothStretch(0, 0, 0, 21);
        mWindow->AddChild(mEditWidget);

        mLayersButton = o2UI.CreateWidget<Button>("panel down");
        mLayersButton->caption = "Layers";
        *mLayersButton->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 100, 0);
        mUpPanel->AddChild(mLayersButton);

        mLayersPopup = mmake<LayersPopup>();
        mLayersButton->AddChild(mLayersPopup);

        mLayersButton->onClick = [&]() { mLayersPopup->Show(mLayersButton->layout->worldLeftBottom); };

        mGizmosButton = o2UI.CreateWidget<Button>("panel down");
        mGizmosButton->caption = "Gizmos";
        *mGizmosButton->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 70, 240);
        mUpPanel->AddChild(mGizmosButton);

        mGizmosPopup = mmake<GizmosPopup>();
        mGizmosButton->AddChild(mGizmosPopup);

        mGizmosButton->onClick = [&]() { mGizmosPopup->Show(mGizmosButton->layout->worldLeftBottom); };

        mView3DButton = o2UI.CreateWidget<Button>("panel down");
        mView3DButton->caption = "3D";
        *mView3DButton->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 50, 100);
        mUpPanel->AddChild(mView3DButton);

        mView3DButton->onClick = [&]() { mEditWidget->SetView3DMode(!mEditWidget->IsView3DMode()); };
        mEditWidget->onView3DModeChanged = [&](bool enabled) {
            mView3DButton->caption = enabled ? WString("2D") : WString("3D");
        };

        // A custom scene camera pipeline (offscreen passes, screen shaders) can distort the edit view: every mode
        // but the scene camera one draws through the default forward 3D/2D pipeline
        mRenderModeButton = o2UI.CreateWidget<Button>("panel down");
        mRenderModeButton->name = "render mode";
        mRenderModeButton->caption = SceneEditScreen::GetRenderModeName(mEditWidget->GetRenderMode());
        mRenderModeButton->layout->minWidth = 130;
        mUpPanel->AddChild(mRenderModeButton);

        mRenderModeMenu = o2UI.CreateWidget<ContextMenu>();
        mRenderModeMenu->name = "render modes";
        for (auto mode : { SceneRenderMode::Camera, SceneRenderMode::Stable, SceneRenderMode::Unlit,
                           SceneRenderMode::Wireframe, SceneRenderMode::ShadedWireframe, SceneRenderMode::Overdraw })
        {
            auto item = mRenderModeMenu->AddToggleItem(SceneEditScreen::GetRenderModeName(mode),
                                                       mEditWidget->GetRenderMode() == mode, [](bool) {});
            item->onChecked = [this, mode](bool) { mEditWidget->SetRenderMode(mode); };
        }

        mWindow->AddChild(mRenderModeMenu);

        mRenderModeButton->onClick = [&]() { mRenderModeMenu->Show(mRenderModeButton->layout->GetWorldLeftBottom()); };
        mEditWidget->onRenderModeChanged = [&](SceneRenderMode mode) { OnRenderModeChanged(mode); };
    }

    void SceneWindow::OnRenderModeChanged(SceneRenderMode mode)
    {
        WString name = SceneEditScreen::GetRenderModeName(mode);
        mRenderModeButton->caption = name;

        // The item widgets are built when the menu shows: the items keep the checks till then
        for (auto& item : mRenderModeMenu->GetItems())
        {
            if (!item->checkable)
                continue;

            item->checked = item->text == name;
            if (auto widget = item->widget.Lock())
                widget->SetChecked(item->checked);
        }
    }

    String SceneWindow::GetWindowTitle() const
    {
        return "Scene";
    }

    void SceneWindow::PostInitializeWindow()
    {
        o2EditorSceneScreen.BindSceneTree();
    }

    const Type& SceneWindow::GetAssetType() const
    {
        return TypeOf(SceneAsset);
    }

    bool SceneWindow::IsCreateNewAssetAtStartupEnabled() const
    {
        return false;
    }

    void SceneWindow::OnStartEditingAsset()
    {
        if (auto sceneAsset = DynamicCast<SceneAsset>(mEditingAsset.Lock()))
        {
            ForcePopEditorScopeOnStack scope;
            sceneAsset->Load();

            if (EditorConfig::IsSingletonInitialzed())
                o2EditorConfig.projectConfig.lastLoadedScene = sceneAsset->GetPath();
        }
    }

    void SceneWindow::OnCompletedEditingAsset()
    {
    }

    AssetRef<Asset> SceneWindow::CreateAssetInstance()
    {
        return AssetRef<SceneAsset>(mmake<SceneAsset>());
    }

    void SceneWindow::OnAssetSaved()
    {
    }

    void SceneWindow::OnActionDone(const Ref<IAction>& action)
    {
        OnAssetChanged();
    }

    void SceneWindow::OnActionUndo(const Ref<IAction>& action)
    {
        OnAssetChanged();
    }

}
// --- META ---

DECLARE_CLASS(Editor::SceneWindow, Editor__SceneWindow);
// --- END META ---
