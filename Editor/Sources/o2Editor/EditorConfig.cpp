#include "o2Editor/stdafx.h"
#include "EditorConfig.h"

#include "o2/Application/Application.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/MenuPanel.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/IEditorWindow.h"
#include "o2Editor/Windows/WindowsManager.h"


namespace Editor
{
    EditorConfig::EditorConfig(RefCounter* refCounter):
        Singleton<EditorConfig>(refCounter)
    {}

    EditorConfig::~EditorConfig()
    {}

    void EditorConfig::SaveGlobalConfigs()
    {
        if (mSaving)
            return;

        mSaving = true;

        DataDocument data;

        globalConfig.mAvailableLayouts = o2EditorWindows.mAvailableLayouts;

        data = globalConfig;
        data.SaveToFile(mGlobalConfigPath);

        mSaving = false;
    }

    void EditorConfig::SaveProjectConfigs()
    {
        if (!WindowsManager::IsSingletonInitialzed())
            return;

        if (mSaving)
            return;

        mSaving = true;

        projectConfig.mLayout = o2EditorWindows.GetWindowsLayout();

        DataDocument data;
        data = projectConfig;
        data.SaveToFile(mConfigPath);

        mSaving = false;
    }

    void EditorConfig::LoadConfigs()
    {
        LoadGlobalConfig();
        LoadProjectConfig();

        mConfigsLoaded = true;
    }

    void EditorConfig::LoadProjectConfig()
    {
        DataDocument data;

        if (data.LoadFromFile(mConfigPath))
            projectConfig = data;
        else
        {
            projectConfig = ProjectConfig();
        }
    }

    void EditorConfig::LoadGlobalConfig()
    {
        DataDocument data;
        if (data.LoadFromFile(mGlobalConfigPath))
            globalConfig = data;
        else
            globalConfig = GlobalConfig();
    }

    void EditorConfig::OnWindowChange()
    {
        Vec2I newPosition = o2Application.GetWindowPosition();
        Vec2I newSize     = o2Application.GetWindowSize();
        bool  newMax      = o2Application.IsMaximized();

        bool changed = projectConfig.mWindowPosition != newPosition ||
                       projectConfig.mWindowSize     != newSize ||
                       projectConfig.mMaximized      != newMax;

        projectConfig.mWindowPosition = newPosition;
        projectConfig.mWindowSize     = newSize;
        projectConfig.mMaximized      = newMax;

        if (changed)
            OnProjectConfigChanged();
    }

    void EditorConfig::OnProjectConfigChanged()
    {
        if (!IsSingletonInitialzed())
            return;

        auto& cfg = Instance();
        if (!cfg.mConfigsLoaded || cfg.mSaving)
            return;

        cfg.SaveProjectConfigs();
    }

    void EditorConfig::OnGlobalConfigChanged()
    {
        if (!IsSingletonInitialzed())
            return;

        auto& cfg = Instance();
        if (!cfg.mConfigsLoaded || cfg.mSaving)
            return;

        cfg.SaveGlobalConfigs();
    }

    // --- ProjectConfig getters/setters ---

    Vec2I EditorConfig::ProjectConfig::GetWindowSize() const
    {
        return mWindowSize;
    }

    void EditorConfig::ProjectConfig::SetWindowSize(const Vec2I& value)
    {
        if (mWindowSize == value)
            return;
        mWindowSize = value;
        EditorConfig::OnProjectConfigChanged();
    }

    Vec2I EditorConfig::ProjectConfig::GetWindowPosition() const
    {
        return mWindowPosition;
    }

    void EditorConfig::ProjectConfig::SetWindowPosition(const Vec2I& value)
    {
        if (mWindowPosition == value)
            return;
        mWindowPosition = value;
        EditorConfig::OnProjectConfigChanged();
    }

    bool EditorConfig::ProjectConfig::GetMaximized() const
    {
        return mMaximized;
    }

    void EditorConfig::ProjectConfig::SetMaximized(bool value)
    {
        if (mMaximized == value)
            return;
        mMaximized = value;
        EditorConfig::OnProjectConfigChanged();
    }

    WindowsLayout EditorConfig::ProjectConfig::GetLayout() const
    {
        return mLayout;
    }

    void EditorConfig::ProjectConfig::SetLayout(const WindowsLayout& value)
    {
        if (mLayout == value)
            return;
        mLayout = value;
        EditorConfig::OnProjectConfigChanged();
    }

    String EditorConfig::ProjectConfig::GetLastLoadedScene() const
    {
        return mLastLoadedScene;
    }

    void EditorConfig::ProjectConfig::SetLastLoadedScene(const String& value)
    {
        if (mLastLoadedScene == value)
            return;
        mLastLoadedScene = value;
        EditorConfig::OnProjectConfigChanged();
    }

    // --- GlobalConfig getters/setters ---

    WindowsLayout EditorConfig::GlobalConfig::GetDefaultLayout() const
    {
        return mDefaultLayout;
    }

    void EditorConfig::GlobalConfig::SetDefaultLayout(const WindowsLayout& value)
    {
        if (mDefaultLayout == value)
            return;
        mDefaultLayout = value;
        EditorConfig::OnGlobalConfigChanged();
    }

    EditorLayoutsMap EditorConfig::GlobalConfig::GetAvailableLayouts() const
    {
        return mAvailableLayouts;
    }

    void EditorConfig::GlobalConfig::SetAvailableLayouts(const EditorLayoutsMap& value)
    {
        if (mAvailableLayouts == value)
            return;
        mAvailableLayouts = value;
        EditorConfig::OnGlobalConfigChanged();
    }

}
// --- META ---

DECLARE_CLASS(Editor::EditorConfig, Editor__EditorConfig);

DECLARE_CLASS(Editor::EditorConfig::GlobalConfig, Editor__EditorConfig__GlobalConfig);

DECLARE_CLASS(Editor::EditorConfig::ProjectConfig, Editor__EditorConfig__ProjectConfig);
// --- END META ---
