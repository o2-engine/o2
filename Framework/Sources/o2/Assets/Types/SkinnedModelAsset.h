#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/SkinnedModelFormat.h"

namespace o2
{
    // ---------------------------------------------------------------------------
    // Skinned model asset. Loads GLB (binary glTF 2.0) files with a skinned mesh,
    // skeleton and animation clips; see SkinnedModelData for the supported subset
    // ---------------------------------------------------------------------------
    class SkinnedModelAsset: public AssetWithDefaultMeta<SkinnedModelAsset>
    {
    public:
        // Default constructor
        SkinnedModelAsset();

        // Copy-constructor
        SkinnedModelAsset(const SkinnedModelAsset& asset);

        // Assign operator
        SkinnedModelAsset& operator=(const SkinnedModelAsset& asset);

        // Returns parsed model data
        const SkinnedModelData& GetModelData() const;

        // Sets model data directly (tests and procedural models)
        void SetModelData(const SkinnedModelData& data);

        // Parses model from GLB binary data, returns false on parsing error
        bool LoadFromGlb(const UInt8* data, UInt size, String* errorMessage = nullptr);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_text_file_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 92; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return false; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        SERIALIZABLE(SkinnedModelAsset);
        CLONEABLE_REF(SkinnedModelAsset);

    protected:
        SkinnedModelData mModelData; // Parsed model data
        Vector<UInt8>    mRawData;   // Source GLB bytes, written back on save

    protected:
        // Loads data: GLB parsing for .glb files, serialized document otherwise
        void LoadData(const String& path) override;

        // Saves data: original GLB bytes for .glb files, serialized document otherwise
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::SkinnedModelAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<SkinnedModelAsset>);
}
END_META;
CLASS_FIELDS_META(o2::SkinnedModelAsset)
{
    FIELD().PROTECTED().NAME(mModelData);
    FIELD().PROTECTED().NAME(mRawData);
}
END_META;
CLASS_METHODS_META(o2::SkinnedModelAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SkinnedModelAsset&);
    FUNCTION().PUBLIC().SIGNATURE(const SkinnedModelData&, GetModelData);
    FUNCTION().PUBLIC().SIGNATURE(void, SetModelData, const SkinnedModelData&);
    FUNCTION().PUBLIC().SIGNATURE(bool, LoadFromGlb, const UInt8*, UInt, String*);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
}
END_META;
// --- END META ---
