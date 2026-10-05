#include "o2/stdafx.h"
#include "Font.h"

#include "o2/Render/FontStyle.h"
#include "o2/Render/Render.h"

namespace o2
{
    Font::Font()
    {}

    Font::Font(const Font& font):
        mCharacters(font.mCharacters), mTexture(font.mTexture), 
        mTextureSrcRect(font.mTextureSrcRect), mReady(font.mReady)
    {}
    
    void Font::PostRefConstruct()
    {
        if (Render::IsSingletonInitialzed())
            o2Render.OnFontCreated(this);
    }

    Font::~Font()
    {
		if (Render::IsSingletonInitialzed())
			o2Render.OnFontDestroyed(this);
    }

    float Font::GetHeightPx(int height) const
    {
        return 1.0f;
    }

    float Font::GetLineHeightPx(int height) const
    {
        return 1.0f;
    }

    const Font::Character& Font::GetCharacter(UInt16 id, int height)
    {
        return GetCharacter(id, height, nullptr);
    }

    const Font::Character& Font::GetCharacter(UInt16 id, int height, const Ref<FontStyle>& style,
                                              float pixelDensity /*= 1.0f*/)
    {
        int styleId = GetStyleId(style);
        auto fndStyleHeight = mCharacters.find(GetStyleHeightKey(styleId, height, styleId != 0 ? 1.0f : pixelDensity));
        if (fndStyleHeight != mCharacters.End())
        {
            auto fndChar = fndStyleHeight->second.find(id);
            if (fndChar != fndStyleHeight->second.End())
                return fndChar->second;
        }

        static Character empty;
        return empty;
    }

    void Font::CheckCharacters(const WString& needChararacters, int height)
    {
        CheckCharacters(needChararacters, height, nullptr);
    }

    void Font::CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style,
                               float pixelDensity /*= 1.0f*/)
    {}

    int Font::GetStyleId(const Ref<FontStyle>& style)
    {
        return 0;
    }

    UInt64 Font::GetStyleHeightKey(int styleId, int height, float pixelDensity /*= 1.0f*/)
    {
        UInt64 densityStep = (UInt64)Math::RoundToInt(pixelDensity*4.0f) & 0xFF;
        return ((UInt64)(UInt32)styleId << 32) | (densityStep << 24) | ((UInt64)(UInt32)height & 0xFFFFFF);
    }

    String Font::GetFileName() const
    {
        return String();
    }

    const TextureRef& Font::GetTexture() const
    {
        return mTexture;
    }

    const RectI& Font::GetTextureSrcRect() const
    {
        return mTextureSrcRect;
    }

    void Font::AddCharacter(const Character& character)
    {
        mCharacters[GetStyleHeightKey(character.mStyleId, character.mHeight, character.mPixelDensity)][character.mId] =
            character;
    }

    bool Font::Character::operator==(const Character& other) const
    {
        return mId == other.mId && mHeight == other.mHeight && mStyleId == other.mStyleId;
    }
}
