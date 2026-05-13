#include "o2Editor/stdafx.h"
#include "CreateAsset.h"

#include "o2/Assets/Asset.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor
{
    CreateAssetAction::CreateAssetAction()
    {}

    CreateAssetAction::CreateAssetAction(const Type& assetType, const String& parentFolderPath, const String& assetName):
        assetTypeName(assetType.GetName()), parentFolderPath(parentFolderPath), assetName(assetName)
    {}

    String CreateAssetAction::GetName() const
    {
        return "Create asset";
    }

    void CreateAssetAction::Redo()
    {
        if (!trashPath.IsEmpty())
        {
            AssetsTrash::RestoreAsset(trashPath, createdPath);
            trashPath = "";
            AssetsTrash::NotifyAssetsChanged();
            return;
        }

        const Type* type = o2Reflection.GetType(assetTypeName);
        if (!type)
            return;

        auto objectType = dynamic_cast<const ObjectType*>(type);
        if (!objectType)
            return;

        auto sample = objectType->CreateSampleRef();
        auto asset = DynamicCast<Asset>(sample);
        if (!asset)
            return;

        createdPath = parentFolderPath.IsEmpty() ? assetName : parentFolderPath + "/" + assetName;
        asset->Save(createdPath);
        AssetsTrash::NotifyAssetsChanged();
    }

    void CreateAssetAction::Undo()
    {
        if (createdPath.IsEmpty())
            return;
        trashPath = AssetsTrash::StashAsset(createdPath);
        AssetsTrash::NotifyAssetsChanged();
    }
}
// --- META ---

DECLARE_CLASS(Editor::CreateAssetAction, Editor__CreateAssetAction);
// --- END META ---
