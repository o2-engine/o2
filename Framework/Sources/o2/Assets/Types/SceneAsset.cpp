#include "o2/stdafx.h"
#include "SceneAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    SceneAsset::SceneAsset()
    {}

    SceneAsset::SceneAsset(const SceneAsset& other):
        AssetWithDefaultMeta<SceneAsset>(other)
    {}

    SceneAsset::~SceneAsset()
    {}

    SceneAsset& SceneAsset::operator=(const SceneAsset& other)
    {
        Asset::operator=(other);
        return *this;
    }

    void SceneAsset::Load() const
    {
        if (GetPath().IsEmpty())
            o2Scene.Clear();
        else    
            o2Scene.Load(GetBuiltFullPath());
    }

    Vector<String> SceneAsset::GetFileExtensions()
    {
        return { "scn" };
    }

    void SceneAsset::LoadData(const String& path)
    {}

    void SceneAsset::SaveData(const String& path) const
    {}
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::SceneAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::SceneAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::SceneAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::SceneAsset>>);
// --- META ---

DECLARE_CLASS(o2::SceneAsset, o2__SceneAsset);
// --- END META ---
