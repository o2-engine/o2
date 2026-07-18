#include "o2/stdafx.h"
#include "VectorFontAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"

namespace o2
{
    VectorFontAsset::VectorFontAsset():
        FontAsset(mmake<Meta>())
    {}

    VectorFontAsset::VectorFontAsset(const VectorFontAsset& asset):
        FontAsset(asset)
    {}

    VectorFontAsset& VectorFontAsset::operator=(const VectorFontAsset& asset)
    {
        FontAsset::operator=(asset);
        mFont = asset.mFont;
        return *this;
    }

    Vector<String> VectorFontAsset::GetFileExtensions()
    {
        return { "ttf" };
    }

    Ref<VectorFontAsset::Meta> VectorFontAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    void VectorFontAsset::LoadData(const String& path)
    {
        mFont = o2Render.mFonts.FindOrDefault([&](auto fnt) { return fnt->GetFileName() == path; });

        if (!mFont)
            mFont = mmake<VectorFont>(path);
    }

    void VectorFontAsset::SaveData(const String& path) const
    {}
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VectorFontAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VectorFontAsset>);
// --- META ---

DECLARE_CLASS(o2::VectorFontAsset, o2__VectorFontAsset);

DECLARE_CLASS(o2::VectorFontAsset::Meta, o2__VectorFontAsset__Meta);
// --- END META ---
