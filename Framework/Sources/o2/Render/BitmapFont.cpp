#include "o2/stdafx.h"
#include "BitmapFont.h"

#include "o2/Assets/Types/ImageAsset.h"
#include "3rdPartyLibs/pugixml/pugixml.hpp"
#include "o2/Render/Render.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
	BitmapFont::BitmapFont():
		Font()
	{}

	BitmapFont::BitmapFont(const String& fileName):
		Font()
	{
		Load(fileName);
	}

	bool BitmapFont::Load(const String& fileName)
	{
		mFileName = fileName;

		pugi::xml_document doc;
		auto res = doc.load_file(fileName.Data());

		if (res.status != pugi::status_ok)
		{
			o2Render.mLog->Error("Failed to load Bitmap Font file: " + fileName);
			return false;
		}

		pugi::xml_node root = doc.child(L"font");

		if (pugi::xml_node commonNode = root.child(L"common"))
		{
			String textureName = commonNode.attribute(L"texture").value();
			ImageAssetRef image(textureName);
			mTexture = image->GetAtlasTextureRef();
			mTextureSrcRect = image->GetAtlasRect();

			mBaseHeight = commonNode.attribute(L"base").as_float();
			mLineHeight = commonNode.attribute(L"lineHeight").as_float();
		}
		else
		{
			o2Render.mLog->Error("Failed to get common info in font: " + fileName + ". Bad file format");
			return false;
		}

		if (pugi::xml_node charsNode = root.child(L"chars"))
		{
			Vec2F texOffs = mTextureSrcRect.LeftBottom();
			for (pugi::xml_node charNode = charsNode.child(L"char"); charNode; charNode = charNode.next_sibling(L"char"))
			{
				Character newChar;

				newChar.mTexSrc.left = charNode.attribute(L"x").as_float();
				newChar.mTexSrc.top = charNode.attribute(L"y").as_float();
				newChar.mTexSrc.right = charNode.attribute(L"width").as_float() + newChar.mTexSrc.left;
				newChar.mTexSrc.bottom = charNode.attribute(L"height").as_float() + newChar.mTexSrc.top;
				newChar.mTexSrc += texOffs;

				newChar.mOrigin.x = -charNode.attribute(L"xoffset").as_float();
				newChar.mOrigin.y = mLineHeight - mBaseHeight;

				newChar.mAdvance = charNode.attribute(L"xadvance").as_float();

				newChar.mId = charNode.attribute(L"id").as_uint();

				newChar.mHeight = 0;

				AddCharacter(newChar);
			}
		}
		else
		{
			o2Render.mLog->Error("Failed to get characters node in BMFont file: " + fileName + ". Bad file format");
			return false;
		}

		Vec2F invTexSize(1.0f/mTexture->GetSize().x, 1.0f/mTexture->GetSize().y);
		for (auto heightKV : mCharacters)
		{
			for (auto charKV : heightKV.second)
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
