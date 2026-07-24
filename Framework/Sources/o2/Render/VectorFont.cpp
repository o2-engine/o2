#include "o2/stdafx.h"
#include "VectorFont.h"

#if defined PLATFORM_WINDOWS
#include "o2/Render/Windows/OpenGL.h"
#elif defined PLATFORM_ANDROID
#include "o2/Render/Android/OpenGL.h"
#endif

#include "o2/Application/Application.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/System/Time/Timer.h"

namespace o2
{
    // NO_AUTOHINT: FreeType 2.5.5's autofitter calls through mismatched function pointers,
    // which traps on WebAssembly (strict call_indirect signature check)
    static const FT_Int32 kGlyphLoadFlags = FT_LOAD_RENDER | FT_LOAD_NO_AUTOHINT;

    VectorFont::VectorFont() :
        Font(), mFreeTypeFace(nullptr)
    {
        InitializeTexture();
    }

    VectorFont::VectorFont(const String& fileName) :
        Font(), mFreeTypeFace(nullptr)
    {
        InitializeTexture();

        Load(fileName);
    }

    VectorFont::VectorFont(const VectorFont& other) :
        Font(), mFreeTypeFace(other.mFreeTypeFace)
    {
        InitializeTexture();
    }

    void VectorFont::InitializeTexture()
    {
        mTexture = TextureRef(Vec2I(mInitialTextureSize, mInitialTextureSize));
        mTextureSrcRect.Set(0, 0, mInitialTextureSize, mInitialTextureSize);
    }

    VectorFont::~VectorFont()
    {
        if (mFreeTypeFace && mFreeTypeFace->internal)
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

    //     void VectorFont::SetSize(UInt size)
    //     {
    //         if (size == mSize)
    //             return;
    // 
    //         mSize = size;
    //         Vec2I dpi = o2Render.GetDPI();
    //         FT_Error error = FT_Set_Char_Size(mFreeTypeFace, 0, mSize*64, dpi.x, dpi.y);
    // 
    //         FT_Load_Char(mFreeTypeFace, 'A', FT_LOAD_RENDER);
    //         mBaseHeight = mFreeTypeFace->glyph->metrics.horiBearingY/64.0f;
    //         mLineHeight = mBaseHeight*2.0f;
    // 
    // //         mBaseHeight = mFreeTypeFace->ascender/64.0f;
    // //         mLineHeight = (mFreeTypeFace->ascender - mFreeTypeFace->descender)/64.0f + 5.0f;
    // 
    //         Reset();
    //     }

    String VectorFont::GetFileName() const
    {
        return mFileName;
    }

    float VectorFont::GetHeightPx(int height) const
    {
        float result = 0;
        if (mHeights.TryGetValue(height, result))
            return result;

        FT_Set_Char_Size(mFreeTypeFace, 0, height*64, mResolution, mResolution);

        FT_Load_Char(mFreeTypeFace, 'A', kGlyphLoadFlags);

        result = mFreeTypeFace->glyph->metrics.horiBearingY/64.0f;
        mHeights.Add(height, result);

        return result;
    }

    float VectorFont::GetLineHeightPx(int height) const
    {
        return GetHeightPx(height)*2.0f;
    }

    void VectorFont::CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style)
    {
        int styleId = GetStyleId(style);
        UInt64 charactersKey = GetStyleHeightKey(styleId, height);

        int len = needChararacters.Length();
        Vector<wchar_t> needToRenderChars;
        needToRenderChars.Reserve(len);

        auto fndStyleHeight = mCharacters.find(charactersKey);
        for (int i = 0; i < len; i++)
        {
            bool isNew = true;
            wchar_t c = needChararacters[i];
            if (fndStyleHeight != mCharacters.End())
                isNew = fndStyleHeight->second.find(c) == fndStyleHeight->second.End();

            if (isNew)
                isNew = !needToRenderChars.Contains(c);

            if (!isNew)
                continue;

            needToRenderChars.Add(c);
        }

        if (needToRenderChars.Count() > 0)
        {
            static const Vector<Ref<Effect>> emptyEffects;
            RenderNewCharacters(needToRenderChars, height, styleId, styleId != 0 ? style->GetEffects() : emptyEffects);
        }
    }

    int VectorFont::GetStyleId(const Ref<FontStyle>& style)
    {
        if (!style || style->GetEffects().IsEmpty())
            return 0;

        UInt64 cacheKey = style->GetCacheKey();

        int id = 0;
        if (mStyleIds.TryGetValue(cacheKey, id))
            return id;

        id = mNextStyleId++;
        mStyleIds.Add(cacheKey, id);

        return id;
    }

    void VectorFont::Reset()
    {
        mCharacters.Clear();
        onCharactersRebuilt();
    }

    void VectorFont::RenderNewCharacters(Vector<wchar_t>& newCharacters, int height, int styleId,
                                         const Vector<Ref<Effect>>& effects)
    {
        if (!mFreeTypeFace)
            return;

        FT_Set_Char_Size(mFreeTypeFace, 0, height * 64, mResolution, mResolution);

        Vec2I border;
        for (auto& effect : effects)
        {
            Vec2I effectExt = effect->GetSizeExtend();
            border.x = Math::Max(border.x, effectExt.x);
            border.y = Math::Max(border.y, effectExt.y);
        }

        border += Vec2I(2, 2);

        FT_Load_Char(mFreeTypeFace, 'A', kGlyphLoadFlags);
        int symbolsHeight = Math::CeilToInt((mFreeTypeFace->glyph->bitmap.rows + border.y*2)*1.25f);

        for (auto& ch : newCharacters)
        {
            CharDef newCharDef;

            FT_Load_Char(mFreeTypeFace, ch, kGlyphLoadFlags);
            auto glyph = mFreeTypeFace->glyph;

            Vec2I glyphSize(glyph->bitmap.width, glyph->bitmap.rows);

            Ref<Bitmap> newBitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, glyphSize + border*2);
            newBitmap->Fill(Color4(255, 255, 255, 0));
            UInt8* newBitmapData = newBitmap->GetData();
            Vec2I newBitmapSize = newBitmap->GetSize();

            for (int x = 0; x < (int)glyph->bitmap.width; x++)
            {
                for (int y = 0; y < (int)glyph->bitmap.rows; y++)
                {
                    Color4 c(255, 255, 255, glyph->bitmap.buffer[y*glyph->bitmap.width + x]);
                    ULong cl = c.ABGR();
                    memcpy(&newBitmapData[((newBitmapSize.y - y - 1 - border.y)*newBitmapSize.x + x + border.x)*4], &cl, 4);
                }
            }

            for (auto& effect : effects)
                effect->Process(*newBitmap);

            newCharDef.bitmap = newBitmap;
            newCharDef.character.mId = ch;
            newCharDef.character.mHeight = height;
            newCharDef.character.mStyleId = styleId;
            newCharDef.character.mSize = newBitmapSize;
            newCharDef.character.mAdvance = glyph->advance.x/64.0f;
            newCharDef.character.mOrigin.x = -glyph->metrics.horiBearingX/64.0f + border.x;
            newCharDef.character.mOrigin.y = (glyph->metrics.height - glyph->metrics.horiBearingY)/64.0f + border.y;

            PackCharacter(newCharDef, symbolsHeight);

            newCharDef.bitmap = nullptr;
        }
    }

    void VectorFont::PackCharacter(CharDef& character, int height)
    {
        Ref<PackLine> packLine;

        while (!packLine)
        {
            packLine = mPackLines.FindOrDefault([&](const Ref<PackLine>& x) {
                return x->height >= height && x->length + character.bitmap->GetSize().x < mTexture->GetSize().x; });

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
                    mTexture = TextureRef(lastTexture->GetSize()*2, TextureFormat::R8G8B8A8, Texture::Usage::Default);
                    mTexture->Copy(*lastTexture.Get(), RectI(Vec2I(0, 0), lastTexture->GetSize()));

                    for (auto& heightKV : mCharacters)
                    {
                        for (auto& charKV : heightKV.second)
                        {
                            // Pixel rows keep their positions from the texture top, and the
                            // v axis is inverted (v = 1 - row/height): v' = 0.5 + v*0.5
                            charKV.second.mTexSrc.left *= 0.5f;
                            charKV.second.mTexSrc.right *= 0.5f;
                            charKV.second.mTexSrc.top = charKV.second.mTexSrc.top*0.5f + 0.5f;
                            charKV.second.mTexSrc.bottom = charKV.second.mTexSrc.bottom*0.5f + 0.5f;
                        }
                    }
                }
            }
        }

        character.packLine = packLine;
        packLine->characters.Add(character);

        character.rect.left = packLine->length;
        character.rect.top = packLine->position + character.bitmap->GetSize().y;
        character.rect.right = packLine->length + character.bitmap->GetSize().x;
        character.rect.bottom = packLine->position;;

        packLine->length += character.bitmap->GetSize().x;

        Vec2F invTexSize(1.0f/mTexture->GetSize().x, 1.0f/mTexture->GetSize().y);
        character.character.mTexSrc.left = character.rect.left*invTexSize.x;
        character.character.mTexSrc.right = character.rect.right*invTexSize.x;
        character.character.mTexSrc.top = 1.0f - character.rect.top*invTexSize.y;
        character.character.mTexSrc.bottom = 1.0f - character.rect.bottom*invTexSize.y;

        mTexture->SetSubData(character.rect.LeftBottom(), *character.bitmap);

        AddCharacter(character.character);
    }
}
// --- META ---

DECLARE_CLASS(o2::VectorFont::Effect, o2__VectorFont__Effect);
// --- END META ---
