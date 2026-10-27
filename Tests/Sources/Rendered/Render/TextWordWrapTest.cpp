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

// A line broken at a space keeps that space at its end: the next line starts with the next word, as browsers lay text out
TEST(TextWordWrap, WrapAtSpaceStartsTheNextLineWithTheWord)
{
    auto font = mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    WString text = "alpha beta gamma delta epsilon";
    font->CheckCharacters(text, 20, nullptr);
    float wordsWidth = Text::GetTextSize("alpha beta", font, 20, Vec2F(), HorAlign::Left, VerAlign::Top, false).x;

    Text::SymbolsSet symbols;
    symbols.Initialize(font, nullptr, text, 20, Vec2F(), Vec2F(wordsWidth + 4.0f, 400), HorAlign::Left, VerAlign::Top,
                       true, false, 1.0f, 1.0f);

    ASSERT_GE(symbols.mLines.Count(), 2);
    WString joined;
    for (int i = 0; i < symbols.mLines.Count(); i++)
    {
        auto& line = symbols.mLines[i];
        joined += line.mString;
        EXPECT_EQ(line.mSymbols.Count(), line.mString.Length());
        if (i > 0)
            EXPECT_NE(line.mString[0], L' ') << (String)line.mString;
        EXPECT_LE(line.mSize.x, wordsWidth + 4.0f) << (String)line.mString;
    }
    EXPECT_EQ((String)symbols.mLines[0].mString, String("alpha beta "));
    EXPECT_EQ((String)joined, (String)text);
}
