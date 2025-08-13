#pragma once

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Singleton.h"
#include "o2Editor/Windows/WindowsLayout.h"

using namespace o2;

namespace o2
{
    class Widget;
}

// Editor configuration access macros
#define o2EditorConfig EditorConfig::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(DockWindowPlace);

    // -------------------------
    // Application configuration
    // -------------------------
    class EditorConfig : public Singleton<EditorConfig>, public ISerializable
    {
    public:
        // ---------------------------
        // Global editor configuration
        // ---------------------------
        class GlobalConfig : public ISerializable
        {
        public:
            WindowsLayout  mDefaultLayout; // Default windows layout, using in resetting @SERIALIZABLE

            Map<String, WindowsLayout> mAvailableLayouts; // Available windows layouts @SERIALIZABLE

            SERIALIZABLE(GlobalConfig);
        };

        // ----------------------------
        // Project editor configuration
        // ----------------------------
        class ProjectConfig : public ISerializable
        {
        public:
            Vec2I         mWindowSize = Vec2I(800, 600); // Application window size @SERIALIZABLE
            Vec2I         mWindowPosition;                      // Application window position @SERIALIZABLE
            bool          mMaximized = true;                    // Is application window is maximized @SERIALIZABLE
            WindowsLayout mLayout;                              // Windows layout @SERIALIZABLE
            String        mLastLoadedScene;                     // Last loaded scene @SERIALIZABLE

            SERIALIZABLE(ProjectConfig);
        };

    public:
        ProjectConfig projectConfig; // Project editor config
        GlobalConfig  globalConfig;  // Global editor config for all projects

    public:
        // Default constructor. Loads data and applies to application 
        EditorConfig(RefCounter* refCounter);

        // Destructor. Saves application configuration
        ~EditorConfig();

        SERIALIZABLE(EditorConfig);

    protected:
        String mConfigPath = "../../EditorConfig.json";
        String mGlobalConfigPath = "../../Config.json";

        bool mConfigsLoaded = false; // True if configurations were loaded

    protected:
        // Saves global configs
        void SaveGlobalConfigs();

        // Saves project configs
        void SaveProjectConfigs();

        // Loads and applies configs
        void LoadConfigs();

        // Loads project configs
        void LoadProjectConfig();

        // Loads global configs
        void LoadGlobalConfig();

        // Updates window configs
        void OnWindowChange();

        friend class EditorApplication;
        friend class MenuPanel;
        friend class WindowsManager;
    };
}
// --- META ---

CLASS_BASES_META(Editor::EditorConfig)
{
    BASE_CLASS(o2::Singleton<EditorConfig>);
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig)
{
    FIELD().PUBLIC().NAME(projectConfig);
    FIELD().PUBLIC().NAME(globalConfig);
    FIELD().PROTECTED().DEFAULT_VALUE("../../EditorConfig.json").NAME(mConfigPath);
    FIELD().PROTECTED().DEFAULT_VALUE("../../Config.json").NAME(mGlobalConfigPath);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mConfigsLoaded);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveGlobalConfigs);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveProjectConfigs);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadConfigs);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadProjectConfig);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadGlobalConfig);
    FUNCTION().PROTECTED().SIGNATURE(void, OnWindowChange);
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::GlobalConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::GlobalConfig)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mDefaultLayout);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mAvailableLayouts);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::GlobalConfig)
{
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::ProjectConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::ProjectConfig)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2I(800, 600)).NAME(mWindowSize);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mWindowPosition);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMaximized);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mLayout);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mLastLoadedScene);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::ProjectConfig)
{
}
END_META;
// --- END META ---
