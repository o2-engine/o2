#include "o2Editor/stdafx.h"

#include <chrono>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "UiDocTestKit.h"

#include "UiDocRenderRefs.inc"

using namespace UiDocTest;

namespace
{
    // Writes a picture to $UIDOC_RENDER_DUMP/<name>.png when that is set (to look at renders by eye)
    void Dump(const UiImage& img, const String& name)
    {
        if (const char* dir = getenv("UIDOC_RENDER_DUMP"))
            Editor::PipelineUtils::WriteFileBytes(String(dir) + "/" + name + ".png", img.Png());
    }

    // The references: the web's renderLayoutSvg drawn by Chrome with the UI fonts (refs.json of render_refs.mts)
    const Json& Refs()
    {
        static const Json refs = []() {
            std::string text;
            for (const char* part : kUiDocRenderRefParts)
                text += part;
            Json out;
            Json::Parse(String(text), out);
            return out;
        }();
        return refs;
    }

    // How far two pictures of one size are apart: the mean channel difference (0..255, colours weighted by alpha)
    // and the share of pixels off by more than 48 in a channel
    struct Diff
    {
        double mean = 1e9, off = 1;
    };

    Diff Compare(const UiImage& a, const UiImage& b)
    {
        Diff d;
        if (a.w != b.w || a.h != b.h || a.IsEmpty())
            return d;
        double sum = 0;
        int off = 0;
        for (int i = 0; i < a.w*a.h; i++)
        {
            const UInt8* p = a.data.data() + (size_t)i*4;
            const UInt8* q = b.data.data() + (size_t)i*4;
            int worst = 0;
            for (int c = 0; c < 4; c++)
            {
                // Colours count as much as they show
                int va = c < 3 ? p[c]*p[3]/255 : p[3], vb = c < 3 ? q[c]*q[3]/255 : q[3];
                int diff = std::abs(va - vb);
                sum += diff;
                worst = std::max(worst, diff);
            }
            if (worst > 48)
                off++;
        }
        d.mean = sum/(a.w*a.h*4.0);
        d.off = (double)off/(a.w*a.h);
        return d;
    }

    // A pixel's straight RGBA
    const UInt8* At(const UiImage& img, int x, int y) { return img.At(std::clamp(x, 0, img.w - 1), std::clamp(y, 0, img.h - 1)); }
}

TEST(UiDocRender, FontsAreFound)
{
    EXPECT_FALSE(UiFontsHome().IsEmpty());
    for (auto& font : UiFonts())
    {
        for (auto& face : font.faces)
            EXPECT_TRUE(HasUiFont(face.file)) << face.file.Data();
    }
}

TEST(UiDocRender, MatchesTheWebsRendererWithinTolerance)
{
    const Json& refs = Refs();
    UiKit kit = ParseKit(S(refs, "kit"));
    for (auto& c : M(refs, "cases").Items())
    {
        String name = S(c, "name");
        SCOPED_TRACE(name.Data());
        UiImage ref;
        ASSERT_TRUE(UiImage::Decode(Editor::PipelineUtils::Base64Decode(S(c, "png")), ref));

        UiImage mine;
        if (name == "sample")
            mine = RenderTextStyleSample(kit.textStyles[0], S(refs, "sampleText"), true, ref.w/TextStyleSampleDoc(kit.textStyles[0], S(refs, "sampleText"), true).width);
        else
        {
            UiRenderOptions o;
            o.mode = S(c, "mode");
            o.kit = M(c, "kit").Bool() ? &kit : nullptr;
            o.pixelLong = (int)M(c, "long").Num();
            o.showIds = M(c, "showIds").Bool();
            o.width = M(c, "width").Num();
            o.height = M(c, "height").Num();
            mine = RenderLayout(Parse(S(c, "src")), o);
        }
        Dump(ref, name + "_ref");
        Dump(mine, name + "_o2");
        ASSERT_EQ(mine.w, ref.w);
        ASSERT_EQ(mine.h, ref.h);
        Diff d = Compare(mine, ref);
        printf("%s: mean %.2f, off %.2f%%\n", name.Data(), d.mean, d.off*100);
        EXPECT_LT(d.mean, 2.5);
        EXPECT_LT(d.off, 0.025);
    }
}

namespace
{
    // A picture of solid colours: the given colour inside, another in the corner squares of size c
    String TwoTonePng(int w, int h, int c, const UiRgba& inside, const UiRgba& corners)
    {
        UiImage img(w, h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                bool corner = (x < c || x >= w - c) && (y < c || y >= h - c);
                const UiRgba& col = corner ? corners : inside;
                UInt8* p = img.At(x, y);
                p[0] = (UInt8)(col.r*255); p[1] = (UInt8)(col.g*255); p[2] = (UInt8)(col.b*255); p[3] = 255;
            }
        }
        return Editor::PipelineUtils::Base64Encode(img.Png());
    }

    bool IsRed(const UInt8* p) { return p[0] > 180 && p[1] < 90 && p[2] < 90; }
    bool IsBlue(const UInt8* p) { return p[2] > 180 && p[0] < 90 && p[1] < 90; }
    bool IsGreen(const UInt8* p) { return p[1] > 150 && p[0] < 100 && p[2] < 100; }
}

// --- the web's tests (backend/test/uiLayout.test.ts: the layout renders with the kit) ---------------------------

TEST(UiDocRender, TheKitButtonIsDrawnWhereThePlayButtonIs)
{
    // The play button (220,660 640x140) is drawn with the kit's red, at the design size
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    UiImage img = RenderLayout(Parse(Screen()), o);
    ASSERT_EQ(img.w, 1080);
    ASSERT_EQ(img.h, 1920);
    EXPECT_TRUE(IsRed(At(img, 240, 730)));
}

// --- geometry ---------------------------------------------------------------------------------------------------

TEST(UiDocRender, NineSliceCornersKeepTheirSizeAtTheGraphicsDensity)
{
    UiKitElement el;
    el.name = "btn";
    el.type = "button";
    el.assetType = "sliced";
    el.width = 100;
    el.height = 100;
    el.nineSlice = UiNine{ 40, 40, 40, 40 };
    el.dpi = 960;
    el.png = TwoTonePng(100, 100, 40, UiRgba{ 1, 0, 0, 1 }, UiRgba{ 0, 0, 1, 1 });
    UiKit kit;
    kit.elements = { el };
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;

    // A 480-dpi layout draws the 960-dpi graphic at half: 20 px corners
    UiImage half = RenderLayout(Parse("screen M 600x400 dpi=480\n  button b anchor=center size=300x100 kit=btn"), o);
    EXPECT_TRUE(IsBlue(At(half, 150 + 10, 150 + 10)));
    EXPECT_TRUE(IsRed(At(half, 150 + 30, 150 + 10)));
    EXPECT_TRUE(IsRed(At(half, 150 + 150, 150 + 50)));
    EXPECT_TRUE(IsBlue(At(half, 450 - 10, 250 - 10)));
    // At its own density: 40 px corners; scaled canvases scale them with the screen
    UiImage same = RenderLayout(Parse("screen M 600x400 dpi=960\n  button b anchor=center size=300x100 kit=btn"), o);
    EXPECT_TRUE(IsBlue(At(same, 150 + 30, 150 + 10)));
    EXPECT_TRUE(IsRed(At(same, 150 + 50, 150 + 10)));
    o.scale = 0.5;
    UiImage small = RenderLayout(Parse("screen M 600x400 dpi=480\n  button b anchor=center size=300x100 kit=btn"), o);
    EXPECT_EQ(small.w, 300);
    EXPECT_TRUE(IsBlue(At(small, 75 + 4, 75 + 4)));
    EXPECT_TRUE(IsRed(At(small, 75 + 15, 75 + 5)));
}

TEST(UiDocRender, AProgressFillIsCutToTheValue)
{
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    UiImage img = RenderLayout(Parse("screen M 1000x200\n  progress hp anchor=center size=800x60 value=0.3"), o);
    // The box is 100..900 across, 70..130 down: green to 340, the dark track after
    EXPECT_TRUE(IsGreen(At(img, 200, 100)));
    EXPECT_TRUE(IsGreen(At(img, 330, 100)));
    const UInt8* track = At(img, 500, 100);
    EXPECT_LT(track[1], 100);
    EXPECT_LT(track[0], 100);
}

TEST(UiDocRender, LabelsAreInkedInsideTheirBoxes)
{
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiDoc doc = Parse("screen M 1000x600\n  text title anchor=top-left pos=100,100 size=800x160 style=title \"Forest Quest\"\n"
                      "  text body anchor=top-left pos=100,400 size=300x100 textColor=#FF0000 \"Hello\"");
    for (const char* mode : { "kit", "lines" })
    {
        UiRenderOptions o;
        o.mode = mode;
        o.kit = &kit;
        UiImage img = RenderLayout(doc, o);
        UiImage bare = RenderLayout(Parse("screen M 1000x600"), o);
        int inside = 0, outside = 0;
        for (int y = 0; y < img.h; y++)
        {
            for (int x = 0; x < img.w; x++)
            {
                const UInt8* p = img.At(x, y);
                const UInt8* q = bare.At(x, y);
                if (std::abs(p[0] - q[0]) + std::abs(p[1] - q[1]) + std::abs(p[2] - q[2]) < 30)
                    continue;
                bool inTitle = x >= 100 && x < 900 && y >= 100 && y < 260;
                bool inBody = x >= 100 && x < 400 && y >= 400 && y < 500;
                (inTitle || inBody ? inside : outside)++;
            }
        }
        EXPECT_GT(inside, 2000) << mode;
        // The title's drop shadow may reach a little below its box
        EXPECT_LT(outside, inside/50) << mode;
    }
}

TEST(UiDocRender, HiddenBlocksMissingGraphicsAndOpacity)
{
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    UiImage img = RenderLayout(Parse("screen M 600x400\n  button a anchor=top-left pos=50,50 size=200x100 kit=btn_red hidden\n"
                                     "  button b anchor=top-left pos=300,50 size=200x100 kit=nope\n"
                                     "  panel c anchor=top-left pos=50,250 size=200x100 kit=panel_wood opacity=0.5"), o);
    // Hidden: the paper shows
    const UInt8* paper = At(img, 150, 100);
    EXPECT_LT(std::abs(paper[0] - 0x20) + std::abs(paper[1] - 0x23) + std::abs(paper[2] - 0x2B), 6);
    // A graphic the kit lacks: line art outlined dashed orange
    int orange = 0;
    for (int x = 300; x < 500; x++)
    {
        // The 1.5 px stroke straddles y = 50: the row below the edge carries most of it
        const UInt8* p = At(img, x, 50);
        if (p[0] > p[1] + 40 && p[1] > p[2] + 20 && p[0] > 120)
            orange++;
    }
    EXPECT_GT(orange, 40);
    EXPECT_LT(orange, 180); // dashed
    // Half opacity: between the paper and the wood
    UiRenderOptions full = o;
    UiImage solid = RenderLayout(Parse("screen M 600x400\n  panel c anchor=top-left pos=50,250 size=200x100 kit=panel_wood"), full);
    const UInt8* faded = At(img, 150, 300);
    const UInt8* wood = At(solid, 150, 300);
    EXPECT_NEAR(faded[0], (wood[0] + 0x20)/2.0, 6);
}

TEST(UiDocRender, TheKitSheetShowsEveryGraphic)
{
    UiKit kit = ParseKit(S(Refs(), "kit"));
    ASSERT_EQ(kit.elements.Count(), 8);
    UiImage sheet = RenderKitSheet(kit);
    // Five across, two rows of 204 x 196 cells with 10 px around
    EXPECT_EQ(sheet.w, 5*204 + 20);
    EXPECT_EQ(sheet.h, 2*196 + 20);
    // The sixth graphic (progress_fill, green) in the first cell of the second row, its name under it
    const UInt8* fillPx = At(sheet, 10 + 102, 10 + 196 + 10 + 68);
    EXPECT_TRUE(IsGreen(fillPx));
    int ink = 0;
    for (int y = 10 + 196 + 136 + 22; y < 10 + 196 + 136 + 40; y++)
        for (int x = 18; x < 150; x++)
            ink += At(sheet, x, y)[0] > 200;
    EXPECT_GT(ink, 30);
    // A checker cell around a picture, the sheet's dark paper between cells
    const UInt8* cell = At(sheet, 10 + 8, 10 + 8);
    EXPECT_TRUE((cell[0] == 0x2B || cell[0] == 0x35));
    const UInt8* paper = At(sheet, 5, 5);
    EXPECT_EQ(paper[0], 0x1B);

    UiKit three = kit;
    three.elements.Resize(3);
    UiImage small = RenderKitSheet(three, 40, 480.0);
    EXPECT_EQ(small.w, 3*204 + 20);
    EXPECT_EQ(small.h, 196 + 20);
    EXPECT_TRUE(RenderKitSheet(UiKit()).IsEmpty());
    EXPECT_EQ(RenderKitSheet(kit, 2).w, 2*204 + 20);
}

TEST(UiDocRender, ATextStyleSampleWidensWithItsText)
{
    TextStyle st = TextStyleDefault();
    st.size = 120;
    UiDoc shortDoc = TextStyleSampleDoc(st, "Hi", true);
    UiDoc longDoc = TextStyleSampleDoc(st, "A rather long sample text", true);
    EXPECT_EQ(shortDoc.width, 680);
    EXPECT_GT(longDoc.width, 680);
    EXPECT_EQ(longDoc.height, 224);
    UiImage img = RenderTextStyleSample(st, "Hi", false, 0.5);
    EXPECT_EQ(img.w, 340);
    EXPECT_EQ(img.At(2, 2)[0], 0xF3);
}

TEST(UiDocRender, AKitLayoutDrawsQuicklyAt600Pixels)
{
    const Json& game = *M(Refs(), "cases").At(1);
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiDoc doc = Parse(S(game, "src"));
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    o.pixelLong = 600;
    RenderLayout(doc, o); // Pictures, faces and glyphs cached
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 5; i++)
        RenderLayout(doc, o);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count()/5;
    o.pixelLong = 0;
    auto t1 = std::chrono::steady_clock::now();
    RenderLayout(doc, o);
    double full = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
    printf("1080x1920 kit layout: %.1f ms at 600 px, %.1f ms at full size\n", ms, full);
    EXPECT_LT(ms, 100); // Debug build; about 25 ms on an M-series Mac
}

TEST(UiDocRender, DISABLED_ProfileLoop)
{
    UiDoc doc = Parse(S(*M(Refs(), "cases").At(1), "src"));
    UiKit kit = ParseKit(S(Refs(), "kit"));
    UiRenderOptions o;
    o.pixelLong = 600;
    o.kit = &kit;
    for (int i = 0; i < 600; i++)
    {
        o.mode = i % 2 ? "kit" : "lines";
        RenderLayout(doc, o);
    }
}
