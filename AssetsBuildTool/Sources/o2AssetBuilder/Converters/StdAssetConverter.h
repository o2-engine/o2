#pragma once

#include "IAssetConverter.h"

namespace o2
{
    // -----------------------------------------------------------------
    // Standard assets converter. Copying file and meta without changing
    // -----------------------------------------------------------------
    class StdAssetConverter: public IAssetConverter
    {
    public:
        // Returns vector of processing assets types
        Vector<const Type*> GetProcessingAssetsTypes() const override;

        // Copies asset
        void ConvertAsset(const AssetInfo& node) override;

        // Removes asset
        void RemoveAsset(const AssetInfo& node) override;

        // Moves asset to new path
        void MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo) override;

        IOBJECT(StdAssetConverter);
    };
}
// --- META ---

CLASS_BASES_META(o2::StdAssetConverter)
{
    BASE_CLASS(o2::IAssetConverter);
}
END_META;
CLASS_FIELDS_META(o2::StdAssetConverter)
{
}
END_META;
CLASS_METHODS_META(o2::StdAssetConverter)
{

    FUNCTION().PUBLIC().SIGNATURE(Vector<const Type*>, GetProcessingAssetsTypes);
    FUNCTION().PUBLIC().SIGNATURE(void, ConvertAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveAsset, const AssetInfo&, const AssetInfo&);
}
END_META;
// --- END META ---
