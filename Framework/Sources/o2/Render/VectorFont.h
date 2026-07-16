#pragma once

#include "ft2build.h"
#include FT_FREETYPE_H

#include "o2/Render/Font.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Tools/RectPacker.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class RectsPacker;
    class Bitmap;

    // -----------
    // Vector font
    // -----------
    class VectorFont: public Font
    {
    public:
        // ---------------------
        // Font effect interface
        // ---------------------
        class Effect: public ISerializable, public RefCounterable, public ICloneableRef
        {
        public:
            // Processes glyph bitmap
            virtual void Process(Bitmap& bitmap) {};

            // Returns needs extending size for glyph bitmap
            virtual Vec2I GetSizeExtend() const { return Vec2I(); };

            // Check effect equals
            virtual bool IsEqual(Effect* other) const { return GetType() == other->GetType(); }

            SERIALIZABLE(Effect);
            CLONEABLE_REF(Effect);
        };

    public:
        // Default constructor
        VectorFont();

        // Constructor with loading file
        VectorFont(const String& fileName);

        // Copy-constructor
        VectorFont(const VectorFont& other);

        // Destructor
        ~VectorFont();

        // Loads font
        bool Load(const String& fileName);

        // Returns font file name
        String GetFileName() const;

        // Returns base height in pixels for font with size
        float GetHeightPx(int height) const;

        // Returns line height in pixels for font with size
        float GetLineHeightPx(int height) const;

        using Font::CheckCharacters;

        // Checks characters for preloading, rendered with style
        void CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style) override;

        // Removes all cached characters
        void Reset();

    protected:
        struct PackLine;

        //-------------------------------
        // Font glyph rendering character
        // ------------------------------
        struct CharDef
        {
            WeakRef<PackLine> packLine;

            RectI       rect;
            Character   character;
            Ref<Bitmap> bitmap;

            bool operator==(const CharDef& other) const { return false; }
        };

        // -----------------------
        // Characters packing line
        // -----------------------
        struct PackLine: public RefCounterable
        {
            int position = 0;
            int height = 0;
            int length = 0;

            Vector<CharDef> characters;

        public:
            bool operator==(const PackLine& other) const { return false; }
        };

    protected:
        static const int mInitialTextureSize = 1024;
        static const int mResolution = 92;

        String  mFileName;     // Source file name
        FT_Face mFreeTypeFace; // Free Type font face

        Vector<Ref<PackLine>> mPackLines;           // Packed symbols lines
        int                   mLastPackLinePos = 0; // Last packed line bottom pos

        Map<UInt64, int> mStyleIds;        // Font-local style ids by style content cache key
        int              mNextStyleId = 1; // Next free style id, 0 is reserved for empty style

        mutable Map<int, float> mHeights; // Cached line heights

    protected:
        // Initializes glyphs texture
        void InitializeTexture();

        // Returns font-local style id, registers new styles by content cache key
        int GetStyleId(const Ref<FontStyle>& style) override;

        // Renders new characters with style effects
        void RenderNewCharacters(Vector<wchar_t>& newCharacters, int height, int styleId,
                                 const Vector<Ref<Effect>>& effects);

        // Packs character in line
        void PackCharacter(CharDef& character, int height);
    };
}
// --- META ---

CLASS_BASES_META(o2::VectorFont::Effect)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::VectorFont::Effect)
{
}
END_META;
CLASS_METHODS_META(o2::VectorFont::Effect)
{

    FUNCTION().PUBLIC().SIGNATURE(void, Process, Bitmap&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetSizeExtend);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, Effect*);
}
END_META;
// --- END META ---
