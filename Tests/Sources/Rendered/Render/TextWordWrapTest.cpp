#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFont.h"

using namespace o2;

// A line ended by '\n' must not hand its last space to the next line as a wrap point
TEST(TextWordWrap, LongWordAfterLineBreakWrapsInsideItsOwnLine)
{
    auto font = mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    WString text = "ab cd\nefghijklmnopqrstuvwxyz";
    font->CheckCharacters(text, 20, nullptr);

    Text::SymbolsSet symbols;
    ASSERT_NO_THROW(symbols.Initialize(font, nullptr, text, 20, Vec2F(), Vec2F(60, 400), HorAlign::Left, VerAlign::Top,
                                       true, false, 1.0f, 1.0f));

    WString joined;
    for (auto& line : symbols.mLines)
    {
        joined += line.mString;
        EXPECT_EQ(line.mSymbols.Count(), line.mString.Length());
        if (line.mSymbols.Count() > 1)
            EXPECT_LE(line.mSize.x, 60.0f) << (String)line.mString;
    }

    EXPECT_EQ((String)joined, String("ab cdefghijklmnopqrstuvwxyz"));
    EXPECT_GT(symbols.mLines.Count(), 3);
}
