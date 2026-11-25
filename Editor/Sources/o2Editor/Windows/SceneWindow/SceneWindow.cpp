#include "o2Editor/stdafx.h"
#include "SceneWindow.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2Editor/Windows/SceneWindow/ViewModeSwitch.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/Windows/SceneWindow/GizmosPopup.h"
#include "o2Editor/Windows/SceneWindow/LayersPopup.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/EditorConfig.h"
#include "o2/Render/VectorGraphics/VectorSprite.h"

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

        // The buttons of the window go left, the scene controls right, the 2D/3D switch at the very end
        auto spacer = mmake<Widget>();
        spacer->name = "spacer";
        mUpPanel->AddChild(spacer);

        auto fixWidth = [](const Ref<Widget>& widget, float width)
        {
            widget->layout->minWidth = width;
            widget->layout->maxWidth = width;
        };

        // A clicked button keeps the focus while its menu is open, and the focus frame is a pixel larger than the
        // button: it looked as if the button grew. The dropdown buttons go without the frame
        auto makeDropdownButton = [&](const String& name, const WString& caption, float width)
        {
            auto button = o2UI.CreateWidget<Button>("panel down");
            button->name = name;
            button->caption = caption;
            if (auto focusFrame = button->FindLayer("focused"))
                focusFrame->SetEnabled(false);

            fixWidth(button, width);
            mUpPanel->AddChild(button);
            return button;
        };

        mLayersButton = makeDropdownButton("layers", "Layers", 72);
        mLayersPopup = mmake<LayersPopup>();
        mLayersButton->AddChild(mLayersPopup);

        mLayersButton->onClick = [&]() { mLayersPopup->Show(mLayersButton->layout->worldLeftBottom); };

        mGizmosButton = makeDropdownButton("gizmos", "Gizmos", 80);

        mGizmosPopup = mmake<GizmosPopup>();
        mGizmosButton->AddChild(mGizmosPopup);

        mGizmosButton->onClick = [&]() { mGizmosPopup->Show(mGizmosButton->layout->worldLeftBottom); };

        // A custom scene camera pipeline (offscreen passes, screen shaders) can distort the edit view: every mode
        // but the scene camera one draws through the default forward 3D/2D pipeline. The button shows the mode by its icon
        mRenderModeButton = makeDropdownButton("render mode", "", 44);
        mRenderModeButton->AddLayer("icon", mmake<VectorSprite>(SceneEditScreen::GetRenderModeIcon(mEditWidget->GetRenderMode())),
                                    Layout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(4, 0)));

        mRenderModeMenu = o2UI.CreateWidget<ContextMenu>();
        mRenderModeMenu->name = "render modes";
        mRenderModeMenu->SetMinFitSize(mRenderModeMenu->GetMinFitSize() + renderModeIconWidth + renderModeCaptionIndent - 20.0f);
        for (auto mode : { SceneRenderMode::Camera, SceneRenderMode::Stable, SceneRenderMode::Unlit,
                           SceneRenderMode::Wireframe, SceneRenderMode::ShadedWireframe, SceneRenderMode::Overdraw })
        {
            auto item = mRenderModeMenu->AddToggleItem(SceneEditScreen::GetRenderModeName(mode),
                                                       mEditWidget->GetRenderMode() == mode, [](bool) {},
                                                       AssetRef<VectorImageAsset>(SceneEditScreen::GetRenderModeIcon(mode)));
            item->onChecked = [this, mode](bool) { mEditWidget->SetRenderMode(mode); };
        }

        mWindow->AddChild(mRenderModeMenu);

        mRenderModeButton->onClick = [&]() { ShowRenderModeMenu(); };
        mEditWidget->onRenderModeChanged = [&](SceneRenderMode mode) { OnRenderModeChanged(mode); };

        mViewModeSwitch = mmake<ViewModeSwitch>();
        mViewModeSwitch->name = "view mode";
        fixWidth(mViewModeSwitch, 62);
        mViewModeSwitch->SetValue(mEditWidget->IsView3DMode(), false);
        mUpPanel->AddChild(mViewModeSwitch);

        mViewModeSwitch->onSwitchByUser = [&](bool enabled) { mEditWidget->SetView3DMode(enabled); };
        mEditWidget->onView3DModeChanged = [&](bool enabled) { OnView3DModeChanged(enabled); };
    }

    void SceneWindow::OnView3DModeChanged(bool enabled)
    {
        if (mViewModeSwitch->GetValue() != enabled)
            mViewModeSwitch->SetValue(enabled);
    }

    void SceneWindow::ShowRenderModeMenu()
    {
        mRenderModeMenu->Show(mRenderModeButton->layout->GetWorldLeftBottom());

        // The check mark and the icon both lie at the left of an item: the icon with the caption step aside
        for (auto& item : mRenderModeMenu->GetItems())
        {
            if (auto widget = item->widget.Lock())
            {
                if (auto basic = widget->FindLayer("basic"))
                {
                    basic->layout = Layout::BothStretch(renderModeIconWidth, 0, 0, 0);

                    if (auto caption = basic->FindChild("caption"))
                        caption->layout.offsetMin = Vec2F(renderModeCaptionIndent, caption->layout.offsetMin.Get().y);
                }
            }
        }
    }

    void SceneWindow::OnRenderModeChanged(SceneRenderMode mode)
    {
        WString name = SceneEditScreen::GetRenderModeName(mode);
        if (auto icon = DynamicCast<VectorSprite>(mRenderModeButton->layer["icon"]->GetDrawable()))
            icon->LoadFromImage(SceneEditScreen::GetRenderModeIcon(mode));

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
