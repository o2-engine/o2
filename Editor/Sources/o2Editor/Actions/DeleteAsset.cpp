#include "o2Editor/stdafx.h"
#include "DeleteAsset.h"

#include "o2/Assets/Assets.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    DeleteAssetAction::DeleteAssetAction()
    {}

    DeleteAssetAction::DeleteAssetAction(const Vector<String>& assetsPaths)
    {
        for (auto& path : assetsPaths)
        {
            Entry e;
            e.originalPath = path;
            entries.Add(e);
        }
    }

    String DeleteAssetAction::GetName() const
    {
        return "Delete asset";
    }

    void DeleteAssetAction::Redo()
    {
        for (auto& e : entries)
            e.trashPath = AssetsTrash::StashAsset(e.originalPath);
        AssetsTrash::NotifyAssetsChanged();
    }

    void DeleteAssetAction::Undo()
    {
        for (auto& e : entries)
            AssetsTrash::RestoreAsset(e.trashPath, e.originalPath);
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::DeleteAssetAction, Editor__DeleteAssetAction);

DECLARE_CLASS(Editor::DeleteAssetAction::Entry, Editor__DeleteAssetAction__Entry);
// --- END META ---
