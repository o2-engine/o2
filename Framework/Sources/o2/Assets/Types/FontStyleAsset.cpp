#include "o2/stdafx.h"
#include "FontStyleAsset.h"

#include "o2/Assets/Assets.h"

namespace o2
{
    FontStyleAsset::FontStyleAsset():
        AssetWithDefaultMeta<FontStyleAsset>(),
        FontStyle()
    {}

    FontStyleAsset::FontStyleAsset(const FontStyleAsset& asset):
        AssetWithDefaultMeta<FontStyleAsset>(asset),
        FontStyle(asset)
    {}

    FontStyleAsset& FontStyleAsset::operator=(const FontStyleAsset& asset)
    {
        Asset::operator=(asset);
        FontStyle::operator=(asset);

        return *this;
    }

    Vector<String> FontStyleAsset::GetFileExtensions()
    {
        return { "fntstyle" };
    }

    Ref<RefCounterable> FontStyleAsset::CastToRefCounterable(const Ref<FontStyleAsset>& ref)
    {
        return DynamicCast<Asset>(ref);
    }

    void FontStyleAsset::OnDeserialized(const DataValue& node)
    {
        FontStyle::OnDeserialized(node);
    }

    void FontStyleAsset::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }
}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::FontStyleAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::FontStyleAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::FontStyleAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::FontStyleAsset>>);
// --- META ---

DECLARE_CLASS(o2::FontStyleAsset, o2__FontStyleAsset);
// --- END META ---
