#include "o2Editor/stdafx.h"
#include "SceneWindow.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2Editor/Windows/SceneWindow/LayersPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SceneWindow::SceneWindow() :
        IEditorWindow()
    {
        InitializeWindow();
    }

    SceneWindow::SceneWindow(const SceneWindow& other) :
        IEditorWindow(other)
    {
        InitializeWindow();
    }

    SceneWindow::~SceneWindow()
    {}

    void SceneWindow::InitializeWindow()
    {
        mWindow->caption = "Scene";
        mWindow->name = "scene window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_scene_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-2, 2)));
        mWindow->SetViewLayout(Layout::BothStretch(-1, 0, 0, 18));

        mEditWidget = mmake<SceneEditScreen>();
        *mEditWidget->layout = WidgetLayout::BothStretch(0, 0, 0, 19);
        mWindow->AddChild(mEditWidget);

        mUpPanel = mmake<Widget>();
        mUpPanel->name = "up panel";
        *mUpPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        mUpPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_back.png"), Layout::BothStretch(-5, -4, -4, -5));
        mWindow->AddChild(mUpPanel);

        mLayersButton = o2UI.CreateWidget<Button>("panel down");
        mLayersButton->caption = "Layers";
        *mLayersButton->layout = WidgetLayout::VerStretch(HorAlign::Right, 0, 0, 100, 0);
        mUpPanel->AddChild(mLayersButton);

        mLayersPopup = mmake<LayersPopup>();
        mUpPanel->AddChild(mLayersPopup);

        mLayersButton->onClick = [&]() { mLayersPopup->Show(mLayersButton->layout->worldLeftBottom); };
    }

    void SceneWindow::PostInitializeWindow()
    {
        o2EditorSceneScreen.BindSceneTree();
    }

}
// --- META ---

DECLARE_CLASS(Editor::SceneWindow, Editor__SceneWindow);
// --- END META ---
