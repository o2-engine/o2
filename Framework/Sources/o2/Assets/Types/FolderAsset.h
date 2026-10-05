#pragma once

#include "o2/Assets/Asset.h"

namespace o2
{
    // ------------
    // Folder asset
    // ------------
    class FolderAsset: public AssetWithDefaultMeta<FolderAsset>
    {
    public:
        // Default constructor
        FolderAsset();

        // Copy-constructor
        FolderAsset(const FolderAsset& asset);

        // Check equals operator
        FolderAsset& operator=(const FolderAsset& asset);

        // Returns containing assets infos
        Vector<AssetRef<Asset>> GetChildrenAssets() const;

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_folder_icon.svg"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 100; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        SERIALIZABLE(FolderAsset);
        CLONEABLE_REF(FolderAsset);

    protected:
        // Loads data
        void LoadData(const String& path) override;

        // Saves asset data
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::FolderAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<FolderAsset>);
}
END_META;
CLASS_FIELDS_META(o2::FolderAsset)
{
}
END_META;
CLASS_METHODS_META(o2::FolderAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FolderAsset&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<AssetRef<Asset>>, GetChildrenAssets);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;
// --- END META ---
