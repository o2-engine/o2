#include "o2/stdafx.h"
#include "AssetsLineConfig.h"

namespace o2
{
    bool AssetsLineConfig::operator==(const AssetsLineConfig& other) const
    {
        return enabled == other.enabled && serverUrl == other.serverUrl && projectId == other.projectId &&
            projectName == other.projectName && folder == other.folder && writeFinishAssets == other.writeFinishAssets &&
            Math::Equals(pollInterval, other.pollInterval);
    }
}
// --- META ---

DECLARE_CLASS(o2::AssetsLineConfig, o2__AssetsLineConfig);
// --- END META ---
