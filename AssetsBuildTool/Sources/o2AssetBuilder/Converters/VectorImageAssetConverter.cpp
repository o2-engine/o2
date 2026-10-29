#include "o2/stdafx.h"
#include "VectorImageAssetConverter.h"

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2AssetBuilder/AssetsBuilder.h"

namespace o2
{
    Vector<const Type*> VectorImageAssetConverter::GetProcessingAssetsTypes() const
    {
        Vector<const Type*> res;
        res.Add(&TypeOf(VectorImageAsset));
        return res;
    }

    void VectorImageAssetConverter::ConvertAsset(const AssetInfo& node)
    {
        String sourceAssetPath = mAssetsBuilder->GetSourceAssetsPath() + node.path;
        String builtAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;
        String meshPath = VectorImageAsset::GetBakedMeshPath(builtAssetPath);

        o2FileSystem.FileCopy(sourceAssetPath, builtAssetPath);

        auto meta = DynamicCast<VectorImageAsset::Meta>(node.meta);
        if (!meta || !meta->scalableMesh)
        {
            if (o2FileSystem.IsFileExist(meshPath))
                o2FileSystem.FileDelete(meshPath);

            return;
        }

        if (!VectorImageAsset::BakeMesh(o2FileSystem.ReadFile(sourceAssetPath), meshPath))
        {
            if (o2FileSystem.IsFileExist(meshPath))
                o2FileSystem.FileDelete(meshPath);

            o2Debug.LogWarningStr("Can't bake the mesh of vector image " + node.path);
        }
    }

    void VectorImageAssetConverter::RemoveAsset(const AssetInfo& node)
    {
        String builtAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;
        String meshPath = VectorImageAsset::GetBakedMeshPath(builtAssetPath);

        o2FileSystem.FileDelete(builtAssetPath);

        if (o2FileSystem.IsFileExist(meshPath))
            o2FileSystem.FileDelete(meshPath);
    }

    void VectorImageAssetConverter::MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo)
    {
        String fullPathFrom = mAssetsBuilder->GetBuiltAssetsPath() + nodeFrom.path;
        String fullPathTo = mAssetsBuilder->GetBuiltAssetsPath() + nodeTo.path;

        o2FileSystem.FileMove(fullPathFrom, fullPathTo);

        String meshFrom = VectorImageAsset::GetBakedMeshPath(fullPathFrom);
        if (o2FileSystem.IsFileExist(meshFrom))
            o2FileSystem.FileMove(meshFrom, VectorImageAsset::GetBakedMeshPath(fullPathTo));
    }

    Vector<UID> VectorImageAssetConverter::AssetsPostProcess()
    {
        Vector<UID> res;
        if (!mAssetsBuilder->mBuiltAssetsTree)
            return res;

        for (auto& infoWeak : mAssetsBuilder->mBuiltAssetsTree->allAssets)
        {
            auto info = infoWeak.Lock();
            auto meta = info ? DynamicCast<VectorImageAsset::Meta>(info->meta) : nullptr;
            if (!meta || !meta->scalableMesh)
                continue;

            String meshPath = VectorImageAsset::GetBakedMeshPath(mAssetsBuilder->GetBuiltAssetsPath() + info->path);
            if (VectorImageAsset::IsBakedMeshCurrent(meshPath))
                continue;

            if (VectorImageAsset::BakeMesh(o2FileSystem.ReadFile(mAssetsBuilder->GetSourceAssetsPath() + info->path), meshPath))
                res.Add(meta->ID());
        }

        return res;
    }
}
// --- META ---

DECLARE_CLASS(o2::VectorImageAssetConverter, o2__VectorImageAssetConverter);
// --- END META ---
