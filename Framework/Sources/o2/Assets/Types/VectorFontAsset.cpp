#include "o2/stdafx.h"
#include "VectorFontAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"
#include "o2/Render/SdfFont.h"

#include <cstdlib>

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

    namespace
    {
        int ReadDistanceFieldOverride()
        {
            const char* value = std::getenv("O2_FONT_DISTANCE_FIELD");
            if (!value || !*value)
                return -1;

            return value[0] == '1' ? 1 : 0;
        }

        int gDistanceFieldOverride = ReadDistanceFieldOverride();
    }

    bool VectorFontAsset::IsDistanceField() const
    {
        if (gDistanceFieldOverride >= 0)
            return gDistanceFieldOverride == 1;

        auto meta = GetMeta();
        return meta && meta->distanceField;
    }

    void VectorFontAsset::SetDistanceFieldOverride(int mode)
    {
        gDistanceFieldOverride = mode < 0 ? -1 : (mode > 0 ? 1 : 0);
    }

    int VectorFontAsset::GetDistanceFieldOverride()
    {
        return gDistanceFieldOverride;
    }

    void VectorFontAsset::LoadData(const String& path)
    {
        bool distanceField = IsDistanceField();
        SdfFontSettings settings = GetMeta() ? GetMeta()->GetFieldSettings() : SdfFontSettings();

        mFont = o2Render.mFonts.FindOrDefault([&](auto fnt) {
            if (fnt->GetFileName() != path)
                return false;

            auto sdf = dynamic_cast<SdfFont*>(fnt.Get());
            return distanceField ? sdf && sdf->GetSettings() == settings : !sdf;
        });

        if (mFont)
            return;

        if (distanceField)
        {
            auto font = mmake<SdfFont>(path, settings);
            font->SetBakedFields(SdfFont::GetBakedFieldsPath(path));
            mFont = font;
        }
        else
            mFont = mmake<VectorFont>(path);
    }

    SdfFontSettings VectorFontAsset::Meta::GetFieldSettings() const
    {
        SdfFontSettings settings;
        settings.glyphSize = Math::Clamp(fieldGlyphSize, 8, 256);
        settings.distanceRange = Math::Clamp(fieldDistanceRange, 1.0f, 64.0f);
        settings.multiChannel = fieldMultiChannel;
        return settings;
    }

    bool VectorFontAsset::Meta::IsEqual(AssetMeta* other) const
    {
        if (!AssetMeta::IsEqual(other))
            return false;

        auto otherMeta = dynamic_cast<Meta*>(other);
        return otherMeta && distanceField == otherMeta->distanceField && fieldGlyphSize == otherMeta->fieldGlyphSize &&
            Math::Equals(fieldDistanceRange, otherMeta->fieldDistanceRange) &&
            fieldMultiChannel == otherMeta->fieldMultiChannel && fieldCharacters == otherMeta->fieldCharacters;
    }

    WString VectorFontAsset::Meta::GetFieldCharacters() const
    {
        return fieldCharacters.IsEmpty() ? SdfFont::GetDefaultCharacters() : fieldCharacters;
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
