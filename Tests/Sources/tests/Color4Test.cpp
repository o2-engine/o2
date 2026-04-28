#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Color.h"

using namespace o2;

TEST(Color4, DefaultIsOpaqueWhite) {
    Color4 c;
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
    EXPECT_EQ(c.a, 255);
}

TEST(Color4, IntFloatConstructorEquivalent) {
    Color4 ci(128, 64, 32, 200);
    Color4 cf(128.0f / 255.0f, 64.0f / 255.0f, 32.0f / 255.0f, 200.0f / 255.0f);

    // Float constructor uses (int)(v*255), so may be off by one due to truncation.
    EXPECT_NEAR(ci.r, cf.r, 1);
    EXPECT_NEAR(ci.g, cf.g, 1);
    EXPECT_NEAR(ci.b, cf.b, 1);
    EXPECT_NEAR(ci.a, cf.a, 1);
}

TEST(Color4, FloatComponentRoundtrip) {
    Color4 c(0.5f, 0.25f, 0.75f, 1.0f);
    EXPECT_NEAR(c.RF(), 0.5f, 1.0f / 255.0f);
    EXPECT_NEAR(c.GF(), 0.25f, 1.0f / 255.0f);
    EXPECT_NEAR(c.BF(), 0.75f, 1.0f / 255.0f);
    EXPECT_NEAR(c.AF(), 1.0f, 1.0f / 255.0f);
}

TEST(Color4, ARGBRoundtrip) {
    Color4 c(11, 22, 33, 44);
    Color4 d;
    d.SetARGB(c.ARGB());
    EXPECT_EQ(d, c);
}

TEST(Color4, ABGRRoundtrip) {
    Color4 c(11, 22, 33, 44);
    Color4 d;
    d.SetABGR(c.ABGR());
    EXPECT_EQ(d, c);
}

TEST(Color4, RGBARoundtrip) {
    Color4 c(11, 22, 33, 44);
    Color4 d;
    d.SetRGBA(c.RGBA());
    EXPECT_EQ(d, c);
}

TEST(Color4, ARGBvsABGRDifferAtRBBytes) {
    Color4 c(0xAB, 0xCD, 0xEF, 0x12);
    Color32Bit argb = c.ARGB();
    Color32Bit abgr = c.ABGR();

    // ARGB layout: a r g b. ABGR layout: a b g r.
    EXPECT_EQ((argb >> 24) & 0xFF, 0x12u);
    EXPECT_EQ((argb >> 16) & 0xFF, 0xABu);
    EXPECT_EQ((argb >>  8) & 0xFF, 0xCDu);
    EXPECT_EQ((argb >>  0) & 0xFF, 0xEFu);

    EXPECT_EQ((abgr >> 24) & 0xFF, 0x12u);
    EXPECT_EQ((abgr >> 16) & 0xFF, 0xEFu);
    EXPECT_EQ((abgr >>  8) & 0xFF, 0xCDu);
    EXPECT_EQ((abgr >>  0) & 0xFF, 0xABu);
}

TEST(Color4, NormalizedClampsToZero255) {
    Color4 c(-50, 300, 128, 1000);
    Color4 n = c.Normalized();
    EXPECT_EQ(n.r, 0);
    EXPECT_EQ(n.g, 255);
    EXPECT_EQ(n.b, 128);
    EXPECT_EQ(n.a, 255);

    c.Normalize();
    EXPECT_EQ(c, n);
}

TEST(Color4, AdditionDoesNotClampWithoutNormalize) {
    Color4 a(200, 200, 200, 200);
    Color4 b(100, 100, 100, 100);
    Color4 sum = a + b;
    EXPECT_EQ(sum.r, 300);
    EXPECT_EQ(sum.g, 300);
    EXPECT_EQ(sum.b, 300);
    EXPECT_EQ(sum.a, 300);

    Color4 normalized = sum.Normalized();
    EXPECT_EQ(normalized.r, 255);
    EXPECT_EQ(normalized.a, 255);
}

TEST(Color4, MultiplyByColorIsModulation) {
    Color4 white(255, 255, 255, 255);
    Color4 half(128, 128, 128, 128);

    Color4 modulated = white * half;
    // 255 * 128 / 255 = 128
    EXPECT_EQ(modulated.r, 128);
    EXPECT_EQ(modulated.g, 128);
    EXPECT_EQ(modulated.b, 128);
    EXPECT_EQ(modulated.a, 128);
}

TEST(Color4, MultiplyByScalar) {
    Color4 c(100, 200, 0, 255);
    Color4 scaled = c * 0.5f;
    EXPECT_EQ(scaled.r, 50);
    EXPECT_EQ(scaled.g, 100);
    EXPECT_EQ(scaled.b, 0);
    EXPECT_NEAR(scaled.a, 127, 1);
}

TEST(Color4, HSLRoundtripPreservesColor) {
    // Pick a few non-degenerate colors with non-zero saturation.
    Color4 inputs[] = {
        Color4(200, 100,  50, 255),
        Color4( 50, 200, 100, 255),
        Color4(100,  50, 200, 255),
        Color4(180, 180,  10, 255),
    };

    for (const Color4& orig : inputs)
    {
        float h, s, l;
        orig.ToHSL(h, s, l);

        Color4 reconstructed;
        reconstructed.SetHSL(h, s, l);
        reconstructed.a = orig.a;

        // Quantization at 1/255 + HSL math may cause off-by-1 per channel.
        EXPECT_NEAR(reconstructed.r, orig.r, 2);
        EXPECT_NEAR(reconstructed.g, orig.g, 2);
        EXPECT_NEAR(reconstructed.b, orig.b, 2);
    }
}

TEST(Color4, HSLZeroSaturationIsGray) {
    Color4 gray;
    gray.SetHSL(0.5f, 0.0f, 0.5f);
    EXPECT_EQ(gray.r, gray.g);
    EXPECT_EQ(gray.g, gray.b);
}

TEST(Color4, ChangeHueByOneIsIdentity) {
    Color4 c(123, 200, 50, 255);
    Color4 original = c;
    c.ChangeHue(1.0f); // hue is in [0,1], full rotation = +1.0

    EXPECT_NEAR(c.r, original.r, 2);
    EXPECT_NEAR(c.g, original.g, 2);
    EXPECT_NEAR(c.b, original.b, 2);
}

TEST(Color4, BlendByAlphaWithOpaqueOther) {
    // If self.alpha is 1, blend collapses to self (a2*(1-a1)*other == 0).
    Color4 self(200, 100, 50, 255);
    Color4 other(0, 0, 0, 255);

    Color4 blended = self.BlendByAlpha(other);
    EXPECT_EQ(blended.r, self.r);
    EXPECT_EQ(blended.g, self.g);
    EXPECT_EQ(blended.b, self.b);
}

TEST(Color4, StaticPresets) {
    EXPECT_EQ(Color4::White(), Color4(255, 255, 255, 255));
    EXPECT_EQ(Color4::Black(), Color4(0, 0, 0, 255));
    EXPECT_EQ(Color4::Red(),   Color4(255, 0, 0, 255));
    EXPECT_EQ(Color4::Green(), Color4(0, 255, 0, 255));
    EXPECT_EQ(Color4::Blue(),  Color4(0, 0, 255, 255));
}

TEST(Color4, EqualityIsExact) {
    Color4 a(10, 20, 30, 40);
    Color4 b(10, 20, 30, 40);
    Color4 c(10, 20, 30, 41);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}
