#include "o2Editor/stdafx.h"
#include "MoveAsset.h"

#include "o2/Assets/Assets.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    MoveAssetAction::MoveAssetAction()
    {}

    MoveAssetAction::MoveAssetAction(const Vector<Entry>& entries, const String& destFolder):
        entries(entries), destFolder(destFolder)
    {}

    String MoveAssetAction::GetName() const
    {
        return "Move asset";
    }

    void MoveAssetAction::Redo()
    {
        Vector<UID> uids;
        for (auto& e : entries)
            uids.Add(e.uid);
        o2Assets.MoveAssets(uids, destFolder);
        AssetsTrash::NotifyAssetsChanged();
    }

    void MoveAssetAction::Undo()
    {
        for (auto& e : entries)
        {
            String back = e.originalParent.IsEmpty() ? e.filename : e.originalParent + "/" + e.filename;
            o2Assets.MoveAsset(e.uid, back);
        }
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::MoveAssetAction, Editor__MoveAssetAction);

DECLARE_CLASS(Editor::MoveAssetAction::Entry, Editor__MoveAssetAction__Entry);
// --- END META ---
