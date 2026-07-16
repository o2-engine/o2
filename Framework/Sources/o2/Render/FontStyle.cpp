#include "o2/stdafx.h"
#include "FontStyle.h"

#include "o2/Utils/Serialization/DataValue.h"

namespace o2
{
    FontStyle::FontStyle()
    {}

    FontStyle::FontStyle(const FontStyle& other)
    {
        for (auto& eff : other.mEffects)
        {
            if (eff)
                mEffects.Add(eff->CloneAsRef<VectorFont::Effect>());
        }
    }

    FontStyle& FontStyle::operator=(const FontStyle& other)
    {
        mEffects.Clear();
        for (auto& eff : other.mEffects)
        {
            if (eff)
                mEffects.Add(eff->CloneAsRef<VectorFont::Effect>());
        }

        OnChanged();

        return *this;
    }

    Ref<VectorFont::Effect> FontStyle::AddEffect(const Ref<VectorFont::Effect>& effect)
    {
        mEffects.Add(effect);
        OnChanged();

        return effect;
    }

    void FontStyle::RemoveEffect(const Ref<VectorFont::Effect>& effect)
    {
        mEffects.Remove(effect);
        OnChanged();
    }

    void FontStyle::RemoveAllEffects()
    {
        mEffects.Clear();
        OnChanged();
    }

    void FontStyle::SetEffects(const Vector<Ref<VectorFont::Effect>>& effects)
    {
        mEffects = effects;
        OnChanged();
    }

    const Vector<Ref<VectorFont::Effect>>& FontStyle::GetEffects() const
    {
        return mEffects;
    }

    UInt64 FontStyle::GetCacheKey() const
    {
        if (mCacheKeyVersion != mVersion)
        {
            DataDocument doc;
            doc.Set(mEffects);
            String data = doc.SaveAsString();

            UInt64 hash = 14695981039346656037ull;
            for (int i = 0; i < data.Length(); i++)
            {
                hash ^= (UInt8)data[i];
                hash *= 1099511628211ull;
            }

            mCacheKey = hash;
            mCacheKeyVersion = mVersion;
        }

        return mCacheKey;
    }

    void FontStyle::OnChanged()
    {
        mVersion++;
        onChanged();
    }

    void FontStyle::OnDeserialized(const DataValue& node)
    {
        OnChanged();
    }
}
// --- META ---

DECLARE_CLASS(o2::FontStyle, o2__FontStyle);
// --- END META ---
