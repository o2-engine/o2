#include "o2/stdafx.h"
#include "SceneDrawableCategory.h"

namespace o2
{
    bool                  ScenePassFilters::sFilterEnabled = false;
    SceneDrawableCategory ScenePassFilters::sFilterCategory = SceneDrawableCategory::Scene2D;
    bool                  ScenePassFilters::sRawAlbedoMode = false;

    bool ScenePassFilters::IsPassing(SceneDrawableCategory category)
    {
        return !sFilterEnabled || sFilterCategory == category;
    }

    void ScenePassFilters::SetCategoryFilter(SceneDrawableCategory category)
    {
        sFilterEnabled = true;
        sFilterCategory = category;
    }

    void ScenePassFilters::ClearCategoryFilter()
    {
        sFilterEnabled = false;
    }

    bool ScenePassFilters::IsRawAlbedoMode()
    {
        return sRawAlbedoMode;
    }

    void ScenePassFilters::SetRawAlbedoMode(bool enabled)
    {
        sRawAlbedoMode = enabled;
    }
}
// --- META ---

ENUM_META(o2::SceneDrawableCategory, o2__SceneDrawableCategory)
{
    ENUM_ENTRY(Scene2D);
    ENUM_ENTRY(Scene3D);
}
END_ENUM_META;
// --- END META ---
