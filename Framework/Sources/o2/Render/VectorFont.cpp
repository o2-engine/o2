#include "o2/stdafx.h"
#include "VectorFont.h"

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/OpenGL.h"
#elif defined PLATFORM_ANDROID
#include "o2/Render/Android/OpenGL.h"
#endif

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/System/Time/Timer.h"

namespace o2
{
	VectorFont::VectorFont() :
		Font(), mFreeTypeFace(nullptr)
	{
		mTexture = TextureRef(Vec2I(512, 512));
		mTextureSrcRect.Set(0, 0, 512, 512);
	}

	VectorFont::VectorFont(const String& fileName) :
		Font(), mFreeTypeFace(nullptr)
	{
		mTexture = TextureRef(Vec2I(512, 512));
		mTextureSrcRect.Set(0, 0, 512, 512);

		Load(fileName);
	}

	VectorFont::VectorFont(const VectorFont& other) :
		Font(), mFreeTypeFace(other.mFreeTypeFace)
	{
		mTexture = TextureRef(Vec2I(512, 512));
		mTextureSrcRect.Set(0, 0, 512, 512);
	}

	VectorFont::~VectorFont()
	{
		if (mFreeTypeFace)
			FT_Done_Face(mFreeTypeFace);
	}

	const char* GetFreeTypeErrorMessage(FT_Error err)
	{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H
		return "(Unknown error)";
	}

	bool VectorFont::Load(const String& fileName)
	{
		InFile file(fileName);

		if (!file.IsOpened())
		{
			o2Render.mLog->Error("Failed to load vector font: " + fileName);
			return false;
		}

		UInt8* data = mnew UInt8[file.GetDataSize()];
		file.ReadFullData(data);

		FT_Error error = FT_New_Memory_Face(o2Render.mFreeTypeLib, data, file.GetDataSize(), 0, &mFreeTypeFace);

		//delete[] data;

		if (error)
		{
			o2Render.mLog->Error("Failed to load vector font: " + fileName + ", error: " + (String)GetFreeTypeErrorMessage(error));
			return false;
		}

		mFileName = fileName;

		return true;
	}

	// 	void VectorFont::SetSize(UInt size)
	// 	{
	// 		if (size == mSize)
	// 			return;
	// 
	// 		mSize = size;
	// 		Vec2I dpi = o2Render.GetDPI();
	// 		FT_Error error = FT_Set_Char_Size(mFreeTypeFace, 0, mSize*64, dpi.x, dpi.y);
	// 
	// 		FT_Load_Char(mFreeTypeFace, 'A', FT_LOAD_RENDER);
	// 		mBaseHeight = mFreeTypeFace->glyph->metrics.horiBearingY/64.0f;
	// 		mLineHeight = mBaseHeight*2.0f;
	// 
	// // 		mBaseHeight = mFreeTypeFace->ascender/64.0f;
	// // 		mLineHeight = (mFreeTypeFace->ascender - mFreeTypeFace->descender)/64.0f + 5.0f;
	// 
	// 		Reset();
	// 	}

	String VectorFont::GetFileName() const
	{
		return mFileName;
	}

	float VectorFont::GetHeightPx(int height) const
	{
		float res = 0;
		if (mHeights.TryGetValue(height, res))
			return res;

		Vec2I dpi = o2Render.GetDPI();
		FT_Set_Char_Size(mFreeTypeFace, 0, height * 64, dpi.x, dpi.y);

		FT_Load_Char(mFreeTypeFace, 'A', FT_LOAD_RENDER);

		res = mFreeTypeFace->glyph->metrics.horiBearingY / 64.0f;
		mHeights.Add(height, res);

		return res;
	}

	float VectorFont::GetLineHeightPx(int height) const
	{
		return GetHeightPx(height) * 2.0f;
	}

	void VectorFont::CheckCharacters(const WString& needChararacters, int height)
	{
		int len = needChararacters.Length();
		Vector<wchar_t> needToRenderChars;
		needToRenderChars.Reserve(len);

		for (int i = 0; i < len; i++)
		{
			bool isNew = true;
			wchar_t c = needChararacters[i];
			auto fndHeight = mCharacters.find(height);
			if (fndHeight != mCharacters.End())
				isNew = fndHeight->second.find(c) == fndHeight->second.End();

			if (isNew)
				isNew = !needToRenderChars.Contains(c);

			if (!isNew)
				continue;

			needToRenderChars.Add(c);
		}

		if (needToRenderChars.Count() > 0)
			UpdateCharacters(needToRenderChars, height);
	}

	const Ref<VectorFont::Effect>& VectorFont::AddEffect(const Ref<Effect>& effect)
	{
		mEffects.Add(effect);
		Reset();

		return mEffects.Last();
	}

	void VectorFont::RemoveEffect(const Ref<Effect>& effect)
	{
		mEffects.Remove(effect);
		Reset();
	}

	void VectorFont::RemoveAllEffects()
	{
		mEffects.Clear();
		Reset();
	}

	void VectorFont::SetEffects(const Vector<Ref<Effect>>& effects)
	{
		mEffects = effects;
		Reset();
	}

	const Vector<Ref<VectorFont::Effect>>& VectorFont::GetEffects() const
	{
		return mEffects;
	}

	void VectorFont::Reset()
	{
		mCharacters.Clear();
		onCharactersRebuilt();
	}

	void VectorFont::UpdateCharacters(Vector<wchar_t>& newCharacters, int height)
	{
		RenderNewCharacters(newCharacters, height);
		//onCharactersRebuilt();
	}

	void VectorFont::RenderNewCharacters(Vector<wchar_t>& newCharacters, int height)
	{
		if (!mFreeTypeFace)
			return;

		Vec2I dpi = o2Render.GetDPI();
		FT_Set_Char_Size(mFreeTypeFace, 0, height * 64, dpi.x, dpi.y);

		Vec2I border;
		for (auto effect : mEffects)
		{
			Vec2I effectExt = effect->GetSizeExtend();
			border.x = Math::Max(border.x, effectExt.x);
			border.y = Math::Max(border.y, effectExt.y);
		}

		border += Vec2I(2, 2);

		FT_Load_Char(mFreeTypeFace, 'A', FT_LOAD_RENDER);
		int symbolsHeight = Math::CeilToInt((mFreeTypeFace->glyph->bitmap.rows + border.y * 2) * 1.25f);

		for (auto ch : newCharacters)
		{
			CharDef newCharDef;

			FT_Load_Char(mFreeTypeFace, ch, FT_LOAD_RENDER);
			auto glyph = mFreeTypeFace->glyph;

			Vec2I glyphSize(glyph->bitmap.width, glyph->bitmap.rows);

			Ref<Bitmap> newBitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, glyphSize + border * 2);
			newBitmap->Fill(Color4(255, 255, 255, 0));
			UInt8* newBitmapData = newBitmap->GetData();
			Vec2I newBitmapSize = newBitmap->GetSize();

			for (int x = 0; x < (int)glyph->bitmap.width; x++)
			{
				for (int y = 0; y < (int)glyph->bitmap.rows; y++)
				{
					Color4 c(255, 255, 255, glyph->bitmap.buffer[y * glyph->bitmap.width + x]);
					ULong cl = c.ABGR();
					memcpy(&newBitmapData[((newBitmapSize.y - y - 1 - border.y) * newBitmapSize.x + x + border.x) * 4], &cl, 4);
				}
			}

			for (auto effect : mEffects)
				effect->Process(newBitmap);

			newCharDef.bitmap = newBitmap;
			newCharDef.character.mId = ch;
			newCharDef.character.mHeight = height;
			newCharDef.character.mSize = newBitmapSize;
			newCharDef.character.mAdvance = glyph->advance.x / 64.0f;
			newCharDef.character.mOrigin.x = -glyph->metrics.horiBearingX / 64.0f + border.x;
			newCharDef.character.mOrigin.y = (glyph->metrics.height - glyph->metrics.horiBearingY) / 64.0f + border.y;

			PackCharacter(newCharDef, symbolsHeight);
		}
	}

	void VectorFont::PackCharacter(CharDef& character, int height)
	{
		Ref<PackLine> packLine;

		while (!packLine)
		{
			packLine = mPackLines.FindOrDefault(
				[&](const Ref<PackLine>& x) { return x->height >= height && x->length + character.bitmap->GetSize().x < mTexture->GetSize().x; }
			);

			if (!packLine)
			{
				if (mLastPackLinePos + height <= mTexture->GetSize().y)
				{
					packLine = mmake<PackLine>();
					packLine->position = mLastPackLinePos;
					packLine->height = height;

					mPackLines.Add(packLine);

					mLastPackLinePos += height;
				}
				else
				{
					TextureRef lastTexture = mTexture;
					mTexture = TextureRef(lastTexture->GetSize() * 2, PixelFormat::R8G8B8A8, Texture::Usage::Default);
					mTexture->Copy(*lastTexture.Get(), RectI(Vec2I(0, 0), lastTexture->GetSize()));

					for (auto heightKV : mCharacters)
					{
						for (auto charKV : heightKV.second)
						{
							charKV.second.mTexSrc.left *= 0.5f;
							charKV.second.mTexSrc.right *= 0.5f;
							charKV.second.mTexSrc.top *= 0.5f;
							charKV.second.mTexSrc.bottom *= 0.5f;
						}
					}
				}
			}
		}

		character.packLine = packLine.Get();
		packLine->characters.Add(character);

		character.rect.left = packLine->length;
		character.rect.top = packLine->position + character.bitmap->GetSize().y;
		character.rect.right = packLine->length + character.bitmap->GetSize().x;
		character.rect.bottom = packLine->position;;

		packLine->length += character.bitmap->GetSize().x;

		Vec2F invTexSize(1.0f / mTexture->GetSize().x, 1.0f / mTexture->GetSize().y);
		character.character.mTexSrc.left = character.rect.left * invTexSize.x;
		character.character.mTexSrc.right = character.rect.right * invTexSize.x;
		character.character.mTexSrc.top = 1.0f - character.rect.top * invTexSize.y;
		character.character.mTexSrc.bottom = 1.0f - character.rect.bottom * invTexSize.y;

		mTexture->SetSubData(character.rect.LeftBottom(), character.bitmap);

		AddCharacter(character.character);

		character.bitmap = nullptr;
	}

	Vec2I VectorFont::Effect::GetSizeExtend() const
	{
		return Vec2I();
	}

	bool VectorFont::Effect::IsEqual(const Ref<Effect>& other) const
	{
		return GetType() == other->GetType();
	}

}

DECLARE_CLASS(o2::VectorFont::Effect);
