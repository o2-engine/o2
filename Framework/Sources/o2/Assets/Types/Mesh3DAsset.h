#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"

namespace o2
{
    // -------------------------------------------------------
    // 3D mesh asset. Stores geometry as vertices with normals,
    // texture coordinates and triangle indices
    // -------------------------------------------------------
    class Mesh3DAsset: public AssetWithDefaultMeta<Mesh3DAsset>
    {
    public:
        Vector<Vec3F> vertices; // Vertices positions @SERIALIZABLE
        Vector<Vec3F> normals;  // Vertices normals @SERIALIZABLE
        Vector<Vec2F> uvs;      // Vertices texture coordinates @SERIALIZABLE
        Vector<UInt>  indices;  // Triangles indices @SERIALIZABLE

    public:
        // Default constructor
        Mesh3DAsset();

        // Copy-constructor
        Mesh3DAsset(const Mesh3DAsset& asset);

        // Check equals operator
        Mesh3DAsset& operator=(const Mesh3DAsset& asset);

        // Sets geometry from primitive builder output
        void SetFromPrimitive(const Mesh3DData& data);

        // Returns geometry as primitive data
        Mesh3DData GetMeshData() const;

        // Loads geometry from Wavefront OBJ text, returns false on parsing error
        bool LoadFromObj(const String& text, String* errorMessage = nullptr);

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_text_file_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 93; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return true; }

        SERIALIZABLE(Mesh3DAsset);
        CLONEABLE_REF(Mesh3DAsset);

    protected:
        // Loads data: OBJ parsing for .obj files, serialized document otherwise
        void LoadData(const String& path) override;

        // Saves data: OBJ text for .obj files, serialized document otherwise
        void SaveData(const String& path) const override;

        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::Mesh3DAsset)
{
    BASE_CLASS(o2::AssetWithDefaultMeta<Mesh3DAsset>);
}
END_META;
CLASS_FIELDS_META(o2::Mesh3DAsset)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(vertices);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(normals);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(uvs);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(indices);
}
END_META;
CLASS_METHODS_META(o2::Mesh3DAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Mesh3DAsset&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFromPrimitive, const Mesh3DData&);
    FUNCTION().PUBLIC().SIGNATURE(Mesh3DData, GetMeshData);
    FUNCTION().PUBLIC().SIGNATURE(bool, LoadFromObj, const String&, String*);
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
