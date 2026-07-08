#include "o2/stdafx.h"
#include "Mesh3DAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/ObjMeshFormat.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    Mesh3DAsset::Mesh3DAsset()
    {}

    Mesh3DAsset::Mesh3DAsset(const Mesh3DAsset& other):
        AssetWithDefaultMeta<Mesh3DAsset>(other), vertices(other.vertices), normals(other.normals),
        uvs(other.uvs), indices(other.indices)
    {}

    Mesh3DAsset& Mesh3DAsset::operator=(const Mesh3DAsset& other)
    {
        Asset::operator=(other);

        vertices = other.vertices;
        normals = other.normals;
        uvs = other.uvs;
        indices = other.indices;

        return *this;
    }

    void Mesh3DAsset::SetFromPrimitive(const Mesh3DData& data)
    {
        vertices = data.positions;
        normals = data.normals;
        uvs = data.uvs;
        indices = data.indices;
    }

    Mesh3DData Mesh3DAsset::GetMeshData() const
    {
        return { vertices, normals, uvs, indices };
    }

    bool Mesh3DAsset::LoadFromObj(const String& text, String* errorMessage /*= nullptr*/)
    {
        Mesh3DData data;
        if (!ObjMeshFormat::Parse(text, data, errorMessage))
            return false;

        SetFromPrimitive(data);
        return true;
    }

    void Mesh3DAsset::LoadData(const String& path)
    {
        if (FileSystem::GetFileExtension(path).ToLowerCase() == "obj")
        {
            String error;
            if (!LoadFromObj(o2FileSystem.ReadFile(path), &error))
                o2Debug.LogError("Failed to load obj mesh " + path + ": " + error);
        }
        else
            Asset::LoadData(path);
    }

    void Mesh3DAsset::SaveData(const String& path) const
    {
        if (FileSystem::GetFileExtension(path).ToLowerCase() == "obj")
            o2FileSystem.WriteFile(path, ObjMeshFormat::Write(GetMeshData()));
        else
            Asset::SaveData(path);
    }

    Vector<String> Mesh3DAsset::GetFileExtensions()
    {
        return { "mesh3d", "obj" };
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::Mesh3DAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::Mesh3DAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::Mesh3DAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::Mesh3DAsset>>);
// --- META ---

DECLARE_CLASS(o2::Mesh3DAsset, o2__Mesh3DAsset);
// --- END META ---
