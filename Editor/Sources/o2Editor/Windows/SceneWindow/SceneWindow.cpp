#include "o2Editor/EditorApplication.h"
#include "o2Editor/stdafx.h"
#include "SceneWindow.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2Editor/Windows/SceneWindow/LayersPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    SceneWindow::SceneWindow() :
        IAssetEditorWindow()
    {
        InitializeWindow();
    }

    SceneWindow::SceneWindow(const SceneWindow& other) :
        IAssetEditorWindow(other)
    {
        InitializeWindow();
    }

    SceneWindow::~SceneWindow()
    {}

    void SceneWindow::InitializeWindow()
    {
        IAssetEditorWindow::InitializeWindow();

        mWindow->caption = "Scene";
        mWindow->name = "scene window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_scene_icon.png"));
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

    void SceneWindow::OnStartEditingAsset()
    {
        if (auto sceneAsset = DynamicCast<SceneAsset>(mEditingAsset.Lock()))
        {
            ForcePopEditorScopeOnStack scope;
            sceneAsset->Load();

            o2EditorConfig.projectConfig.mLastLoadedScene = sceneAsset->GetPath();
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

}
// --- META ---

DECLARE_CLASS(Editor::SceneWindow, Editor__SceneWindow);
// --- END META ---
