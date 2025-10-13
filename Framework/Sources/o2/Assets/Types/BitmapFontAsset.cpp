#include "o2/stdafx.h"
#include "BitmapFontAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/BitmapFont.h"
#include "o2/Render/Font.h"
#include "o2/Render/Render.h"

namespace o2
{
    BitmapFontAsset::BitmapFontAsset():
        FontAsset(mmake<Meta>())
    {}

    BitmapFontAsset::BitmapFontAsset(const BitmapFontAsset& asset):
        FontAsset(asset)
    {}

    BitmapFontAsset& BitmapFontAsset::operator=(const BitmapFontAsset& asset)
    {
        FontAsset::operator=(asset);
        return *this;
    }

    Vector<String> BitmapFontAsset::GetFileExtensions()
    {
        return { "fnt" };
    }

    Ref<BitmapFontAsset::Meta> BitmapFontAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    void BitmapFontAsset::LoadData(const String& path)
    {
        mFont = o2Render.mFonts.FindOrDefault([&](auto fnt) { return fnt->GetFileName() == path; });

        if (!mFont)
            mFont = mmake<BitmapFont>(path);
    }
}
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::BitmapFontAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::BitmapFontAsset>);
// --- META ---

DECLARE_CLASS(o2::BitmapFontAsset, o2__BitmapFontAsset);

DECLARE_CLASS(o2::BitmapFontAsset::Meta, o2__BitmapFontAsset__Meta);
// --- END META ---
