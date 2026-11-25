#include "o2/stdafx.h"
#include "VectorFontAssetConverter.h"

#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Render/Text/SdfFont.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2AssetBuilder/AssetsBuilder.h"

namespace o2
{
    namespace
    {
        // The baked fields, and the atlas of the first version of them
        const char* atlasExtensions[] = { "", ".png", ".json" };

        // The stamp of the atlas: the font file, the settings and the characters
        String AtlasStamp(const String& sourcePath, const Ref<VectorFontAsset::Meta>& meta)
        {
            WString characters = meta->GetFieldCharacters();
            UInt64 hash = 14695981039346656037ull;
            for (int i = 0; i < characters.Length(); i++)
            {
                hash ^= (UInt64)characters[i];
                hash *= 1099511628211ull;
            }

            return SdfFont::GetSourceStamp(sourcePath, meta->GetFieldSettings()) + String::Format("/%llx", (unsigned long long)hash);
        }

        void DeleteAtlas(const String& atlasPath)
        {
            for (auto extension : atlasExtensions)
            {
                if (o2FileSystem.IsFileExist(atlasPath + extension))
                    o2FileSystem.FileDelete(atlasPath + extension);
            }
        }
    }

    Vector<const Type*> VectorFontAssetConverter::GetProcessingAssetsTypes() const
    {
        Vector<const Type*> res;
        res.Add(&TypeOf(VectorFontAsset));
        return res;
    }

    void VectorFontAssetConverter::ConvertAsset(const AssetInfo& node)
    {
        String sourceAssetPath = mAssetsBuilder->GetSourceAssetsPath() + node.path;
        String builtAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;

        o2FileSystem.FileCopy(sourceAssetPath, builtAssetPath);
        BakeAtlas(node);
    }

    void VectorFontAssetConverter::RemoveAsset(const AssetInfo& node)
    {
        String builtAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;
        o2FileSystem.FileDelete(builtAssetPath);
        DeleteAtlas(SdfFont::GetBakedFieldsPath(builtAssetPath));
    }

    void VectorFontAssetConverter::MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo)
    {
        String fullPathFrom = mAssetsBuilder->GetBuiltAssetsPath() + nodeFrom.path;
        String fullPathTo = mAssetsBuilder->GetBuiltAssetsPath() + nodeTo.path;
        o2FileSystem.FileMove(fullPathFrom, fullPathTo);

        String atlasFrom = SdfFont::GetBakedFieldsPath(fullPathFrom), atlasTo = SdfFont::GetBakedFieldsPath(fullPathTo);
        for (auto extension : atlasExtensions)
        {
            if (o2FileSystem.IsFileExist(atlasFrom + extension))
                o2FileSystem.FileMove(atlasFrom + extension, atlasTo + extension);
        }
    }

    Vector<UID> VectorFontAssetConverter::AssetsPostProcess()
    {
        Vector<UID> res;
        if (!mAssetsBuilder->mBuiltAssetsTree)
            return res;

        for (auto& infoWeak : mAssetsBuilder->mBuiltAssetsTree->allAssets)
        {
            auto info = infoWeak.Lock();
            if (info && DynamicCast<VectorFontAsset::Meta>(info->meta) && BakeAtlas(*info))
                res.Add(info->meta->ID());
        }

        return res;
    }

    bool VectorFontAssetConverter::BakeAtlas(const AssetInfo& node)
    {
        String sourceAssetPath = mAssetsBuilder->GetSourceAssetsPath() + node.path;
        String atlasPath = SdfFont::GetBakedFieldsPath(mAssetsBuilder->GetBuiltAssetsPath() + node.path);

        auto meta = DynamicCast<VectorFontAsset::Meta>(node.meta);
        if (!meta || !meta->distanceField)
        {
            DeleteAtlas(atlasPath);
            return false;
        }

        String stamp = AtlasStamp(sourceAssetPath, meta);
        if (SdfFont::ReadBakedStamp(atlasPath) == stamp)
            return false;

        DeleteAtlas(atlasPath);

        Timer timer;
        WString characters = meta->GetFieldCharacters();
        if (!SdfFont::Bake(sourceAssetPath, meta->GetFieldSettings(), characters, atlasPath, stamp))
        {
            DeleteAtlas(atlasPath);
            o2Debug.LogWarningStr("Can't bake the distance fields of font " + node.path);
            return false;
        }

        o2Debug.LogStr(String::Format("Baked the distance fields of font %s: %d characters in %.0f ms",
                                      node.path.Data(), characters.Length(), timer.GetDeltaTime()*1000.0f));
        return true;
    }
}
// --- META ---

DECLARE_CLASS(o2::VectorFontAssetConverter, o2__VectorFontAssetConverter);
// --- END META ---
