#include "o2Editor/stdafx.h"

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"

#include "UiDocTestKit.h"

#include "UiDocRenderRefs2.inc"

// Labels with a gradient fill and layer effects, and kit=none plain blocks, against the web's renderer drawn by Chrome
// (UiDocGolden/render_refs2.mts)
namespace
{
    using namespace UiDocTest;

    const Json& Refs2()
    {
        static const Json refs = []() {
            std::string text;
            for (const char* part : kUiDocRenderRefs2Parts)
                text += part;
            Json out;
            Json::Parse(String(text), out);
            return out;
        }();
        return refs;
    }

    void Dump(const UiImage& img, const String& name)
    {
        if (const char* dir = getenv("UIDOC_RENDER_DUMP"))
            Editor::PipelineUtils::WriteFileBytes(String(dir) + "/" + name + ".png", img.Png());
    }

    // Mean channel difference (colours weighted by alpha) and the share of pixels off by more than 48 in a channel
    std::pair<double, double> Compare(const UiImage& a, const UiImage& b)
    {
        double sum = 0;
        int off = 0;
        for (int i = 0; i < a.w*a.h; i++)
        {
            const UInt8* p = a.data.data() + (size_t)i*4;
            const UInt8* q = b.data.data() + (size_t)i*4;
            int worst = 0;
            for (int c = 0; c < 4; c++)
            {
                int va = c < 3 ? p[c]*p[3]/255 : p[3], vb = c < 3 ? q[c]*q[3]/255 : q[3];
                sum += std::abs(va - vb);
                worst = std::max(worst, std::abs(va - vb));
            }
            if (worst > 48)
                off++;
        }
        return { sum/(a.w*a.h*4.0), (double)off/(a.w*a.h) };
    }
}

TEST(UiDocRenderEffects, TextEffectsMatchTheWebsRenderer)
{
    const Json& refs = Refs2();
    String text = S(refs, "text");
    for (auto& c : M(refs, "cases").Items())
    {
        String name = S(c, "name");
        SCOPED_TRACE(name.Data());
        Json styleJson;
        ASSERT_TRUE(Json::Parse(S(c, "style"), styleJson));
        TextStyle style = NormalizeTextStyle(styleJson, name);
        EXPECT_STR_EQ(TextStyleToJson(style).Dump(), styleJson.Dump());

        UiDoc doc = SampleDoc(style, text, "#20232B");
        EXPECT_STR_EQ(DocToJson(doc).Dump(), S(c, "doc"));
        UiKit kit;
        kit.textStyles.Add(style);
        UiRenderOptions o;
        o.mode = "kit";
        o.kit = &kit;
        o.pixelLong = (int)std::max(doc.width, doc.height);
        UiImage mine = RenderLayout(doc, o);

        UiImage ref;
        ASSERT_TRUE(UiImage::Decode(Editor::PipelineUtils::Base64Decode(S(c, "png")), ref));
        Dump(ref, "fx_" + name + "_ref");
        Dump(mine, "fx_" + name + "_o2");
        ASSERT_EQ(mine.w, ref.w);
        ASSERT_EQ(mine.h, ref.h);
        auto d = Compare(mine, ref);
        printf("%s: mean %.2f, off %.2f%%\n", name.Data(), d.first, d.second*100);
        EXPECT_LT(d.first, 4.0);
        EXPECT_LT(d.second, 0.05);
    }
}

TEST(UiDocRenderEffects, PlainBlocksDrawTheirColourAndNoGraphic)
{
    const Json& plain = M(Refs2(), "plain");
    UiKit kit = ParseKit(S(plain, "kit"));
    UiDoc doc = Parse(S(plain, "src"));
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    o.pixelLong = 600;
    UiImage mine = RenderLayout(doc, o);
    UiImage ref;
    ASSERT_TRUE(UiImage::Decode(Editor::PipelineUtils::Base64Decode(S(plain, "png")), ref));
    Dump(ref, "fx_plain_ref");
    Dump(mine, "fx_plain_o2");
    ASSERT_EQ(mine.w, ref.w);
    auto d = Compare(mine, ref);
    printf("plain: mean %.2f, off %.2f%%\n", d.first, d.second*100);
    EXPECT_LT(d.first, 2.5);
    EXPECT_LT(d.second, 0.025);

    // Plain blocks count neither as covered nor as missing
    auto report = KitReport(doc, &kit);
    EXPECT_EQ(report.total, 0);
    EXPECT_TRUE(report.missing.IsEmpty());
}
