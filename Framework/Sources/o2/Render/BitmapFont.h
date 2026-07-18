#pragma once

#include "o2/Render/Font.h"

namespace o2
{
    // -----------
    // Bitmap font
    // -----------
    class BitmapFont: public Font
    {
    public:
        // Default constructor
        BitmapFont();

        // Constructor with loading from file
        BitmapFont(const String& fileName);

        // Loads bitmap font data from file
        bool Load(const String& fileName);

        // Returns file name
        String GetFileName() const;

        // Returns base height in pixels for font with size
        float GetHeightPx(int height) const;

        // Returns line height in pixels for font with size
        float GetLineHeightPx(int height) const;

        using Font::GetCharacter;

        // Returns character constant reference by id. Bitmap font ignores height and style
        const Character& GetCharacter(UInt16 id, int height, const Ref<FontStyle>& style) override;

    protected:
        String mFileName;   // Source file name
        float  mBaseHeight; // Source font height
        float  mLineHeight; // Source font line height
    };
}
