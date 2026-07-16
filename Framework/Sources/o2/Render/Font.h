#pragma once

#include "3rdPartyLibs/FreeType/include/ft2build.h"
#include FT_FREETYPE_H

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Mesh;
    class Render;
    class FontStyle;

    // -----------------------------------------------------------
    // Font. Containing array of symbol glyphs, symbol index table
    // -----------------------------------------------------------
    class Font: public RefCounterable
    {
    protected:
        struct Character;

    public:
        Function<void()> onCharactersRebuilt; // Called when characters was rebuilt

    public:
        // Default constructor
        Font();

        // Copy-constructor
        Font(const Font& font);

        // Destructor
        virtual ~Font();

        // Returns base height in pixels for font with size
        virtual float GetHeightPx(int height) const;

        // Returns line height in pixels for font with size
        virtual float GetLineHeightPx(int height) const;

        // Returns character constant reference by id
        const Character& GetCharacter(UInt16 id, int height);

        // Returns character constant reference by id, rendered with style
        virtual const Character& GetCharacter(UInt16 id, int height, const Ref<FontStyle>& style);

        // Checks characters for preloading
        void CheckCharacters(const WString& needChararacters, int height);

        // Checks characters for preloading, rendered with style
        virtual void CheckCharacters(const WString& needChararacters, int height, const Ref<FontStyle>& style);

        // Returns font file name
        virtual String GetFileName() const;

        // Returns texture
        const TextureRef& GetTexture() const;

        // Returns texture source rectangle
        const RectI& GetTextureSrcRect() const;

    protected:
        // --------------------
        // Character definition
        // --------------------
        struct Character
        {
            RectF  mTexSrc;      // texture source rect
            Vec2F  mSize;        // Size of source rect
            Vec2F  mOrigin;      // Symbol origin point
            float  mAdvance;     // Symbol advance
            UInt16 mId;          // Character id
            int    mHeight;      // Character height
            int    mStyleId = 0; // Rendered style id, 0 - no style

            bool operator==(const Character& other) const;
        };

    protected:
        Map<UInt64, Map<UInt16, Character>> mCharacters; // Characters map, uint64 - style and height key, uint16 - id

        TextureRef mTexture;        // Texture
        RectI        mTextureSrcRect; // Texture source rectangle

        bool mReady = false; // True when font is ready to use

    protected:
        // It is called after reference initialization at object construction, registers texture in render
        void PostRefConstruct();

        // Adds character and registers in cache map
        void AddCharacter(const Character& character);

        // Returns font-local style id for characters caching. 0 for empty style
        virtual int GetStyleId(const Ref<FontStyle>& style);

        // Returns characters map key, combined from style id and height
        static UInt64 GetStyleHeightKey(int styleId, int height);

        friend class Text;
        friend class Ref<Font>;
        friend class Render;

        FRIEND_REF_MAKE();
    };
}
