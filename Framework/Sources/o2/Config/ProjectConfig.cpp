#include "o2/stdafx.h"
#include "ProjectConfig.h"

#include "o2/EngineSettings.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    DECLARE_SINGLETON(ProjectConfig);

    ProjectConfig::ProjectConfig(RefCounter* refCounter):
        Singleton<ProjectConfig>(refCounter), mPlatform(GetEnginePlatform())
    {
        Load();
    }

    ProjectConfig::~ProjectConfig()
    {
        Save();
    }

    String ProjectConfig::GetProjectName() const
    {
        return mProjectName;
    }

    void ProjectConfig::SetProjectName(const String& name)
    {
        mProjectName = name;
    }

    Platform ProjectConfig::GetPlatform() const
    {
        return mPlatform;
    }

    void ProjectConfig::SetPlatform(Platform platform)
    {
        mPlatform = platform;
    }

    void ProjectConfig::Load()
    {
        Load(GetProjectSettingPath());
    }

    void ProjectConfig::Load(const String& path)
    {
        mLoadedState.Clear();

        DataDocument data;
        if (!data.LoadFromFile(path))
            return;

        Deserialize(data);

        DataDocument current;
        Serialize(current);
        mLoadedState = current.SaveAsString();
    }

    void ProjectConfig::Save() const
    {
        Save(GetProjectSettingPath());
    }

    void ProjectConfig::Save(const String& path) const
    {
        // A process that couldn't read the settings (wrong cwd, file busy) must
        // not overwrite them with defaults on exit
        if (mLoadedState.IsEmpty() && IsDefault())
            return;

        DataDocument data;
        Serialize(data);

        if (data.SaveAsString() == mLoadedState)
            return;

        data.SaveToFile(path);
    }

    bool ProjectConfig::IsDefault() const
    {
        return physics == PhysicsConfig() && mProjectName.IsEmpty();
    }
}
// --- META ---

DECLARE_CLASS(o2::ProjectConfig, o2__ProjectConfig);
// --- END META ---
