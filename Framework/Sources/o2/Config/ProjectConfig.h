#pragma once

#include "o2/Config/PhysicsConfig.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

// Project configuration access macros
#define o2Config o2::ProjectConfig::Instance()

namespace o2
{
    class ProjectBuildConfig;

    // ---------------------
    // Project configuration
    // ---------------------
    class ProjectConfig: public ISerializable, public Singleton<ProjectConfig>
    {
    public:
        PROPERTIES(ProjectConfig);
        PROPERTY(String, projectName, SetProjectName, GetProjectName); // Project name property
        PROPERTY(Platform, currentPlatform, SetPlatform, GetPlatform); // Project platform property

    public:
        PhysicsConfig physics; // Physics world config @SERIALIZABLE

    public:
        // Default constructor
        ProjectConfig(RefCounter* refCounter);

        // Destructor
        ~ProjectConfig();

        // Returns project name 
        String GetProjectName() const;

        // Sets project name
        void SetProjectName(const String& name);

        // Returns platform
        Platform GetPlatform() const;

        // Sets platform
        void SetPlatform(Platform platform);

        // Save config to the default project settings file
        void Save() const;

        // Saves config to file. Skips writing when nothing changed since load,
        // or when the config failed to load and holds nothing but defaults -
        // so a process that couldn't read the settings never clobbers them
        void Save(const String& path) const;

        // Loads config from the default project settings file
        void Load();

        // Loads config from file; on failure keeps current values
        void Load(const String& path);

        // Returns true when every setting equals its default value
        bool IsDefault() const;

        SERIALIZABLE(ProjectConfig);

    protected:
        String   mProjectName; // Current project name @SERIALIZABLE
        Platform mPlatform;    // Current project target platform

        String mLoadedState; // Serialized snapshot from the successful load, empty when load failed

        friend class AssetBuildSystem;
        friend class Application;
    };
}
// --- META ---

CLASS_BASES_META(o2::ProjectConfig)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::Singleton<ProjectConfig>);
}
END_META;
CLASS_FIELDS_META(o2::ProjectConfig)
{
    FIELD().PUBLIC().NAME(projectName);
    FIELD().PUBLIC().NAME(currentPlatform);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(physics);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mProjectName);
    FIELD().PROTECTED().NAME(mPlatform);
    FIELD().PROTECTED().NAME(mLoadedState);
}
END_META;
CLASS_METHODS_META(o2::ProjectConfig)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(String, GetProjectName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetProjectName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Platform, GetPlatform);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlatform, Platform);
    FUNCTION().PUBLIC().SIGNATURE(void, Save);
    FUNCTION().PUBLIC().SIGNATURE(void, Save, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Load);
    FUNCTION().PUBLIC().SIGNATURE(void, Load, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDefault);
}
END_META;
// --- END META ---
