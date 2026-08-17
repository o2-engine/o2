#pragma once

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
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

    using EditorLayoutsMap = Map<String, WindowsLayout>;

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
            PROPERTIES(GlobalConfig);
            PROPERTY(WindowsLayout, defaultLayout, SetDefaultLayout, GetDefaultLayout);             // Default windows layout, used on reset
            PROPERTY(EditorLayoutsMap, availableLayouts, SetAvailableLayouts, GetAvailableLayouts); // Available windows layouts

        public:
            // Returns default windows layout
            WindowsLayout GetDefaultLayout() const;

            // Sets default windows layout
            void SetDefaultLayout(const WindowsLayout& value);

            // Returns available windows layouts
            EditorLayoutsMap GetAvailableLayouts() const;

            // Sets available windows layouts
            void SetAvailableLayouts(const EditorLayoutsMap& value);

            SERIALIZABLE(GlobalConfig);

        private:
            WindowsLayout              mDefaultLayout;     // @SERIALIZABLE
            Map<String, WindowsLayout> mAvailableLayouts;  // @SERIALIZABLE

            friend class EditorConfig;
        };

        // ----------------------------
        // Project editor configuration
        // ----------------------------
        class ProjectConfig : public ISerializable
        {
        public:
            PROPERTIES(ProjectConfig);
            PROPERTY(Vec2I, windowSize, SetWindowSize, GetWindowSize);                 // Application window size
            PROPERTY(Vec2I, windowPosition, SetWindowPosition, GetWindowPosition);     // Application window position
            PROPERTY(bool, maximized, SetMaximized, GetMaximized);                     // Is application window maximized
            PROPERTY(WindowsLayout, layout, SetLayout, GetLayout);                     // Windows layout
            PROPERTY(String, lastLoadedScene, SetLastLoadedScene, GetLastLoadedScene); // Last loaded scene

        public:
            // Returns application window size
            Vec2I GetWindowSize() const;

            // Sets application window size
            void SetWindowSize(const Vec2I& value);

            // Returns application window position
            Vec2I GetWindowPosition() const;

            // Sets application window position
            void SetWindowPosition(const Vec2I& value);

            // Returns is application window maximized
            bool GetMaximized() const;

            // Sets is application window maximized
            void SetMaximized(bool value);

            // Returns windows layout
            WindowsLayout GetLayout() const;

            // Sets windows layout
            void SetLayout(const WindowsLayout& value);

            // Returns last loaded scene path
            String GetLastLoadedScene() const;

            // Sets last loaded scene path
            void SetLastLoadedScene(const String& value);

            SERIALIZABLE(ProjectConfig);

        private:
            Vec2I         mWindowSize = Vec2I(800, 600); // @SERIALIZABLE
            Vec2I         mWindowPosition;               // @SERIALIZABLE
            bool          mMaximized = true;             // @SERIALIZABLE
            WindowsLayout mLayout;                       // @SERIALIZABLE
            String        mLastLoadedScene;              // @SERIALIZABLE

            friend class EditorConfig;
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
#if defined PLATFORM_WASM
        // The page mounts a persistent file system at /config: everything else in the virtual file
        // system is packed into the build and is gone on reload
        String mConfigPath = "/config/EditorConfig.json";
#else
        String mConfigPath = "../../EditorConfig.json";
#endif
        String mGlobalConfigPath = "../../Config.json";

        bool mConfigsLoaded = false; // True if configurations were loaded
        bool mSaving        = false; // True while a save is in progress; prevents recursion

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

        // Triggers project config save in response to a property change
        static void OnProjectConfigChanged();

        // Triggers global config save in response to a property change
        static void OnGlobalConfigChanged();

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
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSaving);
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
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnProjectConfigChanged);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnGlobalConfigChanged);
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::GlobalConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::GlobalConfig)
{
    FIELD().PUBLIC().NAME(defaultLayout);
    FIELD().PUBLIC().NAME(availableLayouts);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mDefaultLayout);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mAvailableLayouts);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::GlobalConfig)
{

    FUNCTION().PUBLIC().SIGNATURE(WindowsLayout, GetDefaultLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDefaultLayout, const WindowsLayout&);
    FUNCTION().PUBLIC().SIGNATURE(EditorLayoutsMap, GetAvailableLayouts);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAvailableLayouts, const EditorLayoutsMap&);
}
END_META;

CLASS_BASES_META(Editor::EditorConfig::ProjectConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorConfig::ProjectConfig)
{
    FIELD().PUBLIC().NAME(windowSize);
    FIELD().PUBLIC().NAME(windowPosition);
    FIELD().PUBLIC().NAME(maximized);
    FIELD().PUBLIC().NAME(layout);
    FIELD().PUBLIC().NAME(lastLoadedScene);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2I(800, 600)).NAME(mWindowSize);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mWindowPosition);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMaximized);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mLayout);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mLastLoadedScene);
}
END_META;
CLASS_METHODS_META(Editor::EditorConfig::ProjectConfig)
{

    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetWindowSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWindowSize, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetWindowPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWindowPosition, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetMaximized);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaximized, bool);
    FUNCTION().PUBLIC().SIGNATURE(WindowsLayout, GetLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayout, const WindowsLayout&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetLastLoadedScene);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLastLoadedScene, const String&);
}
END_META;
// --- END META ---
