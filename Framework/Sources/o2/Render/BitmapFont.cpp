#include "o2/stdafx.h"
#include "BitmapFont.h"

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    BitmapFont::BitmapFont() :
        Font()
    {}

    BitmapFont::BitmapFont(const String& fileName) :
        Font()
    {
        Load(fileName);
    }

    bool BitmapFont::Load(const String& fileName)
    {
        mFileName = fileName;

        DataDocument doc;
        if (!doc.LoadFromFile(fileName))
        {
            o2Render.mLog->Error("Failed to load Bitmap Font file: " + fileName);
            return false;
        }

        if (auto common = doc.FindMember("common"))
        {
            String textureName = (*common)["texture"];
            AssetRef<ImageAsset> image(textureName);
            if (image)
            {
                TextureSource imageSource = image->GetTextureSource();
                mTexture = imageSource.texture;
                mTextureSrcRect = imageSource.sourceRect;

                mBaseHeight = (*common)["base"];
                mLineHeight = (*common)["lineHeight"];
            }
        }
        else
        {
            o2Render.mLog->Error("Failed to get common info in font: " + fileName + ". Bad file format");
            return false;
        }

        if (!mTexture)
        {
			o2Render.mLog->Error("Failed to load texture for Bitmap Font: " + fileName + ". Texture not found");
			return false;
        }

        if (auto chars = doc.FindMember("chars"))
        {
            Vec2F invTexSize = Vec2F(1, 1)/mTexture->GetSize();
            Vec2F texOffs = Vec2F(mTextureSrcRect.LeftBottom())*invTexSize;
            for (auto& child : *chars)
            {
                Character newChar;

                newChar.mTexSrc.left = (float)child["x"];
                newChar.mTexSrc.top = (float)child["y"];
                newChar.mTexSrc.right = (float)child["width"] + newChar.mTexSrc.left;
                newChar.mTexSrc.bottom = (float)child["height"] + newChar.mTexSrc.top;

                newChar.mSize = newChar.mTexSrc.Size().InvertedY();

                newChar.mTexSrc.left *= invTexSize.x;
                newChar.mTexSrc.bottom *= invTexSize.y;
                newChar.mTexSrc.right *= invTexSize.x;
                newChar.mTexSrc.top *= invTexSize.y;

                newChar.mTexSrc += texOffs;

                newChar.mOrigin.x = -(float)(child["xoffset"]);
                newChar.mOrigin.y = mLineHeight - mBaseHeight;

                newChar.mAdvance = child["xadvance"];

                newChar.mId = child["letter"].GetString()[0];
                 
                newChar.mHeight = 0;

                AddCharacter(newChar);
            }
        }
        else
        {
            o2Render.mLog->Error("Failed to get characters node in BMFont file: " + fileName + ". Bad file format");
            return false;
        }

        Vec2F invTexSize(1.0f / mTexture->GetSize().x, 1.0f / mTexture->GetSize().y);
        for (auto& heightKV : mCharacters)
        {
            for (auto& charKV : heightKV.second)
            {
                charKV.second.mSize = charKV.second.mTexSrc.Size().InvertedY();
                charKV.second.mTexSrc.left *= invTexSize.x;
                charKV.second.mTexSrc.right *= invTexSize.x;
                charKV.second.mTexSrc.top *= invTexSize.y;
                charKV.second.mTexSrc.bottom *= invTexSize.y;
            }
        }

        mReady = true;
        return true;
    }

    String BitmapFont::GetFileName() const
    {
        return mFileName;
    }

    float BitmapFont::GetHeightPx(int height) const
    {
        return mBaseHeight;
    }

    float BitmapFont::GetLineHeightPx(int height) const
    {
        return mLineHeight;
    }

    const Font::Character& BitmapFont::GetCharacter(UInt16 id, int height)
    {
        return Font::GetCharacter(id, 0);
    }

}
