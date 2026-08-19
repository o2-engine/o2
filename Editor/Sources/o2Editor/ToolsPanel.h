#pragma once

#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace o2
{
    class Button;
    class CustomDropDown;
    class DropDown;
    class HorizontalLayout;
    class Label;
    class Widget;
}

// Editor tools panel accessor macros
#define o2EditorTools ToolsPanel::Instance()

namespace Editor
{
    // ------------------
    // Editor tools panel
    // ------------------
    class ToolsPanel : public Singleton<ToolsPanel>
    {
    public:
        // Default constructor. Initializes all panel
        ToolsPanel(RefCounter* refCounter);

        // Destructor
        ~ToolsPanel();

        // Returns panel's widgets
        const Ref<Widget>& GetPanelWidget() const;

        // Returns play panel's widget
        const Ref<Widget>& GetPlayPanel() const;

        // Returns tools' panel widget 
        const Ref<HorizontalLayout>& GetToolsPanel() const;

        // Adds tool to panel
        void AddToolToggle(const Ref<Toggle>& toggle);

        // Removes tool from panel
        void RemoveToolToggle(const Ref<Toggle>& toggle);

        // Switches game update speed to the next of the predefined scales
        void IncreaseGameSpeed();

        // Switches game update speed to the previous of the predefined scales
        void DecreaseGameSpeed();

        // Updates panel
        void Update(float dt);

    protected:
        const String mDefaultSchemeName = "Default";
        const String mSaveAsSchemeName = "Save as ...";

        Ref<Widget> mPanelRoot; // Root panel widget

        Ref<Widget> mPlayPanel;   // Play panel widget
        Ref<Toggle> mPlayToggle;  // Play toggle
        Ref<Toggle> mPauseToggle; // Pause toggle
        Ref<Button> mStepButton;  // Step button

        Ref<Button> mSpeedMinusButton; // Slow down game speed button
        Ref<Button> mSpeedPlusButton;  // Speed up game speed button
        Ref<Label>  mSpeedLabel;       // Current game speed scale label
        int         mSpeedIndex = 4;   // Index in game speed scales list (1.0 by default)

        Ref<HorizontalLayout> mToolsPanel;        // Tools panel layout
        Ref<ToggleGroup>      mToolsTogglesGroup; // Group for toggles

        Ref<DropDown> mLayoutSchemesList; // Layouts schemes list

    protected:
        // initializes play panel
        void InitializePlayPanel();

        // initializes schemes panel
        void InitializeLayoutSchemesPanel();

        // Initializes tools panel
        void InitializeToolsPanel();

        // Updates schemes list
        void UpdateWndLayoutSchemas();

        // Called when selected scheme
        void OnSchemeSelected(const WString& name);

        // Called when play/stop toggle changed
        void OnPlayStopToggled(bool play);

        // Called when pause toggle changed
        void OnPauseToggled(bool pause);

        // Called when step button has pressed
        void OnStepPressed();

        // Sets game speed scale by index in scales list: clamps, applies to application, updates label
        void SetGameSpeedIndex(int index);

        friend class EditorApplication;
        friend class EditorConfig;
    };

}
