#pragma once

#include "o2/Utils/Reflection/Enum.h"

namespace o2
{
    // Render category of scene drawable content, used to split content between render passes
    enum class SceneDrawableCategory { Scene2D, Scene3D };

    // --------------------------------------------------------------------------------
    // Pass-scoped drawing filters. Render passes set the current category filter while
    // drawing scene content; when no filter is set everything is drawn (direct paths)
    // --------------------------------------------------------------------------------
    class ScenePassFilters
    {
    public:
        // Returns true when the category passes the current filter
        static bool IsPassing(SceneDrawableCategory category);

        // Sets current category filter
        static void SetCategoryFilter(SceneDrawableCategory category);

        // Clears current category filter, everything passes
        static void ClearCategoryFilter();

        // Returns true when 3D meshes must be drawn with raw albedo colors (G-buffer pass)
        static bool IsRawAlbedoMode();

        // Enables or disables raw albedo mode
        static void SetRawAlbedoMode(bool enabled);

    private:
        static bool                  sFilterEnabled;
        static SceneDrawableCategory sFilterCategory;
        static bool                  sRawAlbedoMode;
    };

    // RAII category filter scope
    struct ScenePassCategoryScope
    {
        ScenePassCategoryScope(SceneDrawableCategory category) { ScenePassFilters::SetCategoryFilter(category); }
        ~ScenePassCategoryScope() { ScenePassFilters::ClearCategoryFilter(); }
    };
}
// --- META ---

PRE_ENUM_META(o2::SceneDrawableCategory);
// --- END META ---
