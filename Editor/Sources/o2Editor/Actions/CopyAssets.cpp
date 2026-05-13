#include "o2Editor/stdafx.h"
#include "CopyAssets.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    CopyAssetsAction::CopyAssetsAction()
    {}

    CopyAssetsAction::CopyAssetsAction(const Vector<String>& sourcePaths, const String& destFolder):
        destFolder(destFolder)
    {
        for (auto& src : sourcePaths)
        {
            Entry e;
            e.sourcePath = src;
            entries.Add(e);
        }
    }

    String CopyAssetsAction::GetName() const
    {
        return "Copy asset";
    }

    void CopyAssetsAction::Redo()
    {
        for (auto& e : entries)
        {
            if (!e.trashPath.IsEmpty())
            {
                AssetsTrash::RestoreAsset(e.trashPath, e.createdPath);
                e.trashPath = "";
                continue;
            }

            String filename = o2FileSystem.GetPathWithoutDirectories(e.sourcePath);
            String target = destFolder.IsEmpty() ? filename : destFolder + "/" + filename;
            target = o2Assets.MakeUniqueAssetName(target);
            o2Assets.CopyAsset(e.sourcePath, target);
            e.createdPath = target;
        }
        AssetsTrash::NotifyAssetsChanged();
    }

    void CopyAssetsAction::Undo()
    {
        for (auto& e : entries)
        {
            if (e.createdPath.IsEmpty())
                continue;
            e.trashPath = AssetsTrash::StashAsset(e.createdPath);
        }
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::CopyAssetsAction, Editor__CopyAssetsAction);

DECLARE_CLASS(Editor::CopyAssetsAction::Entry, Editor__CopyAssetsAction__Entry);
// --- END META ---
