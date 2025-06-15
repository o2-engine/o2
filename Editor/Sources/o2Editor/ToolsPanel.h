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

        // Updates panel
        void Update(float dt);

    protected:
        const String mDefaultSchemeName = "Default";
        const String mSaveAsSchemeName = "Save as ...";

        Ref<Widget> mPanelRoot; // Root panel widget

        Ref<Widget>   mPlayPanel;    // Play panel widget
        Ref<Toggle>   mPlayToggle;    // Play toggle
        Ref<Toggle>   mPauseToggle;    // Pause toggle
        Ref<Button>   mStepButton;    // Step button
        Ref<DropDown> mDevicesList;    // Devices list dropdown

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

        friend class EditorApplication;
        friend class EditorConfig;
    };

}
