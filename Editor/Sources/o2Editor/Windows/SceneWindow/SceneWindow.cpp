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
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/EditorConfig.h"

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
