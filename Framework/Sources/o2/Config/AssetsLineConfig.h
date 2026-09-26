#pragma once
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------------
    // Link of the project to an AssetsLine project: its pipelines and generated
    // results sync both ways with the pipeline assets in `folder`. The access
    // token is per user and lives in Work/, never in this shared config
    // -------------------------------------------------------------------------
    class AssetsLineConfig: public ISerializable
    {
    public:
        bool   enabled = false;           // Sync runs while the editor is open @SERIALIZABLE
        String serverUrl;                 // AssetsLine address, e.g. https://assetsline.app @SERIALIZABLE
        String projectId;                 // Linked AssetsLine project id @SERIALIZABLE
        String projectName;               // Linked project name, for display @SERIALIZABLE
        String folder = String("Pipelines"); // Folder inside Assets the project's pipelines sync with @SERIALIZABLE
        bool   writeFinishAssets = true;  // Results reaching finish nodes are written into the project's assets @SERIALIZABLE
        float  pollInterval = 10.0f;      // Seconds between checks for changes made elsewhere @SERIALIZABLE

        bool operator==(const AssetsLineConfig& other) const;

        SERIALIZABLE(AssetsLineConfig);
    };
}
// --- META ---

CLASS_BASES_META(o2::AssetsLineConfig)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::AssetsLineConfig)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(enabled);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(serverUrl);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(projectId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(projectName);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(String("Pipelines")).NAME(folder);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(writeFinishAssets);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10.0f).NAME(pollInterval);
}
END_META;
CLASS_METHODS_META(o2::AssetsLineConfig)
{
}
END_META;
// --- END META ---
