#include "o2Editor/stdafx.h"

#include <functional>

#include "Network/NetworkTestHelpers.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocExport.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

#include "UiDocTestKit.h"

#include "UiDocExportRefs.inc"

using namespace UiDocTest;
namespace PipelineUtils = Editor::PipelineUtils;
namespace UiLayout = Editor::UiLayout;

// The AI UI layout's Save to project: the o2 prototype's files with the uids the web derives (UiDocExportRefs.inc, from
// UiDocGolden/export_refs.mts), the sprites at the layout's density, every font the styles use and the text looks as font
// styles

namespace
{
    const Json& Refs()
    {
        static const Json refs = []() {
            std::string text;
            for (const char* part : kUiDocExportRefParts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the export references do not parse: " << error.Data();
            return out;
        }();
        return refs;
    }

    // The kit of a structure case, as the golden script made it
    UiKit StructureKit(const Json& tag)
    {
        const Json& g = G("structure");
        if (tag.Str() == "A")
            return ParseKit(S(g, "kitA"));
        if (tag.Str() == "KIT")
        {
            UiKit kit = GoldenKit();
            for (auto& e : kit.elements)
                e.png = S(g, "png");
            return kit;
        }
        return ParseKit(tag.Str());
    }

    // A structure case of the golden data by name
    const Json* StructureCase(const String& name)
    {
        for (auto& c : M(G("structure"), "cases").Items())
        {
            if (S(c, "name") == name)
                return &c;
        }
        return nullptr;
    }

    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./uidoc-export-" + (String)(int)(Math::Random()%1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
        }

        ~WorkDir()
        {
            o2FileSystem.FolderRemove(path, true);
        }
    };

    UiLayoutExportResult RunExport(const UiLayoutExportRequest& request)
    {
        auto co = ExportLayout(request);
        co.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return co.IsDone(); }, 30.0f));
        return co.GetResult();
    }

    UiLayoutExportRequest RequestFor(const char* caseName)
    {
        const Json* c = StructureCase(caseName);
        UiLayoutExportRequest request;
        request.doc = Parse(S(*c, "src"));
        if (!M(*c, "kit").IsNull())
            request.kit = StructureKit(M(*c, "kit"));
        request.nodeId = "node_o2";
        return request;
    }

    // A kit whose title style is Lobster with a gradient, an outline, a drop shadow, a glow and an inner shadow
    UiKit StyledKit()
    {
        return ParseKit(R"({"name":"k","elements":[],"textStyles":[{"name":"title","role":"title","font":"Lobster","weight":400,
            "size":64,"color":"#FFF0A0","fill":{"angle":90,"stops":[{"color":"#FFF0A0","at":0},{"color":"#FF8000","at":1}]},
            "effects":[{"type":"stroke","color":"#402000","width":4,"opacity":1},
                       {"type":"shadow","color":"#000000","x":0,"y":6,"blur":4,"spread":0,"opacity":0.5},
                       {"type":"glow","color":"#FFFFFF","size":8,"opacity":0.8},
                       {"type":"innerShadow","color":"#000000","x":0,"y":2,"blur":2,"opacity":0.5}]}]})");
    }

    UiDoc StyledDoc()
    {
        return Parse("screen M 1000x600\n  text title anchor=top-left pos=100,100 size=800x160 style=title \"Forest\"");
    }

    Json ParsedJson(const String& text)
    {
        Json out;
        EXPECT_TRUE(Json::Parse(text, out)) << text.Data();
        return out;
    }

    const UiLayout::UiPlanNode* PlanNode(const UiLayout::UiWidgetPlan& plan, const String& name)
    {
        for (auto& p : plan.nodes)
        {
            if (p.name == name)
                return &p;
        }
        return nullptr;
    }

    // Records the fonts and looks the builder gives the labels
    class RecordingResources: public UiLayout::UiWidgetResources
    {
    public:
        Vector<String>              fonts;
        Vector<UiLayout::UiTextLook> looks;

        Ref<Sprite> SpriteOf(const String& asset) override { return nullptr; }
        Ref<Sprite> WhiteSprite() override { return mmake<Sprite>(); }
        void SetFont(const Ref<Label>& label, const String& fontFile) override { fonts.Add(fontFile); }
        void SetLook(const Ref<Label>& label, const UiLayout::UiTextLook& look) override { looks.Add(look); }
    };

    String ColorHex(const Json& color)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X", (int)M(color, "r").Num(), (int)M(color, "g").Num(), (int)M(color, "b").Num(),
                 (int)M(color, "a").Num());
        return buffer;
    }
}

TEST(UiDocExport, TheFolderAndThePrototypePath)
{
    EXPECT_STR_EQ(ExportFolderOf(""), "Generated/UI");
    EXPECT_STR_EQ(ExportFolderOf("  "), "Generated/UI");
    EXPECT_STR_EQ(ExportFolderOf(" /My/Folder/ "), "My/Folder");
    EXPECT_STR_EQ(ExportFolderOf("UI\\Screens\\"), "UI/Screens");
    EXPECT_STR_EQ(ExportPrototypePathOf(Parse("screen Main 100x100"), ""), "Generated/UI/Main.proto");
    EXPECT_STR_EQ(ExportPrototypePathOf(Parse("screen Main 100x100"), "UI/Menus"), "UI/Menus/Main.proto");
}

// Into a project's assets that are not the editor's: sprites, fonts and metas written with the uids the web's o2 export
// derives from the node id, the .proto left to the editor that owns the assets
TEST(UiDocExport, TheO2ExportWritesTheResourcesWithTheWebsUids)
{
    WorkDir work;
    auto request = RequestFor("screen_kitA");
    request.assetsPath = work.path;
    auto res = RunExport(request);
    ASSERT_TRUE(res.ok) << res.error.Data();
    EXPECT_TRUE(res.prototype.IsEmpty());
    EXPECT_TRUE(res.warnings.Any([](const String& w) { return w.Contains("the .proto not"); }));
    EXPECT_TRUE(res.written.Contains("Generated/UI/Menu_Resources/Sprites/btn_red.png"));
    EXPECT_TRUE(res.written.Contains("Generated/UI/Menu_Resources/Fonts/LICENSE-Roboto.txt"));

    int compared = 0;
    for (auto& f : M(M(Refs(), "o2"), "files").Items())
    {
        String path = f.At(0)->Str().SubStr(7); // Without "Assets/"
        String meta = PipelineUtils::ReadFileBytes(work.path + path);
        if (meta.IsEmpty())
            continue;
        Json value;
        ASSERT_TRUE(Json::Parse(meta, value)) << path.Data();
        EXPECT_STR_EQ(S(M(value, "Value"), "mId"), f.At(1)->Str()) << path.Data();
        compared++;
    }
    EXPECT_GE(compared, 8);
}

// A sprite drawn for another density goes to o2 resampled to the layout's, its borders (the meta's and the ones the
// layout gives it) with it
TEST(UiDocExport, TheO2ExportResamplesDenserSprites)
{
    const Json* c = StructureCase("dpi_kit");
    UiDoc doc = Parse(S(*c, "src"));
    UiKit kit = StructureKit(M(*c, "kit"));
    auto st = ToUiStructure(doc, &kit);
    ASSERT_EQ(st.sprites.Count(), 1);
    ASSERT_TRUE(st.sprites[0].density);
    EXPECT_EQ(*st.sprites[0].density, 2);

    auto request = PrototypeRequest(doc, st, "", "/x/", "seed");
    EXPECT_STR_EQ(request.folder, "Generated/UI");
    EXPECT_STR_EQ(request.seed, "seed");
    ASSERT_EQ(request.sprites.Count(), 1);
    UiLayout::UiImage img;
    ASSERT_TRUE(UiLayout::UiImage::Decode(request.sprites[0].png, img));
    EXPECT_EQ(img.w, (int)JsRound(st.sprites[0].width / 2));
    EXPECT_EQ(img.h, (int)JsRound(st.sprites[0].height / 2));
    ASSERT_TRUE(request.sprites[0].nine);
    EXPECT_EQ(request.sprites[0].nine->left, 20);
    EXPECT_EQ(request.sprites[0].nine->top, 20);

    Json layout;
    ASSERT_TRUE(Json::Parse(request.layout, layout));
    std::function<const Json*(const Json&)> find = [&](const Json& v) -> const Json*
    {
        if (v.IsObject() && v.Get("nineSlice") && v.Get("asset"))
            return &v;
        for (auto& m : v.Members())
            if (auto r = find(m.second)) return r;
        for (auto& i : v.Items())
            if (auto r = find(i)) return r;
        return nullptr;
    };
    const Json* node = find(layout);
    ASSERT_TRUE(node);
    EXPECT_EQ(M(M(*node, "nineSlice"), "left").Num(), 20);
    EXPECT_EQ(M(M(*node, "nineSlice"), "bottom").Num(), 20);
}

// A style in another bundled font: the font file and its family's licence go into the prototype's Fonts with metas, and
// the label is set to it; a font outside the bundled set is still drawn with Roboto, said in a warning
TEST(UiDocExport, FontsBeyondRobotoAreShippedWithTheirLicencesAndUsed)
{
    EXPECT_STR_EQ(UiLayout::BundledFontOf("fonts/lobster-regular.TTF"), "Lobster-Regular.ttf");
    EXPECT_STR_EQ(UiLayout::BundledFontOf("FiraSans-Black.ttf"), "FiraSans-Black.ttf");
    EXPECT_STR_EQ(UiLayout::BundledFontOf("Cairo-Black 1.ttf"), "");

    WorkDir work;
    UiLayoutExportRequest request;
    request.doc = StyledDoc();
    request.kit = StyledKit();
    request.nodeId = "node_fonts";
    request.assetsPath = work.path;
    auto res = RunExport(request);
    ASSERT_TRUE(res.ok) << res.error.Data();

    String fonts = work.path + "Generated/UI/M_Resources/Fonts/";
    EXPECT_GT(PipelineUtils::ReadFileBytes(fonts + "Lobster-Regular.ttf").Length(), 10000);
    EXPECT_GT(PipelineUtils::ReadFileBytes(fonts + "LICENSE-Lobster.txt").Length(), 100);
    EXPECT_STR_EQ(S(ParsedJson(PipelineUtils::ReadFileBytes(fonts + "Lobster-Regular.ttf.meta")), "Type"), "o2::VectorFontAsset::Meta");
    EXPECT_FALSE(o2FileSystem.IsFileExist(fonts + "Roboto-Regular.ttf")) << "only the fonts the labels use";
    for (auto& w : res.warnings)
        EXPECT_FALSE(w.Contains("drawn with") || w.Contains("missing")) << w.Data();

    // The label is set to the font the prototype ships
    auto st = ToUiStructure(request.doc, &*request.kit);
    auto proto = PrototypeRequest(request.doc, st, "", work.path, "node_fonts");
    UiLayout::UiWidgetPlan plan;
    String error;
    ASSERT_TRUE(UiLayout::PlanWidgets(ParsedJson(proto.layout), UiLayout::SpriteInfos(proto.sprites), plan, &error)) << error.Data();
    auto title = PlanNode(plan, "title");
    ASSERT_TRUE(title);
    EXPECT_STR_EQ(title->fontFile, "Lobster-Regular.ttf");
    RecordingResources resources;
    UiLayout::BuildWidgets(plan, resources, UiLayout::UiWidgetOptions());
    EXPECT_EQ(resources.fonts, Vector<String>({ "Lobster-Regular.ttf" }));

    // A font the bundled set does not have
    Json unknown = ParsedJson(R"({"canvas":{"name":"S","width":200,"height":100},"root":{"name":"S","type":"container",
        "position":{"x":0,"y":0},"size":{"width":200,"height":100},"children":[{"name":"t","type":"text","text":"Hi",
        "fontFamily":"Cairo-Black 1.ttf","fontSize":20,"position":{"x":0,"y":0},"size":{"width":200,"height":100}}]}})");
    ASSERT_TRUE(UiLayout::PlanWidgets(unknown, {}, plan, &error)) << error.Data();
    EXPECT_STR_EQ(PlanNode(plan, "t")->fontFile, "Roboto-Bold.ttf");
    EXPECT_TRUE(plan.warnings.Contains("root/t: font Cairo-Black 1.ttf drawn with Roboto-Bold.ttf")) << plan.warnings.Count();
}

// The style's gradient fill, outline, drop shadow and glow become the effects of one font style the label uses; an inner
// shadow, which o2's font styles cannot draw, is a warning. The structure the web compares stays without them
TEST(UiDocExport, GradientOutlineShadowAndGlowLandInTheFontStyle)
{
    UiDoc doc = StyledDoc();
    UiKit kit = StyledKit();
    auto st = ToUiStructure(doc, &kit);
    EXPECT_FALSE(st.json.Contains("textStyle")) << "ui_structure.json stays the web's";
    Json look;
    ASSERT_TRUE(st.textStyles.TryGetValue("title", look));
    EXPECT_EQ(M(look, "effects").Count(), 4);

    auto proto = PrototypeRequest(doc, st, "", "/x/", "seed");
    UiLayout::UiWidgetPlan plan;
    String error;
    ASSERT_TRUE(UiLayout::PlanWidgets(ParsedJson(proto.layout), {}, plan, &error)) << error.Data();
    auto title = PlanNode(plan, "title");
    ASSERT_TRUE(title && title->look);
    EXPECT_STR_EQ(title->look->fill, "#FFF0A0");
    EXPECT_STR_EQ(title->look->fillTo, "#FF8000");
    EXPECT_EQ(title->look->angle, 90);
    ASSERT_EQ(title->look->effects.Count(), 3);
    EXPECT_STR_EQ(title->look->effects[0].type, "stroke");
    EXPECT_STR_EQ(title->look->effects[1].type, "shadow");
    EXPECT_GT(title->look->effects[1].y, 0);
    EXPECT_GT(title->look->effects[1].blur, 0);
    EXPECT_STR_EQ(title->look->effects[2].type, "shadow");
    EXPECT_EQ(title->look->effects[2].x, 0);
    EXPECT_EQ(title->look->effects[2].y, 0);
    EXPECT_GT(title->look->effects[2].blur, 0) << "a glow is a blurred shadow without an offset";
    EXPECT_TRUE(plan.warnings.Contains("root/title: an inner shadow is not drawn: o2's font styles draw outlines and shadows outside "
                                       "the letters"));

    RecordingResources resources;
    UiLayout::BuildWidgets(plan, resources, UiLayout::UiWidgetOptions());
    ASSERT_EQ(resources.looks.Count(), 1);
    EXPECT_TRUE(resources.looks[0] == *title->look);

    // Written into the assets: the font style file with its meta
    WorkDir work;
    UiLayoutExportRequest request;
    request.doc = doc;
    request.kit = kit;
    request.nodeId = "node_looks";
    request.assetsPath = work.path;
    auto res = RunExport(request);
    ASSERT_TRUE(res.ok) << res.error.Data();
    String file = UiLayout::FontStyleFileOf(*title->look);
    EXPECT_TRUE(file.StartsWith("text_") && file.EndsWith(".fntstyle")) << file.Data();
    String rel = "Generated/UI/M_Resources/FontStyles/" + file;
    EXPECT_TRUE(res.written.Contains(rel));
    EXPECT_STR_EQ(S(ParsedJson(PipelineUtils::ReadFileBytes(work.path + rel + ".meta")), "Type"), "o2::DefaultAssetMeta<o2::FontStyleAsset>");

    Json style = ParsedJson(PipelineUtils::ReadFileBytes(work.path + rel));
    auto& effects = M(style, "mEffects").Items();
    ASSERT_EQ(effects.Count(), 4);
    EXPECT_STR_EQ(S(effects[0], "Type"), "o2::FontGradientEffect");
    const Json& gradient = M(effects[0], "Value");
    EXPECT_STR_EQ(ColorHex(M(gradient, "color1")), "FF8000FF") << "rows run bottom-up: the bottom colour first";
    EXPECT_STR_EQ(ColorHex(M(gradient, "color2")), "FFF0A0FF");
    EXPECT_EQ(M(gradient, "angle").Num(), 0);
    EXPECT_STR_EQ(S(effects[1], "Type"), "o2::FontStrokeEffect");
    EXPECT_STR_EQ(ColorHex(M(M(effects[1], "Value"), "color")), "402000FF");
    EXPECT_GE(M(M(effects[1], "Value"), "radius").Num(), 1);
    EXPECT_STR_EQ(S(effects[2], "Type"), "o2::FontShadowEffect");
    EXPECT_STR_EQ(ColorHex(M(M(effects[2], "Value"), "color")), "00000080");
    EXPECT_EQ(M(M(M(effects[2], "Value"), "offset"), "x").Num(), 0);
    EXPECT_GT(M(M(M(effects[2], "Value"), "offset"), "y").Num(), 0) << "down the screen";
    EXPECT_STR_EQ(S(effects[3], "Type"), "o2::FontShadowEffect");
    EXPECT_STR_EQ(ColorHex(M(M(effects[3], "Value"), "color")), "FFFFFFCC");
    EXPECT_EQ(M(M(M(effects[3], "Value"), "offset"), "y").Num(), 0);

    // An outline alone keeps its readable name; other gradient directions go to the nearest axis
    UiLayout::UiTextLook outline;
    outline.fill = "#FFFFFF";
    UiLayout::UiTextEffect stroke;
    stroke.type = "stroke";
    stroke.color = "#000000";
    stroke.width = 2.4;
    outline.effects.Add(stroke);
    EXPECT_STR_EQ(UiLayout::FontStyleFileOf(outline), "stroke_ffffff_000000_2.fntstyle");
    UiLayout::UiTextLook across;
    across.fill = "#FF0000";
    across.fillTo = "#0000FF";
    across.angle = 10;
    auto acrossStyle = UiLayout::FontStyleOf(across);
    auto acrossGradient = DynamicCast<FontGradientEffect>(acrossStyle->GetEffects()[0]);
    ASSERT_TRUE(acrossGradient);
    EXPECT_EQ(acrossGradient->angle, -90.0f) << "left to right";
    EXPECT_EQ(acrossGradient->color1, Color4(255, 0, 0, 255));
}

// A shadow is drawn in its own colour from the glyph's coverage: a fill before it does not tint it
TEST(UiDocExport, AShadowKeepsItsColourAfterAFill)
{
    Bitmap glyph(PixelFormat::R8G8B8A8, Vec2I(16, 16));
    glyph.Fill(Color4(255, 255, 255, 0));
    UInt8* data = glyph.GetData();
    for (int y = 6; y < 10; y++)
    {
        for (int x = 2; x < 6; x++)
        {
            UInt8* p = data + (y*16 + x)*4;
            p[0] = p[1] = p[2] = p[3] = 255;
        }
    }

    FontColorEffect(Color4(255, 0, 0, 255)).Process(glyph);
    FontShadowEffect(0.5f, Vec2I(6, 0), Color4(0, 0, 255, 255)).Process(glyph);
    const UInt8* inside = data + (8*16 + 3)*4;
    const UInt8* shadow = data + (8*16 + 10)*4;
    EXPECT_EQ(inside[0], 255) << "the glyph stays red";
    EXPECT_EQ(inside[2], 0);
    EXPECT_EQ(shadow[0], 0) << "the shadow is blue, not red times blue";
    EXPECT_EQ(shadow[2], 255);
    EXPECT_EQ(shadow[3], 255);
}
