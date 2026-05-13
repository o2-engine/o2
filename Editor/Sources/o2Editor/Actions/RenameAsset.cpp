#include "o2Editor/stdafx.h"
#include "RenameAsset.h"

#include "o2/Assets/Assets.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    RenameAssetAction::RenameAssetAction()
    {}

    RenameAssetAction::RenameAssetAction(const UID& assetUid, const String& originalName, const String& newName):
        assetUid(assetUid), originalName(originalName), newName(newName)
    {}

    String RenameAssetAction::GetName() const
    {
        return "Rename asset";
    }

    void RenameAssetAction::Redo()
    {
        o2Assets.RenameAsset(assetUid, newName);
        AssetsTrash::NotifyAssetsChanged();
    }

    void RenameAssetAction::Undo()
    {
        o2Assets.RenameAsset(assetUid, originalName);
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::RenameAssetAction, Editor__RenameAssetAction);
// --- END META ---
