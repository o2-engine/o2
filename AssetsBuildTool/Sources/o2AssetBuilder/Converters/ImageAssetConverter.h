#pragma once

#include "IAssetConverter.h"

namespace o2
{
    // ---------------------
    // Image asset converter
    // ---------------------
    class ImageAssetConverter: public IAssetConverter
    {
    public:
        // Returns vector of processing assets types
        Vector<const Type*> GetProcessingAssetsTypes() const override;

        // Converts image
        void ConvertAsset(const AssetInfo& node) override;

        // Removes image
        void RemoveAsset(const AssetInfo& node) override;

        // Moves image to new path
        void MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo) override;

        IOBJECT(ImageAssetConverter);
    };
}
// --- META ---

CLASS_BASES_META(o2::ImageAssetConverter)
{
    BASE_CLASS(o2::IAssetConverter);
}
END_META;
CLASS_FIELDS_META(o2::ImageAssetConverter)
{
}
END_META;
CLASS_METHODS_META(o2::ImageAssetConverter)
{

    FUNCTION().PUBLIC().SIGNATURE(Vector<const Type*>, GetProcessingAssetsTypes);
    FUNCTION().PUBLIC().SIGNATURE(void, ConvertAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveAsset, const AssetInfo&, const AssetInfo&);
}
END_META;
// --- END META ---
