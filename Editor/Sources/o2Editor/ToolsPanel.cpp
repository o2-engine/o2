#include "o2Editor/stdafx.h"
#include "ToolsPanel.h"

#include "EditorApplication.h"
#include "o2/Animation/Tracks/AnimationColor4Track.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Tasks/TaskManager.h"
#include "o2Editor/Dialogs/EditNameDlg.h"
#include "o2Editor/Tools/FrameTool.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Tools/RotateTool.h"
#include "o2Editor/Tools/ScaleTool.h"
#include "o2Editor/Tools/SelectionTool.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"


namespace Editor
{
    const Ref<Widget>& ToolsPanel::GetPanelWidget() const
    {
        return mPanelRoot;
    }

    const Ref<Widget>& ToolsPanel::GetPlayPanel() const
    {
        return mPlayPanel;
    }

    const Ref<HorizontalLayout>& ToolsPanel::GetToolsPanel() const
    {
        return mToolsPanel;
    }

    void ToolsPanel::AddToolToggle(const Ref<Toggle>& toggle)
    {
        if (!toggle->GetStateObject("visible")->GetAnimationClip()->GetTrack<float>("layout/minWidth"))
        {
            *toggle->GetStateObject("visible")->GetAnimationClip()->AddTrack<float>("layout/minWidth") =
                AnimationTrack<float>::EaseInOut(0.0f, 20.0f, 0.2f);
        }

        mToolsPanel->AddChild(toggle);
        toggle->SetEnabledForcible(false);
        toggle->SetEnabled(true);
        toggle->SetToggleGroup(mToolsTogglesGroup);
    }

    void ToolsPanel::RemoveToolToggle(const Ref<Toggle>& toggle)
    {
        toggle->GetStateObject("visible")->onStateFullyFalse = [=]() { 
            o2Tasks.Invoke([=]() { mToolsPanel->RemoveChild(toggle, false); });
        };
        toggle->SetEnabled(false);
    }

    void ToolsPanel::Update(float dt)
    {
        mPlayToggle->value = o2EditorApplication.IsPlaying();
        mPauseToggle->value = o2EditorApplication.isPaused;
    }

    ToolsPanel::ToolsPanel(RefCounter* refCounter):
        Singleton<ToolsPanel>(refCounter), mToolsTogglesGroup(mmake<ToggleGroup>(ToggleGroup::Type::OnlySingleTrue))
    {
        mPanelRoot = mmake<Widget>();
        mPanelRoot->name = "tools panel";
        mPanelRoot->AddLayer("back", mmake<Sprite>("ui/UI4_ToolsPanel_bk.png"), Layout::BothStretch(-2, 0, -2, -8));

        mPanelRoot->layout->anchorMin = Vec2F(0, 1);
        mPanelRoot->layout->anchorMax = Vec2F(1, 1);
        mPanelRoot->layout->offsetMin = Vec2F(0, -45);
        mPanelRoot->layout->offsetMax = Vec2F(0, -20);

        InitializePlayPanel();
        InitializeLayoutSchemesPanel();
        InitializeToolsPanel();

        EditorUIRoot.AddWidget(mPanelRoot);
    }

    ToolsPanel::~ToolsPanel()
    {}

    void ToolsPanel::InitializePlayPanel()
    {
        mPlayPanel = mmake<Widget>();
        mPlayPanel->name = "play panel";
        mPlayPanel->AddLayer("back", mmake<Sprite>("ui/UI4_play_panel_bk.png"), Layout::BothStretch(-7, -5, -5, -5));
        *mPlayPanel->layout = WidgetLayout::VerStretch(HorAlign::Left, 3, 2, 200, 10);
        mPanelRoot->AddChild(mPlayPanel);

        mPlayToggle = o2UI.CreateWidget<Toggle>("play-stop");
        mPlayToggle->onToggle = [&](bool value) { OnPlayStopToggled(value); };
        *mPlayToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(5, 1));
        mPlayPanel->AddChild(mPlayToggle);

        mPauseToggle = o2UI.CreateWidget<Toggle>("pause");
        mPauseToggle->onToggle = [this](bool value) { OnPauseToggled(value); };
        *mPauseToggle->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(22, 1));
        mPauseToggle->shortcut = ShortcutKeys(VK_F11);
        mPlayPanel->AddChild(mPauseToggle);

        mStepButton = o2UI.CreateWidget<Button>("step");
        mStepButton->onClick = [this]() { OnStepPressed(); };
        *mStepButton->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(-5, 1));
        mStepButton->shortcut = ShortcutKeys(VK_F10);
        mPlayPanel->AddChild(mStepButton);

        auto playPanelPlayStateAnim = mmake<AnimationClip>();

        *playPanelPlayStateAnim->AddTrack<float>("layout/offsetRight") =
            AnimationTrack<float>::EaseInOut(59.0f, 73.0f, 0.3f);

        auto visiblePauseBtnAnim = playPanelPlayStateAnim->AddTrack<bool>("child/pause/enabled");
        visiblePauseBtnAnim->AddKey(0.0f, false);
        visiblePauseBtnAnim->AddKey(0.1f, false);
        visiblePauseBtnAnim->AddKey(0.11f, true);
        visiblePauseBtnAnim->AddKey(0.3f, true);

        mPlayPanel->AddState("playing", playPanelPlayStateAnim);

        mPlayToggle->onToggle += [=](bool playing) {
            mPlayPanel->SetState("playing", playing);
        };
    }

    void ToolsPanel::InitializeLayoutSchemesPanel()
    {
        mLayoutSchemesList = o2UI.CreateDropdown("round");
        *mLayoutSchemesList->layout = WidgetLayout::VerStretch(HorAlign::Right, 3, 2, 150, 10);
        mPanelRoot->AddChild(mLayoutSchemesList);
        UpdateWndLayoutSchemas();
        mLayoutSchemesList->onSelectedText = MakeFunction(this, &ToolsPanel::OnSchemeSelected);
    }

    void ToolsPanel::InitializeToolsPanel()
    {
        mToolsPanel = mmake<HorizontalLayout>();
        mToolsPanel->name = "edit tools";
        mToolsPanel->AddLayer("back", mmake<Sprite>("ui/UI4_panel_subpanel_bk.png"), Layout::BothStretch(-7, -5, -10, -5));
        *mToolsPanel->layout = WidgetLayout::VerStretch(HorAlign::Middle, 3, 2, 200, 10);
        mToolsPanel->expandHeight = true;
        mToolsPanel->expandWidth = false;
        mToolsPanel->fitByChildren = true;
        mToolsPanel->baseCorner = BaseCorner::Center;
        mPanelRoot->AddChild(mToolsPanel);

        for (auto& tool : o2EditorSceneScreen.GetTools())
            AddToolToggle(tool->GetPanelToggle());

        o2EditorSceneScreen.SelectTool<FrameTool>();
    }

    void ToolsPanel::UpdateWndLayoutSchemas()
    {
        mLayoutSchemesList->RemoveAllItems();

        auto currentLayout = o2EditorWindows.GetWindowsLayout();
        String currentLayoutName = mDefaultSchemeName;
        for (auto& availableLayoutKV : o2EditorWindows.mAvailableLayouts)
        {
            mLayoutSchemesList->AddItem(availableLayoutKV.first);

            if (currentLayout == availableLayoutKV.second)
                currentLayoutName = availableLayoutKV.first;
        }

        mLayoutSchemesList->AddItem(mDefaultSchemeName);
        mLayoutSchemesList->AddItem(mSaveAsSchemeName);

        mLayoutSchemesList->SelectItemText(currentLayoutName);
    }

    void ToolsPanel::OnSchemeSelected(const WString& name)
    {
        if ((String)name == mSaveAsSchemeName)
        {
            NameEditDlg::Show("New layout", [&](const String& name) {
                o2EditorWindows.mAvailableLayouts.Add(name, o2EditorWindows.GetWindowsLayout());
                UpdateWndLayoutSchemas();
            });
        }

        if ((String)name == mDefaultSchemeName)
        {
            o2EditorWindows.SetDefaultWindowsLayout();
        }

        if (o2EditorWindows.mAvailableLayouts.ContainsKey(name))
        {
            o2EditorWindows.SetWindowsLayout(name);
        }
    }

    void ToolsPanel::OnPlayStopToggled(bool play)
    {
        o2EditorApplication.SetPlaying(play);
    }

    void ToolsPanel::OnPauseToggled(bool pause)
    {
        o2EditorApplication.isPaused = pause;
    }

    void ToolsPanel::OnStepPressed()
    {
        o2EditorApplication.SetPlaying(true);
        o2EditorApplication.isPaused = true;
        o2EditorApplication.step = true;
    }

}
