#include "o2Editor/stdafx.h"

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "UiDocTestKit.h"

using namespace UiDocTest;
using namespace Editor;

namespace
{
    // A golden picture's PNG bytes
    String Picture(const String& name)
    {
        return PipelineUtils::Base64Decode(S(M(G("prep"), "pics"), name.Data()));
    }

    String RawSha(const UiImage& img)
    {
        return Sha1Hex(String(std::string(img.data.begin(), img.data.end())));
    }

    UiPrepInput InputOf(const String& pic, const Json& p)
    {
        UiPrepInput in;
        in.data = Picture(pic);
        in.assetType = S(p, "assetType");
        in.key = M(p, "key").Bool();
        in.trim = M(p, "trim").Bool();
        in.optimize = M(p, "optimize").Bool();
        in.nineSlice = NormalizeNine(p.Get("nineSlice"));
        return in;
    }
}

// --- the web's tests (backend/test/uiLayout.test.ts: kit element preparation) --------------------------------------------

TEST(UiKitPrep, TheGreenBackdropGoesMarginsTrimTheUniformMiddleCollapsesWithItsBordersKept)
{
    UiPrepInput in;
    in.data = Picture("green");
    in.assetType = "sliced";
    in.key = true;
    UiPrepOutput p;
    ASSERT_TRUE(PrepareElement(in, p));
    double trimW = M(M(p.source, "trim"), "w").Num();
    EXPECT_LE(std::fabs(trimW - 286), 3) << p.source.Dump().Data();
    ASSERT_TRUE(p.nineSlice.has_value());
    EXPECT_GE(p.nineSlice->left, 20);
    EXPECT_LT(p.nineSlice->left, 60);
    EXPECT_LT(p.width, 140);
    EXPECT_LT(p.height, 120);
    EXPECT_EQ(p.width, p.nineSlice->left + 4 + p.nineSlice->right);

    UiImage back;
    ASSERT_TRUE(UiImage::Decode(p.png, back));
    EXPECT_EQ(back.data, p.image.data); // o2's PNG keeps the pixels
    EXPECT_EQ(back.At(0, 0)[3], 0);    // the rounded corner is clear
    const UInt8* mid = back.At(back.w >> 1, back.h >> 1);
    EXPECT_GT(mid[0], 180);
    EXPECT_LT(mid[1], 100);
    EXPECT_EQ(mid[3], 255);
}

TEST(UiKitPrep, BordersGivenInInputPixelsMapIntoTheTrimmedPicture)
{
    UiPrepInput in;
    in.data = Picture("green");
    in.assetType = "sliced";
    in.key = true;
    in.optimize = false;
    in.nineSlice = UiNine{ 80, 70, 80, 70 };
    UiPrepOutput p;
    ASSERT_TRUE(PrepareElement(in, p));
    const Json& t = M(p.source, "trim");
    double tx = M(t, "x").Num(), ty = M(t, "y").Num(), tw = M(t, "w").Num(), th = M(t, "h").Num();
    EXPECT_EQ(*p.nineSlice, (UiNine{ 80 - tx, 70 - ty, 80 - (360 - tx - tw), 70 - (200 - ty - th) }));
}

// --- golden ------------------------------------------------------------------------------------------------------

TEST(UiKitPrep, GoldenDecodersAgreeAndInputHashes)
{
    const Json& g = G("prep");
    for (auto& m : M(g, "pics").Members())
    {
        String bytes = Picture(m.first);
        UiImage img;
        ASSERT_TRUE(UiImage::Decode(bytes, img)) << m.first.Data();
        EXPECT_STR_EQ(RawSha(img), S(M(g, "inputShas"), m.first.Data())) << m.first.Data();
        EXPECT_STR_EQ(InputHash(bytes), S(M(g, "hashes"), m.first.Data())) << m.first.Data();
    }
    EXPECT_STR_EQ(Sha1Hex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    EXPECT_STR_EQ(Sha1Hex("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST(UiKitPrep, GoldenPreparedElements)
{
    for (auto& c : M(G("prep"), "results").Items())
    {
        SCOPED_TRACE((S(c, "pic") + " " + M(c, "p").Dump()).Data());
        UiPrepOutput out;
        ASSERT_TRUE(PrepareElement(InputOf(S(c, "pic"), M(c, "p")), out));
        EXPECT_EQ(out.width, M(c, "width").Num());
        EXPECT_EQ(out.height, M(c, "height").Num());
        EXPECT_STR_EQ(out.nineSlice ? NineToJson(*out.nineSlice).Dump() : String("\"__undef__\""), M(c, "nineSlice").Dump());
        EXPECT_STR_EQ(out.source.Dump(), M(c, "source").Dump());
        EXPECT_STR_EQ(out.detected ? NineToJson(*out.detected).Dump() : String("\"__undef__\""), M(c, "detected").Dump());
        EXPECT_STR_EQ(RawSha(out.image), S(c, "sha"));
    }

    for (auto& c : M(G("prep"), "detect").Items())
    {
        UiInputNine r;
        ASSERT_TRUE(DetectInputNine(Picture(S(c, "pic")), M(c, "key").Bool(), r));
        Json j = Json::Object();
        j["nine"] = NineToJson(r.nine);
        j["uniformX"] = r.uniformX;
        j["uniformY"] = r.uniformY;
        j["w"] = r.w;
        j["h"] = r.h;
        EXPECT_STR_EQ(j.Dump(), M(c, "r").Dump()) << c.Dump().Data();
    }

    UiPrepOutput none;
    UiPrepInput bad;
    bad.data = "not a picture";
    EXPECT_FALSE(PrepareElement(bad, none));
}
