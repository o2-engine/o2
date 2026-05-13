#pragma once

#include "o2/Utils/Types/String.h"

namespace Editor::AssetsTrash
{
    // Absolute path to the editor trash root. Created lazily on first stash.
    o2::String GetRoot();

    // Returns the absolute filesystem path used by StashAsset/RestoreAsset for
    // a given asset-relative path. Exposed for diagnostics and tests.
    o2::String AbsolutePathFor(const o2::String& assetsRelPath);

    // Last failure reason set by StashAsset/RestoreAsset. Useful for tests.
    const o2::String& LastError();

    // Toggle whether asset actions call o2Assets.RebuildAssets after mutation.
    // Tests set this to false to avoid spawning AssetsBuilder in parallel.
    void SetRebuildAssetsAfterMutation(bool enabled);
    bool ShouldRebuildAssetsAfterMutation();

    // Helper for asset actions to refresh the asset index after a mutation,
    // honoring the flag above.
    void NotifyAssetsChanged();

    // Moves an asset file (and its .meta sibling, if present) from assetsRelPath
    // into a unique subfolder under GetRoot(). Returns the subfolder path so the
    // caller can pass it back to RestoreAsset(). For folder assets, moves the
    // whole directory tree. Returns empty string on failure.
    o2::String StashAsset(const o2::String& assetsRelPath);

    // Restores a previously stashed asset back to assetsRelPath. trashSubfolder
    // must be exactly what StashAsset returned. Removes the empty subfolder
    // when done.
    bool RestoreAsset(const o2::String& trashSubfolder, const o2::String& assetsRelPath);

    // Wipes the trash root. Called by EditorApplication at startup since previous
    // session's undo frames are no longer valid.
    void ClearAllOnStartup();
}
