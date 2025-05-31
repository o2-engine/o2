#include "o2/stdafx.h"
#include "VectorFontAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"

namespace o2
{
    VectorFontAsset::Meta::Meta()
    {}

    VectorFontAsset::Meta::Meta(const Meta& other):
        DefaultAssetMeta<VectorFontAsset>(other)
    {
        for (auto& eff : other.mEffects)
            mEffects.Add(eff->CloneAsRef<VectorFont::Effect>());
    }

    bool VectorFontAsset::Meta::IsEqual(AssetMeta* other) const
    {
        if (!AssetMeta::IsEqual(other))
            return false;

        Meta* otherMeta = (Meta*)other;
        for (auto& eff : mEffects)
        {
            bool found = false;
            for (auto& otherEff : otherMeta->mEffects)
            {
                if (eff && otherEff && eff->IsEqual(otherEff.Get()))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                return false;
        }

        return true;
    }

    void VectorFontAsset::Meta::UpdateFontEffects()
    {
        if (mAsset)
            mAsset.Lock()->UpdateFontEffects();
    }

    VectorFontAsset::VectorFontAsset():
        FontAsset(mmake<Meta>())
    {}

    VectorFontAsset::VectorFontAsset(const VectorFontAsset& asset):
        FontAsset(asset), meta(this)
    {}

    VectorFontAsset& VectorFontAsset::operator=(const VectorFontAsset& asset)
    {
        FontAsset::operator=(asset);
        mFont = asset.mFont;
        return *this;
    }

    const Vector<Ref<VectorFont::Effect>>& VectorFontAsset::GetEffects() const
    {
        return GetMeta()->mEffects;
    }

    void VectorFontAsset::AddEffect(const Ref<VectorFont::Effect>& effect)
    {
        GetMeta()->mEffects.Add(effect);
        DynamicCast<VectorFont>(mFont)->AddEffect(effect);
    }

    void VectorFontAsset::RemoveEffect(const Ref<VectorFont::Effect>& effect)
    {
        GetMeta()->mEffects.Remove(effect);
        DynamicCast<VectorFont>(mFont)->RemoveEffect(effect);
    }

    void VectorFontAsset::RemoveAllEffects()
    {
        GetMeta()->mEffects.Clear();

        DynamicCast<VectorFont>(mFont)->RemoveAllEffects();
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
        {
            mFont = mmake<VectorFont>(path);
            UpdateFontEffects();
        }
        
        GetMeta()->mAsset = WeakRef(this);
    }

    void VectorFontAsset::SaveData(const String& path) const
    {}

    void VectorFontAsset::UpdateFontEffects()
    {
        Vector<Ref<VectorFont::Effect>> clonedEffects;;
        for (auto& eff : GetMeta()->mEffects)
        {
            if (eff)
                clonedEffects.Add(eff->CloneAsRef<VectorFont::Effect>());
        }

        DynamicCast<VectorFont>(mFont)->SetEffects(clonedEffects);
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VectorFontAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VectorFontAsset>);
// --- META ---

DECLARE_CLASS(o2::VectorFontAsset, o2__VectorFontAsset);

DECLARE_CLASS(o2::VectorFontAsset::Meta, o2__VectorFontAsset__Meta);
// --- END META ---
