#include "o2/stdafx.h"
#include "SdfFont.h"

#include "3rdPartyLibs/zlib/zlib.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Material.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Timer.h"

#if defined PLATFORM_ANDROID
#include "o2/Render/Android/OpenGL.h"
#endif

namespace o2
{
    namespace
    {
        // Material the draw styles are copied from: the shaders are compiled once for all the fonts
        Ref<Material> gPrototypeMaterial;

        // The shaders didn't compile or link: the fonts draw by the bitmap atlas
        bool gMaterialFailed = false;

        // Materials built and the time of that
        int   gMaterialsCount = 0;
        float gMaterialsMs = 0.0f;

        const char* fieldParamName = "u_field";
        const char* layersParamName = "u_layers";
        const char* passParamName = "u_pass";

        // Start of a baked fields file
        const char bakedMagic[8] = { 'O', '2', 'S', 'D', 'F', 'v', '0', '2' };

        // Header of the GLSL sources: GLSL ES 3 where the context has it, the derivatives are core there
        String GetShadersHeader()
        {
#if defined(PLATFORM_WASM)
            return "#version 300 es\n#define O2_GLSL_ES3\n";
#elif defined(PLATFORM_ANDROID)
            return GLES3::available ? "#version 300 es\n#define O2_GLSL_ES3\n" : "";
#else
            return "";
#endif
        }

        // Places fields into the rows of an atlas
        struct ShelfPacker
        {
            struct Shelf { int y = 0, height = 0, length = 0; };

            Vector<Shelf> shelves;
            int           top = 0;
            Vec2I         size;

            // Returns the place of the field, an empty rect when the atlas is full
            RectI Place(const Vec2I& fieldSize, int step)
            {
                int height = (fieldSize.y + step - 1)/step*step;
                int maxWaste = Math::Max(8, height/4);

                Shelf* best = nullptr;
                for (auto& shelf : shelves)
                {
                    if (shelf.height < fieldSize.y || shelf.height > height + maxWaste || shelf.length + fieldSize.x > size.x)
                        continue;

                    if (!best || shelf.height < best->height)
                        best = &shelf;
                }

                if (!best)
                {
                    if (top + height > size.y || fieldSize.x > size.x)
                        return RectI();

                    Shelf shelf;
                    shelf.y = top;
                    shelf.height = height;
                    shelves.Add(shelf);
                    top += height;
                    best = &shelves.Last();
                }

                RectI rect(best->length, best->y + fieldSize.y, best->length + fieldSize.x, best->y);
                best->length += fieldSize.x;
                return rect;
            }
        };

        void ToGlyph(const SdfGlyphField& field, const RectI& rect, int padding, float glyphSize, SdfFont::Glyph& glyph)
        {
            glyph.id = field.character;
            glyph.advance = field.advance;
            glyph.empty = field.size.x == 0 || field.size.y == 0;
            glyph.atlasRect = rect;
            glyph.bounds = field.bounds;

            float inset = (float)padding/glyphSize;
            glyph.inkBounds = glyph.empty ? RectF() :
                RectF(field.bounds.left + inset, field.bounds.top - inset, field.bounds.right - inset,
                      field.bounds.bottom + inset);
        }

        UInt64 HashBytes(const Vector<UInt8>& data)
        {
            UInt64 hash = 14695981039346656037ull;
            for (UInt8 byte : data)
            {
                hash ^= byte;
                hash *= 1099511628211ull;
            }
            return hash;
        }

        // Reads the whole binary file
        Vector<UInt8> ReadBytes(const String& path)
        {
            Vector<UInt8> data;
            InFile file(path);
            if (!file.IsOpened())
                return data;

            data.resize(file.GetDataSize());
            if (!data.IsEmpty())
                file.ReadFullData(data.Data());

            return data;
        }

        // Little-endian writer of a baked fields file
        struct BinaryWriter
        {
            Vector<UInt8> data;

            void Bytes(const void* bytes, size_t count)
            {
                size_t at = data.Count();
                data.resize(at + count);
                memcpy(data.Data() + at, bytes, count);
            }

            void U32(UInt32 value) { Bytes(&value, 4); }
            void U16(UInt16 value) { Bytes(&value, 2); }
            void F32(float value) { Bytes(&value, 4); }
            void Str(const String& value) { U32((UInt32)value.Length()); Bytes(value.Data(), value.Length()); }
        };

        // Reader of a baked fields file; fails on a read past the end
        struct BinaryReader
        {
            const Vector<UInt8>& data;
            size_t               position = 0;
            bool                 failed = false;

            BinaryReader(const Vector<UInt8>& data): data(data) {}

            bool Bytes(void* bytes, size_t count)
            {
                if (failed || position + count > data.Count())
                {
                    failed = true;
                    return false;
                }

                memcpy(bytes, data.Data() + position, count);
                position += count;
                return true;
            }

            UInt32 U32() { UInt32 value = 0; Bytes(&value, 4); return value; }
            UInt16 U16() { UInt16 value = 0; Bytes(&value, 2); return value; }
            float F32() { float value = 0; Bytes(&value, 4); return value; }

            String Str()
            {
                UInt32 length = U32();
                if (failed || position + length > data.Count())
                {
                    failed = true;
                    return String();
                }

                std::string value((const char*)data.Data() + position, length);
                position += length;
                return String(value.c_str());
            }
        };

        // The fields are smooth: a pixel is stored as its difference from the plane through the left, upper and upper
        // left neighbours (as the PNG filters do), which compresses several times better
        void PredictPixels(Vector<UInt8>& pixels, const Vec2I& size, bool encode)
        {
            auto at = [&](int x, int y, int c) -> int { return x < 0 || y < 0 ? 0 : pixels[(y*size.x + x)*4 + c]; };

            // Encoding goes from the end, so every prediction reads the original neighbours; decoding from the start,
            // so it reads the restored ones
            int count = size.x*size.y;
            for (int k = 0; k < count; k++)
            {
                int index = encode ? count - 1 - k : k;
                int x = index%size.x, y = index/size.x;
                for (int c = 0; c < 4; c++)
                {
                    int prediction = at(x - 1, y, c) + at(x, y - 1, c) - at(x - 1, y - 1, c);
                    UInt8& value = pixels[index*4 + c];
                    value = (UInt8)(encode ? value - prediction : value + prediction);
                }
            }
        }

        // Reads the header of a baked fields file: the stamp and the settings
        bool ReadBakedHeader(BinaryReader& reader, String& stamp, String& settings)
        {
            char magic[8];
            if (!reader.Bytes(magic, 8) || memcmp(magic, bakedMagic, 8) != 0)
                return false;

            stamp = reader.Str();
            settings = reader.Str();
            return !reader.failed;
        }
    }

    SdfFont::SdfFont():
        VectorFont(false)
    {}

    SdfFont::SdfFont(const String& fileName, const SdfFontSettings& settings /*= SdfFontSettings()*/):
        VectorFont(false), mSettings(settings)
    {
        Load(fileName);
    }

    bool SdfFont::Load(const String& fileName)
    {
        if (!VectorFont::Load(fileName))
            return false;

        mCapHeight = SdfGlyphGenerator::GetCapHeight(mFreeTypeFace);
        return true;
    }

    void SdfFont::SetSettings(const SdfFontSettings& settings)
    {
        if (settings == mSettings)
            return;

        mSettings = settings;
        mGlyphs.Clear();
        mShelves.Clear();
        mShelvesTop = 0;
        mAtlasSize = Vec2I();
        mFieldTexture = TextureRef();
        mBakedGlyphs.Clear();
        mBakedData.Clear();
        mDrawStyles.Clear();
        Reset();
    }

    const SdfFontSettings& SdfFont::GetSettings() const
    {
        return mSettings;
    }

    int SdfFont::WarmUp(const WString& characters)
    {
        PROFILE_SAMPLE("Font: warm up distance fields");

        LoadPendingBaked();
        mNotifyLock++;

        int added = 0;
        for (int i = 0; i < characters.Length(); i++)
        {
            UInt32 id = (UInt32)characters[i];
            if (mGlyphs.ContainsKey(id) || id == '\n')
                continue;

            if (AddGlyph(id))
                added++;
        }

        mNotifyLock--;
        NotifyAtlasChanged();

        return added;
    }

    void SdfFont::WarmUp(const WString& characters, int height, const Ref<FontStyle>& style, float pixelDensity /*= 1.0f*/)
    {
        if (!IsDistanceFieldStyle(style))
        {
            VectorFont::WarmUp(characters, height, style, pixelDensity);
            return;
        }

        WarmUp(characters);
        GetDrawStyle(style);

        if (style && style->GetEffectsMode() == FontEffectsMode::WholeText)
            GetLayerMaterials(style);

        if (height > 0)
            CheckCharacters(characters, height, style, pixelDensity);
    }

    const SdfFont::Glyph* SdfFont::GetGlyph(UInt32 id) const
    {
        auto found = mGlyphs.find(id);
        return found != mGlyphs.end() ? &found->second : nullptr;
    }

    int SdfFont::GetGlyphsCount() const
    {
        return mGlyphs.Count();
    }

    int SdfFont::GetBakedGlyphsCount()
    {
        LoadPendingBaked();
        return mBakedGlyphs.Count();
    }

    const TextureRef& SdfFont::GetFieldTexture() const
    {
        return mFieldTexture;
    }

    Vec2I SdfFont::GetFieldTextureSize() const
    {
        return mAtlasSize;
    }

    bool SdfFont::IsDistanceFieldStyle(const Ref<FontStyle>& style)
    {
        if (gMaterialFailed)
            return false;

        int styleId = GetStyleId(style);
        if (styleId == 0)
            return true;

        bool supported = false;
        if (mSupportedStyles.TryGetValue(styleId, supported))
            return supported;

        supported = SdfTextStyle::Build(style).IsSupported();
        mSupportedStyles.Add(styleId, supported);

        if (!supported)
            o2Debug.LogWarning("A font style has an effect without the distance field version, its text is rendered "
                               "into the bitmap atlas: " + GetFileName());

        return supported;
    }

    const SdfFont::DrawStyle& SdfFont::GetDrawStyle(const Ref<FontStyle>& style)
    {
        int styleId = GetStyleId(style);
        auto found = mDrawStyles.find(styleId);
        if (found != mDrawStyles.end())
        {
            if (!found->second.material)
                CreateMaterial(found->second);

            return found->second;
        }

        DrawStyle& drawStyle = mDrawStyles[styleId];
        drawStyle.style = styleId != 0 ? SdfTextStyle::Build(style) : SdfTextStyle();
        drawStyle.extent = drawStyle.style.GetExtent();

        CreateMaterial(drawStyle);
        return drawStyle;
    }

    float SdfFont::GetEmPixels(int height) const
    {
        return (float)height*(float)mResolution/72.0f;
    }

    float SdfFont::GetHeightPx(int height) const
    {
        // Whole pixels as the hinted vector font has them: the layout puts lines at whole pixels
        return Math::Round(mCapHeight*GetEmPixels(height));
    }

    float SdfFont::GetLineHeightPx(int height) const
    {
        return GetHeightPx(height)*2.0f;
    }

    const Font::Character& SdfFont::GetCharacter(UInt16 id, int height, const Ref<FontStyle>& style,
                                                 float pixelDensity /*= 1.0f*/)
    {
        if (!IsDistanceFieldStyle(style))
            return VectorFont::GetCharacter(id, height, style, pixelDensity);

        auto found = mCharacters.find(GetStyleHeightKey(GetStyleId(style), height, 1.0f));
        if (found != mCharacters.End())
        {
            auto foundCharacter = found->second.find(id);
            if (foundCharacter != found->second.End())
                return foundCharacter->second;
        }

        static Character empty;
        return empty;
    }

    void SdfFont::CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style,
                                  float pixelDensity /*= 1.0f*/)
    {
        if (!IsDistanceFieldStyle(style))
        {
            VectorFont::CheckCharacters(needChararacters, height, style, pixelDensity);
            return;
        }

        LoadPendingBaked();
        mNotifyLock++;

        int styleId = GetStyleId(style);
        const DrawStyle& drawStyle = GetDrawStyle(style);
        Vec2F extent = drawStyle.extent;

        auto& characters = mCharacters[GetStyleHeightKey(styleId, height, 1.0f)];
        for (int i = 0; i < needChararacters.Length(); i++)
        {
            UInt16 id = (UInt16)needChararacters[i];
            if (characters.find(id) != characters.end())
                continue;

            const Glyph* glyph = GetGlyph(id);
            if (!glyph && AddGlyph(id))
                glyph = GetGlyph(id);

            if (glyph)
                characters[id] = MakeCharacter(*glyph, height, styleId, extent);
            else
            {
                Character missing;
                missing.mId = id;
                missing.mHeight = height;
                missing.mStyleId = styleId;
                characters[id] = missing;
            }
        }

        mNotifyLock--;
        NotifyAtlasChanged();
    }

    const SdfFont::Statistics& SdfFont::GetStatistics() const
    {
        return mStatistics;
    }

    Font::Character SdfFont::MakeCharacter(const Glyph& glyph, int height, int styleId, const Vec2F& extent) const
    {
        Character character;
        character.mId = (UInt16)glyph.id;
        character.mHeight = height;
        character.mStyleId = styleId;
        character.mPixelDensity = 1.0f;

        float em = GetEmPixels(height);
        character.mAdvance = glyph.advance*em;

        if (glyph.empty)
            return character;

        RectF quad(glyph.inkBounds.left*em - extent.x, glyph.inkBounds.top*em + extent.y,
                   glyph.inkBounds.right*em + extent.x, glyph.inkBounds.bottom*em - extent.y);

        character.mSize = quad.Size();
        character.mOrigin = Vec2F(-quad.left, -quad.bottom);

        // The texture rect has the convention of the bitmap glyphs: top and bottom count from the top of the atlas
        if (mAtlasSize.x > 0 && mAtlasSize.y > 0)
        {
            float texelsPerPixel = (float)mSettings.glyphSize/em;
            float x0 = glyph.atlasRect.left + (quad.left/em - glyph.bounds.left)*mSettings.glyphSize;
            float x1 = x0 + quad.Width()*texelsPerPixel;
            float y0 = glyph.atlasRect.bottom + (quad.bottom/em - glyph.bounds.bottom)*mSettings.glyphSize;
            float y1 = y0 + quad.Height()*texelsPerPixel;

            character.mTexSrc.left = x0/(float)mAtlasSize.x;
            character.mTexSrc.right = x1/(float)mAtlasSize.x;
            character.mTexSrc.top = 1.0f - y1/(float)mAtlasSize.y;
            character.mTexSrc.bottom = 1.0f - y0/(float)mAtlasSize.y;
        }

        return character;
    }

    bool SdfFont::AddGlyph(UInt32 id)
    {
        Timer timer;
        SdfGlyphField field;
        bool baked = UnpackBaked(id, field);
        if (!baked)
        {
            if (!mFreeTypeFace || !SdfGlyphGenerator::Generate(mFreeTypeFace, id, mSettings, field))
                return false;
        }

        RectI rect;
        if (field.size.x > 0 && field.size.y > 0)
        {
            rect = Pack(field.size);
            UploadField(rect, field);
        }

        Glyph glyph;
        ToGlyph(field, rect, mSettings.GetPadding(), (float)mSettings.glyphSize, glyph);
        mGlyphs[id] = glyph;

        float ms = timer.GetDeltaTime()*1000.0f;
        if (baked)
        {
            mStatistics.bakedGlyphs++;
            mStatistics.bakedLoadMs += ms;
        }
        else
        {
            mStatistics.generatedGlyphs++;
            mStatistics.generationMs += ms;
        }

        return true;
    }

    bool SdfFont::UnpackBaked(UInt32 id, SdfGlyphField& field)
    {
        auto found = mBakedGlyphs.find(id);
        if (found == mBakedGlyphs.end())
            return false;

        PROFILE_SAMPLE("Font: unpack baked distance field");

        const BakedGlyph& baked = found->second;
        field = SdfGlyphField();
        field.character = id;
        field.size = baked.size;
        field.bounds = baked.bounds;
        field.advance = baked.advance;

        if (baked.size.x == 0 || baked.size.y == 0)
            return true;

        uLongf size = (uLongf)(baked.size.x*baked.size.y*4);
        field.pixels.resize((int)size);
        if (baked.offset + baked.packedSize > (UInt32)mBakedData.Count() ||
            uncompress(field.pixels.Data(), &size, mBakedData.Data() + baked.offset, baked.packedSize) != Z_OK ||
            size != (uLongf)field.pixels.Count())
        {
            o2Debug.LogWarning("Broken baked distance field of character " + (String)(int)id + " in font " + GetFileName());
            mBakedGlyphs.Remove(id);
            return false;
        }

        PredictPixels(field.pixels, field.size, false);
        return true;
    }

    RectI SdfFont::Pack(const Vec2I& size)
    {
        if (mAtlasSize == Vec2I())
        {
            int side = initialAtlasSize;
            while (side < size.x || side < size.y)
                side *= 2;

            mAtlasSize = Vec2I(side, side);
            mAtlasChanged = true;
            if (Render::IsSingletonInitialzed())
            {
                Bitmap clear(PixelFormat::R8G8B8A8, mAtlasSize);
                clear.Fill(Color4(0, 0, 0, 0));
                mFieldTexture = TextureRef(clear);
                mFieldTexture->SetName("Distance field font atlas " + GetFileName());
            }
        }

        while (true)
        {
            ShelfPacker packer;
            packer.size = mAtlasSize;
            packer.top = mShelvesTop;
            for (auto& shelf : mShelves)
                packer.shelves.Add({ shelf.y, shelf.height, shelf.length });

            RectI rect = packer.Place(size, shelfStep);
            if (rect != RectI())
            {
                mShelves.Clear();
                for (auto& shelf : packer.shelves)
                {
                    Shelf s;
                    s.y = shelf.y;
                    s.height = shelf.height;
                    s.length = shelf.length;
                    mShelves.Add(s);
                }

                mShelvesTop = packer.top;
                return rect;
            }

            GrowAtlas();
        }
    }

    void SdfFont::GrowAtlas()
    {
        PROFILE_SAMPLE("Font: grow the distance field atlas");

        // Twice the height first: the shelves keep their places, and a tall atlas grows in width next
        Vec2I oldSize = mAtlasSize;
        mAtlasSize = oldSize.y <= oldSize.x ? Vec2I(oldSize.x, oldSize.y*2) : Vec2I(oldSize.x*2, oldSize.y);
        mStatistics.atlasGrowths++;

        if (mFieldTexture)
        {
            TextureRef oldTexture = mFieldTexture;
            Bitmap clear(PixelFormat::R8G8B8A8, mAtlasSize);
            clear.Fill(Color4(0, 0, 0, 0));
            mFieldTexture = TextureRef(clear);
            mFieldTexture->SetName(oldTexture->GetName());
            mFieldTexture->Copy(*oldTexture.Get(), RectI(Vec2I(0, 0), oldSize));
        }

        mAtlasChanged = true;
    }

    void SdfFont::UploadField(const RectI& rect, const SdfGlyphField& field)
    {
        if (!mFieldTexture)
            return;

        Bitmap bitmap(PixelFormat::R8G8B8A8, field.size);
        memcpy(bitmap.GetData(), field.pixels.Data(), field.pixels.Count());
        mFieldTexture->SetSubData(rect.LeftBottom(), bitmap);
    }

    void SdfFont::NotifyAtlasChanged()
    {
        if (mNotifyLock > 0 || !mAtlasChanged)
            return;

        mAtlasChanged = false;

        // The characters have the texture rects of the old atlas size: they are made anew, and the texts rebuilt
        for (auto& heightKV : mCharacters)
        {
            int styleId = (int)(heightKV.first >> 32);
            if (!IsDistanceFieldStyleId(styleId))
                continue;

            auto drawStyle = mDrawStyles.find(styleId);
            Vec2F extent = drawStyle != mDrawStyles.end() ? drawStyle->second.extent : Vec2F();
            for (auto& charKV : heightKV.second)
            {
                if (const Glyph* glyph = GetGlyph(charKV.first))
                    charKV.second = MakeCharacter(*glyph, charKV.second.mHeight, styleId, extent);
            }
        }

        for (auto& kv : mDrawStyles)
        {
            if (kv.second.material)
                UpdateMaterialAtlas(kv.second, *kv.second.material);

            for (auto& material : kv.second.layerMaterials)
                UpdateMaterialAtlas(kv.second, *material);
        }

        onCharactersRebuilt();
    }

    bool SdfFont::IsDistanceFieldStyleId(int styleId) const
    {
        if (styleId == 0)
            return true;

        bool supported = false;
        return mSupportedStyles.TryGetValue(styleId, supported) && supported;
    }

    void SdfFont::CreateMaterial(DrawStyle& drawStyle)
    {
        drawStyle.material = CreateLayersMaterial(drawStyle, -1);
    }

    const Vector<Ref<Material>>& SdfFont::GetLayerMaterials(const Ref<FontStyle>& style)
    {
        int styleId = GetStyleId(style);
        GetDrawStyle(style);
        DrawStyle& drawStyle = mDrawStyles[styleId];

        int layersCount = Math::Min(drawStyle.style.GetLayers().Count(), SdfTextStyle::maxLayers);
        if (drawStyle.layerMaterials.Count() != layersCount && drawStyle.material)
        {
            drawStyle.layerMaterials.Clear();
            for (int i = 0; i < layersCount; i++)
            {
                if (auto material = CreateLayersMaterial(drawStyle, i))
                    drawStyle.layerMaterials.Add(material);
            }
        }

        return drawStyle.layerMaterials;
    }

    Ref<Material> SdfFont::CreateLayersMaterial(const DrawStyle& drawStyle, int drawnLayer)
    {
        if (!Render::IsSingletonInitialzed())
            return nullptr;

        Timer timer;

        if (!gPrototypeMaterial && !gMaterialFailed)
        {
            PROFILE_SAMPLE("Font: distance field material");

            gPrototypeMaterial = Material::CreateFromBuiltinShaders("SdfText", GetShadersHeader());
            if (!gPrototypeMaterial)
            {
                gMaterialFailed = true;
                o2Debug.LogError("The distance field text shaders don't compile: the texts of distance field fonts are "
                                 "rendered into the bitmap atlas");
                return nullptr;
            }

            // The vertices carry the gradient coordinates, the field cell and the scale: the extra coordinate sets are
            // bound by the sampler attributes, the samplers have no textures
            TextureSampler gradientCoordinates;
            gradientCoordinates.samplerUniformName = "u_unusedSampler2";
            gradientCoordinates.texCoordsAttrName = "a_texCoords2";
            gPrototypeMaterial->AddTextureSampler(gradientCoordinates);

            TextureSampler cellCoordinates;
            cellCoordinates.samplerUniformName = "u_unusedSampler3";
            cellCoordinates.texCoordsAttrName = "a_texCoords3";
            gPrototypeMaterial->AddTextureSampler(cellCoordinates);

            Vector<float> layers;
            layers.resize(SdfTextStyle::maxLayers*SdfTextStyle::vectorsPerLayer*4, 0.0f);
            gPrototypeMaterial->AddParam(mmake<ShaderParamFloatVector>(fieldParamName, Vector<float>({ 1, 16, 256, 256 })));
            gPrototypeMaterial->AddParam(mmake<ShaderParamFloatVector>(layersParamName, layers));
            gPrototypeMaterial->AddParam(mmake<ShaderParamFloatVector>(passParamName, Vector<float>({ -1, 0, 0, 0 })));

            if (!gPrototypeMaterial->Build())
            {
                gMaterialFailed = true;
                o2Debug.LogError("Failed to build the distance field text material: the texts of distance field fonts "
                                 "are rendered into the bitmap atlas");
                gPrototypeMaterial = nullptr;
                return nullptr;
            }
        }

        if (!gPrototypeMaterial)
            return nullptr;

        auto material = mmake<Material>(*gPrototypeMaterial);

        Vector<float> layers;
        drawStyle.style.Pack(layers);
        if (auto param = DynamicCast<ShaderParamFloatVector>(material->GetShaderParam(layersParamName)))
            param->SetValue(layers);

        if (auto param = DynamicCast<ShaderParamFloatVector>(material->GetShaderParam(passParamName)))
            param->SetValue({ (float)drawnLayer, 0.0f, 0.0f, 0.0f });

        UpdateMaterialAtlas(drawStyle, *material);

        gMaterialsCount++;
        gMaterialsMs += timer.GetDeltaTime()*1000.0f;

        return material;
    }

    Pair<int, float> SdfFont::GetMaterialsStatistics()
    {
        return { gMaterialsCount, gMaterialsMs };
    }

    void SdfFont::UpdateMaterialAtlas(const DrawStyle& drawStyle, Material& material) const
    {
        int layersCount = Math::Min(drawStyle.style.GetLayers().Count(), SdfTextStyle::maxLayers);
        Vec2I size = mAtlasSize == Vec2I() ? Vec2I(1, 1) : mAtlasSize;
        if (auto param = DynamicCast<ShaderParamFloatVector>(material.GetShaderParam(fieldParamName)))
            param->SetValue({ (float)layersCount, mSettings.distanceRange, (float)size.x, (float)size.y });

        material.InvalidateHash();
    }


    void SdfFont::ReleaseRenderResources()
    {
        for (auto& kv : mDrawStyles)
        {
            kv.second.material = nullptr;
            kv.second.layerMaterials.Clear();
        }

        mFieldTexture = TextureRef();
        gPrototypeMaterial = nullptr;

        VectorFont::ReleaseRenderResources();
    }

    String SdfFont::GetBakedFieldsPath(const String& fontPath)
    {
        return fontPath + ".sdf";
    }

    String SdfFont::GetSourceStamp(const String& fontPath, const SdfFontSettings& settings)
    {
        return String::Format("%llx", (unsigned long long)HashBytes(ReadBytes(fontPath))) + "/" + settings.ToString();
    }

    String SdfFont::ReadBakedStamp(const String& bakedPath)
    {
        if (!o2FileSystem.IsFileExist(bakedPath))
            return String();

        Vector<UInt8> data = ReadBytes(bakedPath);
        BinaryReader reader(data);
        String stamp, settings;
        return ReadBakedHeader(reader, stamp, settings) ? stamp : String();
    }

    void SdfFont::SetBakedFields(const String& bakedPath)
    {
        mPendingBaked = bakedPath;
    }

    void SdfFont::LoadPendingBaked()
    {
        if (mPendingBaked.IsEmpty())
            return;

        String path = mPendingBaked;
        mPendingBaked.Clear();
        LoadBaked(path);
    }

    bool SdfFont::LoadBaked(const String& bakedPath)
    {
        PROFILE_SAMPLE("Font: load baked distance fields");
        Timer timer;

        mPendingBaked.Clear();
        if (!o2FileSystem.IsFileExist(bakedPath))
            return false;

        Vector<UInt8> data = ReadBytes(bakedPath);
        BinaryReader reader(data);

        String stamp, settings;
        if (!ReadBakedHeader(reader, stamp, settings) || settings != mSettings.ToString())
            return false;

        float capHeight = reader.F32();
        UInt32 count = reader.U32();

        Map<UInt32, BakedGlyph> glyphs;
        for (UInt32 i = 0; i < count && !reader.failed; i++)
        {
            UInt32 id = reader.U32();
            BakedGlyph glyph;
            glyph.size.x = reader.U16();
            glyph.size.y = reader.U16();
            float left = reader.F32(), bottom = reader.F32(), right = reader.F32(), top = reader.F32();
            glyph.bounds = RectF(left, top, right, bottom);
            glyph.advance = reader.F32();
            glyph.offset = reader.U32();
            glyph.packedSize = reader.U32();
            glyphs[id] = glyph;
        }

        if (reader.failed)
            return false;

        // The offsets count from the end of the table
        UInt32 dataStart = (UInt32)reader.position;
        for (auto& kv : glyphs)
            kv.second.offset += dataStart;

        mCapHeight = capHeight;
        mBakedGlyphs = glyphs;
        mBakedData = std::move(data);
        mStatistics.bakedLoadMs += timer.GetDeltaTime()*1000.0f;

        return true;
    }

    bool SdfFont::Bake(const String& fontPath, const SdfFontSettings& settings, const WString& characters,
                       const String& bakedPath, const String& stamp /*= String()*/)
    {
        PROFILE_SAMPLE("Font: bake distance fields");

        Vector<UInt8> fontData = ReadBytes(fontPath);
        if (fontData.IsEmpty())
            return false;

        FT_Library library = nullptr;
        if (FT_Init_FreeType(&library) != 0)
            return false;

        FT_Face face = nullptr;
        if (FT_New_Memory_Face(library, (const FT_Byte*)fontData.Data(), (FT_Long)fontData.Count(), 0, &face) != 0)
        {
            FT_Done_FreeType(library);
            return false;
        }

        // Every field is compressed on its own: the font unpacks only the characters it shows
        BinaryWriter table, blob;
        UInt32 count = 0;
        Vector<UInt32> done;
        for (int i = 0; i < characters.Length(); i++)
        {
            UInt32 id = (UInt32)characters[i];
            if (done.Contains(id))
                continue;

            done.Add(id);

            SdfGlyphField field;
            if (!SdfGlyphGenerator::Generate(face, id, settings, field))
                continue;

            Vector<UInt8> packed;
            if (!field.pixels.IsEmpty())
            {
                PredictPixels(field.pixels, field.size, true);

                uLongf packedSize = compressBound((uLong)field.pixels.Count());
                packed.resize((int)packedSize);
                if (compress2(packed.Data(), &packedSize, field.pixels.Data(), (uLong)field.pixels.Count(), 9) != Z_OK)
                    continue;

                packed.resize((int)packedSize);
            }

            table.U32(id);
            table.U16((UInt16)field.size.x);
            table.U16((UInt16)field.size.y);
            table.F32(field.bounds.left);
            table.F32(field.bounds.bottom);
            table.F32(field.bounds.right);
            table.F32(field.bounds.top);
            table.F32(field.advance);
            table.U32((UInt32)blob.data.Count());
            table.U32((UInt32)packed.Count());
            if (!packed.IsEmpty())
                blob.Bytes(packed.Data(), packed.Count());

            count++;
        }

        BinaryWriter file;
        file.Bytes(bakedMagic, 8);
        file.Str(stamp.IsEmpty() ? GetSourceStamp(fontPath, settings) : stamp);
        file.Str(settings.ToString());
        file.F32(SdfGlyphGenerator::GetCapHeight(face));
        file.U32(count);
        file.Bytes(table.data.Data(), table.data.Count());
        file.Bytes(blob.data.Data(), blob.data.Count());

        FT_Done_Face(face);
        FT_Done_FreeType(library);

        OutFile out(bakedPath);
        if (!out.IsOpened())
            return false;

        out.WriteData(file.data.Data(), (UInt)file.data.Count());
        return true;
    }

    WString SdfFont::GetDefaultCharacters()
    {
        WString characters;
        for (wchar_t c = 32; c < 127; c++)
            characters += c;

        for (wchar_t c = 0x410; c <= 0x44F; c++)
            characters += c;

        characters += (wchar_t)0x401;
        characters += (wchar_t)0x451;

        const wchar_t marks[] = { 0x2014, 0x2013, 0x2026, 0x00AB, 0x00BB, 0x201C, 0x201D, 0x2018, 0x2019, 0x2022,
                                  0x00B7, 0x00D7, 0x00A9, 0x00AE, 0x2122, 0x20BD, 0x20AC, 0x00B0, 0x2116, 0 };
        for (int i = 0; marks[i]; i++)
            characters += marks[i];

        return characters;
    }
}
