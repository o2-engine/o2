#pragma once

#include "IAssetConverter.h"

namespace o2
{
    // ------------------------------------------------------------------------------------------------------------
    // Vector font asset converter: copies the font and bakes its distance fields when the meta asks
    // ------------------------------------------------------------------------------------------------------------
    class VectorFontAssetConverter: public IAssetConverter
    {
    public:
        // Returns vector of processing assets types
        Vector<const Type*> GetProcessingAssetsTypes() const override;

        // Copies the font and bakes its fields
        void ConvertAsset(const AssetInfo& node) override;

        // Removes the font and its fields
        void RemoveAsset(const AssetInfo& node) override;

        // Moves the font and its fields to new path
        void MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo) override;

        // Bakes anew the fields baked for another source, settings or generator version, returns their fonts
        Vector<UID> AssetsPostProcess() override;

        IOBJECT(VectorFontAssetConverter);

    protected:
        // Bakes the fields of the font when the meta asks and the baked ones are not current; removes them when the meta
        // doesn't ask. Returns true when the fields were baked
        bool BakeAtlas(const AssetInfo& node);
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorFontAssetConverter)
{
    BASE_CLASS(o2::IAssetConverter);
}
END_META;
CLASS_FIELDS_META(o2::VectorFontAssetConverter)
{
}
END_META;
CLASS_METHODS_META(o2::VectorFontAssetConverter)
{

    FUNCTION().PUBLIC().SIGNATURE(Vector<const Type*>, GetProcessingAssetsTypes);
    FUNCTION().PUBLIC().SIGNATURE(void, ConvertAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveAsset, const AssetInfo&, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<UID>, AssetsPostProcess);
    FUNCTION().PROTECTED().SIGNATURE(bool, BakeAtlas, const AssetInfo&);
}
END_META;
// --- END META ---
