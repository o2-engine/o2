#pragma once

#include "o2/Render/Text/SdfGlyphGenerator.h"
#include "o2/Render/Text/SdfTextStyle.h"
#include "o2/Render/Text/VectorFont.h"
#include "o2/Utils/Types/Containers/Pair.h"

namespace o2
{
    class Material;

    // ------------------------------------------------------------------------------------------------------------------
    // Distance field font. Every glyph is turned into a multi-channel signed distance field once - any size, scale and
    // style draw it from there: the effects of a style are layers of the text shader, not pixels of the atlas. The
    // fields come baked by the assets build (the characters of the font asset meta, each field compressed on its own):
    // a glyph is unpacked into the atlas when it is first shown, one that isn't baked is generated then. A style with
    // an effect that has no distance field version is drawn as by VectorFont
    // ------------------------------------------------------------------------------------------------------------------
    class SdfFont: public VectorFont
    {
    public:
        // ---------------------------
        // Distance field of a glyph
        // ---------------------------
        struct Glyph
        {
            UInt32 id = 0;           // Character code
            RectI  atlasRect;        // Pixels of the field in the atlas; bottom is the first row
            RectF  bounds;           // Box of the field in em, from the pen at the baseline, Y up
            RectF  inkBounds;        // Box of the outline in em: the field box without the padding
            float  advance = 0.0f;   // Advance in em
            bool   empty = true;     // No outline, nothing to draw

            bool operator==(const Glyph& other) const { return id == other.id; }
        };

        // ---------------------------------------------------
        // Layers and material of a style the text draws with
        // ---------------------------------------------------
        struct DrawStyle
        {
            SdfTextStyle          style;          // Layers
            Ref<Material>         material;       // Material with the shader parameters of the layers, null without render
            Vector<Ref<Material>> layerMaterials; // Materials drawing one layer each, for the effects over the whole text
            Vec2F                 extent;         // Pixels the layers reach out of the outline

            bool operator==(const DrawStyle& other) const { return false; }
        };

        // ----------------------
        // Statistics of the font
        // ----------------------
        struct Statistics
        {
            int   generatedGlyphs = 0; // Glyphs generated at runtime
            float generationMs = 0.0f; // Time of that
            int   bakedGlyphs = 0;     // Glyphs unpacked from the baked fields
            float bakedLoadMs = 0.0f;  // Time of reading the baked file and unpacking them
            int   atlasGrowths = 0;    // Times the atlas has grown
        };

    public:
        // Default constructor
        SdfFont();

        // Constructor with loading the font file
        SdfFont(const String& fileName, const SdfFontSettings& settings = SdfFontSettings());

        // Loads the font file
        bool Load(const String& fileName);

        // Sets the field settings; drops the glyphs
        void SetSettings(const SdfFontSettings& settings);

        // Returns the field settings
        const SdfFontSettings& GetSettings() const;

        // Puts the fields of the characters the atlas doesn't have yet into it: unpacks the baked ones, generates the
        // others. Returns how many were added
        int WarmUp(const WString& characters);

        // Warms up the characters for drawing with the style: their fields and the style's material
        void WarmUp(const WString& characters, int height, const Ref<FontStyle>& style, float pixelDensity = 1.0f) override;

        // Returns the field of the character in the atlas, null when it isn't there yet
        const Glyph* GetGlyph(UInt32 id) const;

        // Returns count of glyphs in the atlas
        int GetGlyphsCount() const;

        // Returns count of the baked glyphs
        int GetBakedGlyphsCount();

        // Returns the atlas of the fields
        const TextureRef& GetFieldTexture() const;

        // Returns the size of the atlas of the fields
        Vec2I GetFieldTextureSize() const;

        // Returns true when the text of the style is drawn by the fields
        bool IsDistanceFieldStyle(const Ref<FontStyle>& style);

        // Returns the layers and the material of the style
        const DrawStyle& GetDrawStyle(const Ref<FontStyle>& style);

        // Returns the materials of the layers of the style, the front one first: the effects over the whole text draw
        // all the letters by each of them, from the back one
        const Vector<Ref<Material>>& GetLayerMaterials(const Ref<FontStyle>& style);

        // Returns pixels of the em of the text height
        float GetEmPixels(int height) const;

        // Returns base height in pixels for font with size
        float GetHeightPx(int height) const override;

        // Returns line height in pixels for font with size
        float GetLineHeightPx(int height) const override;

        // Returns character by id, of the fields for a distance field style
        const Character& GetCharacter(UInt16 id, int height, const Ref<FontStyle>& style,
                                      float pixelDensity = 1.0f) override;

        using Font::CheckCharacters;

        // Checks characters: puts the fields of the new ones into the atlas, or renders them as VectorFont for a style
        // the fields can't draw
        void CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style,
                             float pixelDensity = 1.0f) override;

        // Returns the statistics
        const Statistics& GetStatistics() const;

        // Returns the materials all the distance field fonts have built and the time of that, in milliseconds
        static Pair<int, float> GetMaterialsStatistics();

        // Releases the materials and the atlas texture before the render goes
        void ReleaseRenderResources() override;

        // Loads the baked fields; false when there are none or they were baked for other settings
        bool LoadBaked(const String& bakedPath);

        // Sets the baked fields to load when the font is first used
        void SetBakedFields(const String& bakedPath);

        // Returns the path of the baked fields of the font file
        static String GetBakedFieldsPath(const String& fontPath);

        // Bakes the fields of the characters of the font file - without render, for the assets build. The stamp is
        // kept in the file to tell later that it is current
        static bool Bake(const String& fontPath, const SdfFontSettings& settings, const WString& characters,
                         const String& bakedPath, const String& stamp = String());

        // Returns the stamp of the source file of baked fields: its content and the settings
        static String GetSourceStamp(const String& fontPath, const SdfFontSettings& settings);

        // Returns the stamp the fields were baked for, empty when there is no such file
        static String ReadBakedStamp(const String& bakedPath);

        // Returns the default characters to bake: printable ASCII, Cyrillic and the common typographic marks
        static WString GetDefaultCharacters();

    protected:
        // Row of the atlas glyphs are put into one after another
        struct Shelf
        {
            int y = 0;      // Bottom row
            int height = 0; // Rows
            int length = 0; // Pixels taken from the left

            bool operator==(const Shelf& other) const { return y == other.y; }
        };

        // Baked field of a glyph: where its compressed pixels are in the baked file
        struct BakedGlyph
        {
            Vec2I  size;            // Field size in pixels, zero for a glyph without an outline
            RectF  bounds;          // Box of the field in em
            float  advance = 0.0f;  // Advance in em
            UInt32 offset = 0;      // Offset of the compressed pixels in the data
            UInt32 packedSize = 0;  // Size of the compressed pixels

            bool operator==(const BakedGlyph& other) const { return offset == other.offset; }
        };

    protected:
        static constexpr int initialAtlasSize = 256; // Atlas side at start, doubles when it is full
        static constexpr int shelfStep = 4;           // Shelf heights are rounded up to this

        SdfFontSettings mSettings; // Field settings

        Map<UInt32, Glyph> mGlyphs;         // Fields in the atlas by character code
        Vector<Shelf>      mShelves;        // Rows of the atlas
        int                mShelvesTop = 0; // Top of the last row
        Vec2I              mAtlasSize;      // Atlas size in pixels
        TextureRef         mFieldTexture;   // Atlas texture, null without render

        String                  mPendingBaked; // Baked fields to load at the first use
        Map<UInt32, BakedGlyph> mBakedGlyphs;  // Baked fields by character code
        Vector<UInt8>           mBakedData;    // Compressed pixels of the baked fields

        float mCapHeight = 0.7f; // Top of 'A' in em, the base height of the text

        Map<int, DrawStyle> mDrawStyles;      // Draw styles by font-local style id
        Map<int, bool>      mSupportedStyles; // Is the style drawn by fields, by font-local style id

        Statistics mStatistics; // Statistics

        int  mNotifyLock = 0;       // Calls in progress that change the atlas: the texts are told when the last ends
        bool mAtlasChanged = false; // The atlas has grown or was created, the texts aren't told yet

    protected:
        // Loads the baked fields set to load at the first use
        void LoadPendingBaked();

        // Puts the field of the character into the atlas: the baked one or a generated one. False when the font has no
        // such glyph
        bool AddGlyph(UInt32 id);

        // Unpacks the baked field of the character; false when there is none
        bool UnpackBaked(UInt32 id, SdfGlyphField& field);

        // Places a field of the size into the atlas, growing it when it is full
        RectI Pack(const Vec2I& size);

        // Grows the atlas, keeps the pixels of the glyphs
        void GrowAtlas();

        // Puts the pixels of the field into the atlas
        void UploadField(const RectI& rect, const SdfGlyphField& field);

        // Updates the characters, the materials and the texts after the atlas has changed, unless a call that changes
        // it is still in progress
        void NotifyAtlasChanged();

        // Returns true when the font-local style id is drawn by the fields
        bool IsDistanceFieldStyleId(int styleId) const;

        // Returns the character of the field glyph for the height and the extent of the style
        Character MakeCharacter(const Glyph& glyph, int height, int styleId, const Vec2F& extent) const;

        // Creates the material of the draw style
        void CreateMaterial(DrawStyle& drawStyle);

        // Creates a material of the layers of the draw style; drawn layer -1 - all of them
        Ref<Material> CreateLayersMaterial(const DrawStyle& drawStyle, int drawnLayer);

        // Updates the atlas parameters of the material of the draw style
        void UpdateMaterialAtlas(const DrawStyle& drawStyle, Material& material) const;
    };
}
