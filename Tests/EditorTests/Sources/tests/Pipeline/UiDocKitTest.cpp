#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

using namespace UiDocTest;

namespace
{
    UiKitElement Element(const String& name, const String& type, const String& assetType, double w, double h,
                         const String& description = "")
    {
        UiKitElement e;
        e.name = name;
        e.type = type;
        e.assetType = assetType;
        e.width = w;
        e.height = h;
        e.png = "x";
        if (!description.IsEmpty())
            e.description = description;
        return e;
    }

    Json NineOrUndef(const std::optional<UiNine>& n)
    {
        return n ? NineToJson(*n) : Json(String("__undef__"));
    }

    template<typename T>
    Json OrUndef(const std::optional<T>& v)
    {
        return v ? Json(*v) : Json(String("__undef__"));
    }

    Json FrameJson(const UiFrame& f)
    {
        Json out = Json::Object();
        out["w"] = f.w;
        out["h"] = f.h;
        out["scale"] = f.scale;
        return out;
    }

    std::optional<double> DpiOf(const Json& obj)
    {
        return CleanDpi(obj.Get("dpi"));
    }
}

// --- the web's tests (backend/test/uiLayout.test.ts: kit matching, coverage, text styles) --------------------------

TEST(UiDocKit, IdsPickGraphicsByTheirMeaningfulWordsIconMatchesNothingByItself)
{
    UiKit kit;
    kit.name = "k";
    kit.elements = { Element("icon_avatar", "icon", "simple", 1, 1), Element("icon_map", "icon", "simple", 1, 1),
                     Element("icon_coin", "icon", "simple", 1, 1), Element("icon_gem", "icon", "simple", 1, 1, "premium currency") };
    EXPECT_STR_EQ(KitElementOfType(&kit, { "icon" }, "", "map_icon")->name, "icon_map");
    EXPECT_STR_EQ(KitElementOfType(&kit, { "icon" }, "", "coins_icon")->name, "icon_coin");
    EXPECT_STR_EQ(KitElementOfType(&kit, { "icon" }, "", "currency_icon")->name, "icon_gem");
    EXPECT_EQ(NameScore("settings_icon", kit.elements[0]), 0);

    UiDoc doc = Parse("screen S 1080x1920\n  icon settings_icon size=80x80\n  icon map_icon size=80x80\n  text heart \"\xE2\x99\xA5\"");
    UiKitReport r = KitReport(doc, &kit);
    ASSERT_EQ(r.missing.Count(), 1);
    EXPECT_STR_EQ(r.missing[0].first, "settings_icon");
    EXPECT_EQ(r.unused, Vector<String>({ "icon_avatar", "icon_coin", "icon_gem" }));
    EXPECT_EQ(r.glyphs, Vector<String>({ "heart" }));
}

TEST(UiDocKit, CoverageWhatTheKitSkinsAndWhatStaysLineArt)
{
    UiKit kit;
    kit.elements = { Element("btn_red", "button", "sliced", 60, 40), Element("icon_coin", "icon", "simple", 64, 64) };
    UiKitCoverage cov = KitCoverage(Parse(Screen()), &kit);
    EXPECT_TRUE(cov.missing.Contains("bg"));        // kit=bg_plain names nothing in the kit
    EXPECT_FALSE(cov.missing.Contains("coin"));     // the icon id names the coin
    EXPECT_FALSE(cov.missing.Contains("settings")); // a button takes the kit's button
    EXPECT_TRUE(cov.missing.Contains("s1"));        // no slot graphic
}

TEST(UiDocKit, ATextStyleCleansWhatItIsGiven)
{
    Json raw;
    ASSERT_TRUE(Json::Parse("{\"name\": \"big title\", \"font\": \"nope\", \"weight\": 650, \"size\": 999, \"color\": \"fff\", \"stroke\": {\"width\": 3}}", raw));
    TextStyle st = NormalizeTextStyle(raw);
    EXPECT_STR_EQ(st.name, "big_title");
    EXPECT_STR_EQ(st.font, "Roboto");
    EXPECT_EQ(st.weight, 700);
    EXPECT_EQ(st.size, 400);
    EXPECT_STR_EQ(st.color, "#FFFFFF");
    ASSERT_TRUE(st.stroke.has_value());
    EXPECT_STR_EQ(st.stroke->color, "#000000");
    EXPECT_EQ(st.stroke->width, 3);
}

// --- the web's tests (backend/test/dpi.test.ts, the parts without PNG metadata or rendering) ---------------------------

TEST(UiDocKit, AGraphicsScaleAndDesignSizeScreensMetByDpiOrBySize)
{
    EXPECT_EQ(KitScale(std::nullopt, std::nullopt), 1);
    EXPECT_EQ(KitScale(480.0, 960.0), 0.5);
    EXPECT_EQ(DesignSize(480.0, 400, 120, 960.0), (UiSize{ 200, 60 }));
    // By DPI: a 460-dpi phone of 1179x2556 shows 1230x2667 design pixels
    EXPECT_EQ(PreviewFrame(1080, 1920, 480.0, UiScreen{ 1179, 2556, 460 }, "dpi"), (UiFrame{ 1230, 2667, 460.0/480 }));
    // By size: Unity's match 0.5 - the scale is the geometric mean of the two ratios
    UiFrame fit = PreviewFrame(1080, 1920, 480.0, UiScreen{ 1080, 2340, 416 }, "fit");
    EXPECT_LT(std::fabs(fit.scale - std::sqrt(2340.0/1920)), 1e-9);
    EXPECT_EQ(fit.w, JsRound(1080/fit.scale));
    EXPECT_EQ(fit.h, JsRound(2340/fit.scale));
}

TEST(UiDocKit, UimlKeepsTheScreenDpiScreenSetsItADocWithoutOneIsTheBase)
{
    UiDoc doc = Parse("screen Menu 1179x2556 dpi=460 layout=column\n  button play \"Play\"");
    EXPECT_EQ(*doc.dpi, 460);
    EXPECT_STR_EQ(*doc.root.layout, "column");
    EXPECT_TRUE(StartsWith(DocToUiml(doc), "screen Menu 1179x2556 dpi=460 layout=column"));
    EXPECT_EQ(*Parse(DocToUiml(doc)).dpi, 460);
    EXPECT_FALSE(Parse("screen M 100x100").dpi.has_value());
    auto edits = ParseEdits("@screen dpi=320", doc);
    EXPECT_EQ(*ApplyOps(doc, edits.ops).doc.dpi, 320);
}

TEST(UiDocKit, NineSliceCornersAtTheLayoutsScaleTheAgentAndTheExportersGetTheSameNumbers)
{
    UiKitElement el = Element("btn", "button", "sliced", 200, 120);
    el.nineSlice = UiNine{ 40, 40, 40, 40 };
    el.dpi = 960;
    UiKit kit;
    kit.elements = { el };
    UiDoc doc = Parse("screen M 600x400 dpi=480\n  button b anchor=center size=300x100 kit=btn \"Go\"");
    EXPECT_NE(((std::string)KitSummary(kit, 480.0)).find("btn: button, sliced, 100x60 (picture 200x120 px at 960 dpi) nine-slice corners 20,20,20,20"),
              std::string::npos);
    auto st = ToUiStructure(doc, &kit);
    ASSERT_EQ(st.sprites.Count(), 1);
    EXPECT_STR_EQ(st.sprites[0].file, "btn.png");
    EXPECT_EQ(*st.sprites[0].density, 2);
    EXPECT_EQ(*st.sprites[0].dpi, 960);
    UiDoc flatDoc = doc;
    flatDoc.dpi = 960;
    EXPECT_FALSE(ToUiStructure(flatDoc, &kit).sprites[0].density.has_value());
}

// --- golden ------------------------------------------------------------------------------------------------------

TEST(UiDocKit, GoldenParseKitAndSummaries)
{
    const Json& g = G("kit");
    UiKit kit = GoldenKit();
    const Json& parsed = M(g, "parsed");
    ASSERT_EQ(kit.elements.Count(), parsed.Count());
    for (int i = 0; i < parsed.Count(); i++)
    {
        const UiKitElement& e = kit.elements[i];
        Json row = Json::Array();
        row.Add(Json(e.name)); row.Add(Json(e.type)); row.Add(Json(e.assetType)); row.Add(Json(e.width)); row.Add(Json(e.height));
        row.Add(OrUndef(e.variant)); row.Add(OrUndef(e.description)); row.Add(NineOrUndef(e.nineSlice)); row.Add(OrUndef(e.dpi));
        row.Add(OrUndef(e.dpiFrom)); row.Add(OrUndef(e.port)); row.Add(e.source.IsNull() ? Json(String("__undef__")) : e.source);
        row.Add(Json(e.png));
        EXPECT_STR_EQ(row.Dump(), parsed.At(i)->Dump());
    }
    Json styles = Json::Array();
    for (auto& s : kit.textStyles)
        styles.Add(TextStyleToJson(s));
    EXPECT_STR_EQ(styles.Dump(), S(g, "styles"));
    EXPECT_STR_EQ(kit.name, S(g, "name"));

    const Json& summary = M(g, "summary");
    EXPECT_STR_EQ(KitSummary(kit), summary.At(0)->Str());
    EXPECT_STR_EQ(KitSummary(kit, 480.0), summary.At(1)->Str());
    EXPECT_STR_EQ(KitSummary(kit, 960.0), summary.At(2)->Str());
    EXPECT_STR_EQ(KitSummary(kit, 326.0), summary.At(3)->Str());
    EXPECT_STR_EQ(KitSummary(UiKit()), summary.At(4)->Str());

    const char* bad[] = { "", "nope", "5", "null", "[1]", "{\"name\": 12345}", "{\"name\": null, \"elements\": {}}" };
    for (int i = 0; i < 7; i++)
        EXPECT_STR_EQ(KitToJson(ParseKit(bad[i])).Dump(), M(g, "bad").At(i)->Str()) << bad[i];
}

TEST(UiDocKit, GoldenNameMatchingTypesAndNames)
{
    UiKit kit = GoldenKit();
    for (auto& c : G("names").Items())
    {
        Json scores = Json::Array();
        for (auto& e : kit.elements)
            scores.Add(Json(NameScore(c.At(0)->Str(), e)));
        EXPECT_STR_EQ(scores.Dump(), c.At(1)->Dump()) << c.At(0)->Str().Data();
    }
    for (auto& c : G("ofType").Items())
    {
        const UiKitElement* e = KitElementOfType(&kit, Strings(*c.At(0)), c.At(1)->IsNull() ? String() : c.At(1)->Str(),
                                                 c.At(2)->IsNull() ? String() : c.At(2)->Str());
        EXPECT_STR_EQ(e ? e->name : String("null"), c.At(3)->IsNull() ? String("null") : c.At(3)->Str()) << c.Dump().Data();
    }
    for (auto& c : G("guess").Items())
        EXPECT_STR_EQ(GuessElementType(c.At(0)->Str()), c.At(1)->Str()) << c.At(0)->Str().Data();
    for (auto& c : G("kitNames").Items())
        EXPECT_STR_EQ(CleanKitName(c.At(0), "fb"), c.At(1)->Str()) << c.At(0)->Dump().Data();
    for (auto& c : G("nine").Items())
        EXPECT_STR_EQ(NineOrUndef(NormalizeNine(c.At(0))).Dump(), c.At(1)->Dump()) << c.At(0)->Dump().Data();
}

TEST(UiDocKit, GoldenKitReportSkinsRolesAndStyles)
{
    UiKit kit = GoldenKit();
    UiKit empty;
    empty.name = "k";
    UiDoc doc = Parse(S(Golden(), "reportDoc"));
    const UiKit* kits[] = { &kit, nullptr, &empty };
    for (int i = 0; i < 3; i++)
    {
        const Json& e = *G("report").At(i);
        UiKitReport r = KitReport(doc, kits[i]);
        Json missing = Json::Array();
        for (auto& m : r.missing)
        {
            Json item = Json::Object();
            item["id"] = m.first;
            item["type"] = m.second;
            missing.Add(item);
        }
        Json report = Json::Object();
        report["missing"] = missing;
        report["unused"] = JsonStrings(r.unused);
        report["glyphs"] = JsonStrings(r.glyphs);
        report["covered"] = r.covered;
        report["total"] = r.total;
        EXPECT_STR_EQ(report.Dump(), M(e, "report").Dump()) << i;

        UiKitCoverage c = KitCoverage(doc, kits[i]);
        Json cov = Json::Object();
        cov["skinned"] = c.skinned;
        cov["missing"] = JsonStrings(c.missing);
        cov["total"] = c.total;
        EXPECT_STR_EQ(cov.Dump(), M(e, "coverage").Dump()) << i;
    }

    for (int k = 0; k < 2; k++)
    {
        const Json& rows = *G("skins").At(k);
        int i = 0;
        Walk(doc.root, [&](const UiNode& n, const UiNode*, int) {
            const Json& row = *rows.At(i++);
            UiSkin s = SkinOf(n, kits[k]);
            Json mine = Json::Array();
            mine.Add(Json(n.id));
            mine.Add(s.el ? Json(s.el->name) : Json());
            mine.Add(s.fill ? Json(s.fill->name) : Json());
            mine.Add(s.handle ? Json(s.handle->name) : Json());
            mine.Add(Json(s.explicitKit));
            mine.Add(Json(s.missing));
            mine.Add(Json(RoleOf(n)));
            mine.Add(Json(TextStyleToJson(StyleOf(n, kits[k])).Dump()));
            mine.Add(Json(TextStyleToJson(StyleOf(n, kits[k], &n)).Dump()));
            EXPECT_STR_EQ(mine.Dump(), row.Dump());
        });
        EXPECT_EQ(i, rows.Count());
    }
}

TEST(UiDocKit, GoldenTextStylesWidthsPromptsAndAnswers)
{
    for (auto& c : G("styles").Items())
        EXPECT_STR_EQ(TextStyleToJson(NormalizeTextStyle(M(c, "raw"), "fallback")).Dump(), S(c, "style")) << M(c, "raw").Dump().Data();

    Json config;
    ASSERT_TRUE(Json::Parse("{\"name\": \"  hello world \", \"font\": \"Bangers\", \"size\": 50, \"uiThing\": 1}", config));
    EXPECT_STR_EQ(TextStyleToJson(TextStyleFromConfig(config)).Dump(), G("styleConfig").Str());

    Vector<String> texts = Strings(G("widthTexts"));
    for (auto& c : G("widths").Items())
    {
        const Json& st = M(c, "style");
        for (int i = 0; i < texts.Count(); i++)
        {
            double w = EstimateTextWidth(texts[i], S(st, "font"), M(st, "weight").Num(), M(st, "size").Num(), M(st, "letterSpacing").Num(),
                                         M(st, "uppercase").Bool());
            EXPECT_EQ(w, M(c, "w").At(i)->Num()) << texts[i].Data() << " in " << st.Dump().Data();
        }
    }

    Json curRaw;
    ASSERT_TRUE(Json::Parse(S(G("stylePrompt"), "cur"), curRaw));
    TextStyle cur = NormalizeTextStyle(curRaw, "cur");
    EXPECT_STR_EQ(TextStyleToJson(cur).Dump(), S(G("stylePrompt"), "cur"));
    EXPECT_STR_EQ(BuildStylePrompt(cur, "  make it bold \xE2\x80\x94 please  ", 0), M(G("stylePrompt"), "prompts").At(0)->Str());
    EXPECT_STR_EQ(BuildStylePrompt(TextStyleDefault(), "x", 2), M(G("stylePrompt"), "prompts").At(1)->Str());

    for (auto& c : G("styleAnswers").Items())
    {
        TextStyle out;
        bool ok = ParseStyleAnswer(S(c, "a"), cur, out);
        EXPECT_STR_EQ(ok ? TextStyleToJson(out).Dump() : String("null"), S(c, "s")) << S(c, "a").Data();
    }

    Vector<TextStyle> styles = { cur, TextStyleDefault(), TextStyleDefault(), cur };
    styles[2].weight = 700;
    EXPECT_EQ(FontFilesOf(styles), Strings(G("fontFiles")));
}

TEST(UiDocKit, GoldenLayoutTextFitsLabels)
{
    for (auto& c : G("layoutText").Items())
    {
        Json raw;
        ASSERT_TRUE(Json::Parse(S(c, "style"), raw));
        TextStyle style = NormalizeTextStyle(raw, S(raw, "name"));
        UiTextFit fit = LayoutText(S(c, "t"), M(c, "w").Num(), M(c, "h").Num(), style, M(c, "size").IsNull() ? 0 : M(c, "size").Num(),
                                   M(c, "pad").Num());
        Json r = Json::Object();
        r["lines"] = JsonStrings(fit.lines);
        r["size"] = fit.size;
        r["lineH"] = fit.lineH;
        EXPECT_STR_EQ(r.Dump(), M(c, "r").Dump()) << c.Dump().Data();
    }
}

TEST(UiDocKit, GoldenDevicesAndDensities)
{
    const Json& g = G("device");
    for (auto& c : M(g, "cleanDpi").Items())
        EXPECT_STR_EQ(OrUndef(CleanDpi(c.At(0))).Dump(), c.At(1)->Dump()) << c.At(0)->Dump().Data();
    for (auto& c : M(g, "names").Items())
        EXPECT_STR_EQ(DensityName(c.At(0)->Num()), c.At(1)->Str());

    const Json& frames = M(g, "frames");
    ASSERT_EQ(UiDevices().Count(), frames.Count());
    for (int i = 0; i < frames.Count(); i++)
    {
        const UiDevice& d = UiDevices()[i];
        const Json& c = *frames.At(i);
        EXPECT_STR_EQ(d.id, c.At(0)->Str());
        EXPECT_STR_EQ(FrameJson(PreviewFrame(1080, 1920, 480.0, d.Screen(), "dpi")).Dump(), c.At(1)->Dump());
        EXPECT_STR_EQ(FrameJson(PreviewFrame(1080, 1920, std::nullopt, d.Screen(), "fit")).Dump(), c.At(2)->Dump());
        EXPECT_STR_EQ(FrameJson(PreviewFrame(750, 1334, 326.0, d.Screen(), "dpi")).Dump(), c.At(3)->Dump());
    }

    for (auto& c : M(g, "scale").Items())
    {
        auto docDpi = DpiOf(*c.At(0)), elDpi = DpiOf(*c.At(1));
        EXPECT_EQ(KitScale(docDpi, elDpi), c.At(2)->Num()) << c.Dump().Data();
        UiSize s = DesignSize(docDpi, 401, 121, elDpi);
        EXPECT_EQ(s.w, M(*c.At(3), "w").Num());
        EXPECT_EQ(s.h, M(*c.At(3), "h").Num());
    }
}
