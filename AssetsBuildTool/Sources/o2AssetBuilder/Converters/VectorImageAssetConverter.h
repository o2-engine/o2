#pragma once

#include "IAssetConverter.h"

namespace o2
{
    // -----------------------------------------------------------------------------------------------
    // Vector image asset converter: copies the SVG and bakes the mesh for any scale when the meta asks
    // -----------------------------------------------------------------------------------------------
    class VectorImageAssetConverter: public IAssetConverter
    {
    public:
        // Returns vector of processing assets types
        Vector<const Type*> GetProcessingAssetsTypes() const override;

        // Copies the image and bakes its mesh
        void ConvertAsset(const AssetInfo& node) override;

        // Removes the image and its mesh
        void RemoveAsset(const AssetInfo& node) override;

        // Moves the image and its mesh to new path
        void MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo) override;

        // Bakes anew the meshes of another tessellation version, returns their images
        Vector<UID> AssetsPostProcess() override;

        IOBJECT(VectorImageAssetConverter);
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorImageAssetConverter)
{
    BASE_CLASS(o2::IAssetConverter);
}
END_META;
CLASS_FIELDS_META(o2::VectorImageAssetConverter)
{
}
END_META;
CLASS_METHODS_META(o2::VectorImageAssetConverter)
{

    FUNCTION().PUBLIC().SIGNATURE(Vector<const Type*>, GetProcessingAssetsTypes);
    FUNCTION().PUBLIC().SIGNATURE(void, ConvertAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAsset, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveAsset, const AssetInfo&, const AssetInfo&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<UID>, AssetsPostProcess);
}
END_META;
// --- END META ---
