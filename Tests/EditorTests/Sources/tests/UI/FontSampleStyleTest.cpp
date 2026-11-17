#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/FontStyle.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2Editor/UI/Preview/FontSampleView.h"

using namespace o2;
using namespace Editor;

// A style that paints the glyphs keeps its colours in the samples; one with only a stroke or a shadow gets the text colour
TEST(FontSampleStyle, StylesThatPaintTheGlyphsKeepTheirColours)
{
    EXPECT_FALSE(FontSampleView::IsStylePaintingGlyphs(nullptr));

    auto strokeOnly = mmake<FontStyle>();
    strokeOnly->AddEffect<FontStrokeEffect>();
    EXPECT_FALSE(FontSampleView::IsStylePaintingGlyphs(strokeOnly));

    auto colored = mmake<FontStyle>();
    colored->AddEffect<FontStrokeEffect>();
    colored->AddEffect<FontColorEffect>();
    EXPECT_TRUE(FontSampleView::IsStylePaintingGlyphs(colored));

    auto gradient = mmake<FontStyle>();
    gradient->AddEffect<FontGradientEffect>();
    EXPECT_TRUE(FontSampleView::IsStylePaintingGlyphs(gradient));
}
