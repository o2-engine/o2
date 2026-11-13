#include "o2Editor/stdafx.h"

#include <tuple>

#include "UiDocTestKit.h"

#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"

#include "UiDocGolden2.inc"

// Round 2 of the golden checks (AssetsLine after 77d5b09): the web's outputs for the same inputs, made by
// UiDocGolden/cases2.mts
namespace
{
    using namespace UiDocTest;

    const Json& Golden2()
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGolden2Parts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        return golden;
    }

    const Json& G2(const char* key)
    {
        static const Json none;
        const Json* v = Golden2().Get(key);
        return v ? *v : none;
    }

    // A JSON value with every object's members sorted by key: for documents whose key order is not the point
    Json Canon(const Json& v)
    {
        if (v.IsArray())
        {
            Json out = Json::Array();
            for (auto& item : v.Items())
                out.Add(Canon(item));
            return out;
        }
        if (!v.IsObject())
            return v;

        Vector<String> keys;
        for (auto& m : v.Members())
            keys.Add(m.first);
        std::sort(keys.begin(), keys.end(), [](const String& a, const String& b) { return (std::string)a < (std::string)b; });
        Json out = Json::Object();
        for (auto& k : keys)
            out[k] = Canon(*v.Get(k));
        return out;
    }

    Json ParseJson(const String& text)
    {
        Json out;
        Json::Parse(text, out);
        return out;
    }

    Vector<TextStyle> GoldenStyles()
    {
        Vector<TextStyle> out;
        for (auto& c : G2("styles").Items())
            out.Add(NormalizeTextStyle(M(c, "raw"), "fb"));
        return out;
    }

    Json StylesJson(const Vector<TextStyle>& styles)
    {
        Json out = Json::Array();
        for (auto& s : styles)
            out.Add(TextStyleToJson(s));
        return out;
    }
}

TEST(UiDocGolden2, TextStylesWithFillAndEffects)
{
    auto& cases = G2("styles");
    ASSERT_GT(cases.Count(), 10);
    for (auto& c : cases.Items())
        EXPECT_STR_EQ(TextStyleToJson(NormalizeTextStyle(M(c, "raw"), "fb")).Dump(), S(c, "style")) << M(c, "raw").Dump().Data();

    auto styles = GoldenStyles();
    for (int i = 0; i < styles.Count(); i++)
    {
        TextReach r = EffectsReach(styles[i]);
        auto& want = *G2("reach").At(i);
        EXPECT_EQ(r.left, M(want, "left").Num());
        EXPECT_EQ(r.top, M(want, "top").Num());
        EXPECT_EQ(r.right, M(want, "right").Num());
        EXPECT_EQ(r.bottom, M(want, "bottom").Num());
        EXPECT_STR_EQ(StyleLook(styles[i]), G2("looks").At(i)->Str()) << i;
    }

    int k = 0;
    for (auto& s : styles)
    {
        for (double f : { 0.5, 1.0004, 2.731, 0.0, -1.0 })
            EXPECT_STR_EQ(TextStyleToJson(ScaleTextStyle(s, f)).Dump(), G2("scaled").At(k++)->Str()) << s.name.Data() << " x" << f;
    }
}

TEST(UiDocGolden2, MergedStylesAndNewEffects)
{
    auto styles = GoldenStyles();
    Vector<TextStyle> curs = { styles[0], styles[8], TextStyleDefault() };
    Vector<Json> merges;
    {
        const char* raws[] = {
            R"({"color":"#ff0000"})", R"({"gradient":"#00ff00"})", R"({"stroke":{"color":"#fff","width":4}})", R"({"stroke":null})",
            R"({"shadow":{"x":1,"y":2}})", R"({"shadow":null})", R"({"effects":[{"type":"glow","size":5}]})", R"({"fill":null})",
            R"({"fill":{"stops":["#000","#fff"]}})", R"({"name":"renamed","size":50,"font":"Bangers"})", R"({"stroke":{"width":0},"color":"#00f"})",
            R"({"stroke":{"type":"glow","width":3}})",
        };
        for (auto r : raws)
            merges.Add(ParseJson(r));
    }
    int k = 0;
    for (auto& cur : curs)
    {
        for (auto& m : merges)
            EXPECT_STR_EQ(TextStyleToJson(MergeTextStyle(cur, m)).Dump(), G2("merges").At(k++)->Str()) << cur.name.Data() << " " << m.Dump().Data();
    }

    TextStyle small;
    small.size = 10;
    Vector<TextStyle> bases = { TextStyleDefault(), styles[0], small };
    k = 0;
    for (auto& s : bases)
    {
        for (auto& t : TextEffectTypes())
            EXPECT_STR_EQ(TextEffectToJson(DefaultEffect(t, s)).Dump(), G2("defaults").At(k++)->Dump()) << t.Data();
    }
}

TEST(UiDocGolden2, StylePromptsAndExtractedAnswers)
{
    auto spec = StyleLookSpec("px at that size");
    ASSERT_EQ(spec.Count(), G2("lookSpec").Count());
    for (int i = 0; i < spec.Count(); i++)
        EXPECT_STR_EQ(spec[i], G2("lookSpec").At(i)->Str());

    auto styles = GoldenStyles();
    EXPECT_STR_EQ(BuildStylePrompt(styles[0], " gold please ", 0), G2("stylePrompt").At(0)->Str());
    EXPECT_STR_EQ(BuildStylePrompt(styles[8], "x", 3), G2("stylePrompt").At(1)->Str());

    StyleExtractRequest reqs[4];
    reqs[0].width = 1024; reqs[0].height = 576;
    reqs[1].width = 300; reqs[1].height = 120; reqs[1].hint = "  the title  "; reqs[1].matte = "#808080";
    reqs[2].width = 400; reqs[2].height = 100; reqs[2].fullWidth = 1600;
    reqs[3].width = 400; reqs[3].height = 100; reqs[3].fullWidth = 405;
    for (int i = 0; i < 4; i++)
        EXPECT_STR_EQ(BuildStyleExtractPrompt(reqs[i]), G2("extractPrompts").At(i)->Str()) << i;

    for (auto& c : G2("extractAnswers").Items())
    {
        auto& over = M(c, "over");
        Json out;
        String kind = ParseStyleExtractAnswer(S(c, "a"), M(c, "w").Num(), out, JsStringOr(over.Get("name")), JsStringOr(over.Get("role")));
        auto& want = M(c, "r");
        if (want.IsNull())
            EXPECT_TRUE(kind.IsEmpty()) << S(c, "a").Data();
        else if (want.Str() == "none")
            EXPECT_STR_EQ(kind, "none");
        else
            EXPECT_STR_EQ(out.Dump(), want.Str()) << S(c, "a").Data() << " " << over.Dump().Data();
    }

    // Overrides over an extracted style and over a plain one
    Json ex;
    ParseStyleExtractAnswer(S(*G2("extractAnswers").At(0), "a"), 600, ex);
    Json plain = TextStyleToJson(styles[0]);
    EXPECT_STR_EQ(ApplyStyleOverrides(ex, "", "").Dump(), G2("overrides").At(0)->Str());
    EXPECT_STR_EQ(ApplyStyleOverrides(ex, "other one", "caption").Dump(), G2("overrides").At(1)->Str());
    EXPECT_STR_EQ(ApplyStyleOverrides(ex, "!!!", "").Dump(), G2("overrides").At(2)->Str());
    EXPECT_STR_EQ(ApplyStyleOverrides(plain, "", "title").Dump(), G2("overrides").At(3)->Str());

    const char* objects[] = { "x ```JSON\n{\"a\":1}\n``` y", "{\"a\": {\"b\": 2}} tail }", "[1]", "{bad}", "none" };
    for (int i = 0; i < 5; i++)
    {
        Json o;
        bool ok = AnswerObject(objects[i], o);
        EXPECT_STR_EQ(ok ? o.Dump() : String("null"), G2("answerObjects").At(i)->Dump()) << objects[i];
    }

    Vector<TextStyle> samples = { styles[0], styles[3], TextStyleDefault(), NormalizeTextStyle(ParseJson(R"({"size":300,"effects":[{"type":"glow","size":40}]})")) };
    for (int i = 0; i < samples.Count(); i++)
        EXPECT_STR_EQ(DocToJson(SampleDoc(samples[i], "Hello world", "#202020")).Dump(), G2("sample").At(i)->Str()) << i;
}

TEST(UiDocGolden2, TheTargetDevice)
{
    auto& devices = G2("devices");
    ASSERT_EQ(UiDevices().Count(), devices.Count());
    for (int i = 0; i < devices.Count(); i++)
    {
        auto& d = UiDevices()[i];
        auto& w = *devices.At(i);
        EXPECT_STR_EQ(d.id, S(w, "id"));
        EXPECT_STR_EQ(d.name, S(w, "name"));
        EXPECT_STR_EQ(d.kind, S(w, "kind"));
        EXPECT_EQ(d.w, M(w, "w").Num());
        EXPECT_EQ(d.h, M(w, "h").Num());
        EXPECT_EQ(d.dpi, M(w, "dpi").Num());
    }

    for (auto& c : G2("targets").Items())
    {
        const Json& raw = *c.At(0);
        auto t = CleanTarget(IsUndef(raw) ? nullptr : &raw);
        EXPECT_STR_EQ(t ? TargetToJson(*t).Dump() : String("\"__undef__\""), c.At(1)->Dump()) << raw.Dump().Data();
    }

    const char* configs[] = { "{}", R"({"kitDevice":"pixel-8"})", R"({"kitDevice":"custom","kitDeviceCustom":{"w":1000,"h":2000,"dpi":400}})",
                              R"({"kitDevice":"custom"})", R"({"kitDevice":"custom","kitDeviceCustom":null})", R"({"kitDevice":"nope"})",
                              R"({"kitDevice":5})", R"({"kitDevice":"custom","kitDeviceCustom":{"w":10,"h":10,"dpi":400}})" };
    for (int i = 0; i < 8; i++)
        EXPECT_STR_EQ(TargetToJson(TargetOfConfig(ParseJson(configs[i]))).Dump(), G2("targetConfigs").At(i)->Dump()) << configs[i];

    UiScreen screens[] = { { 1179, 2556, 460 }, { 2064, 2752, 264 }, { 1920, 1080, 96 } };
    double sources[][2] = { { 768, 1376 }, { 1080, 1920 }, { 1920, 1080 }, { 500, 500 }, { 0, 100 }, { 33, 7 } };
    int k = 0;
    for (auto& t : screens)
    {
        for (auto& s : sources)
        {
            auto& want = *G2("fits").At(k++);
            EXPECT_EQ(SourceFit(t, s[0], s[1]), want.At(0)->Num());
            auto on = SourceOnTarget(t, s[0], s[1]);
            EXPECT_EQ(on.w, M(*want.At(1), "w").Num());
            EXPECT_EQ(on.h, M(*want.At(1), "h").Num());
            EXPECT_EQ(on.k, M(*want.At(1), "k").Num());
        }
    }

    struct Design { double w, h; std::optional<double> dpi; };
    Design designs[] = { { 768, 1376, std::nullopt }, { 1080, 1920, 480.0 }, { 1179, 2556, 460.0 }, { 750, 1334, 326.0 } };
    k = 0;
    for (auto& t : screens)
    {
        for (auto& d : designs)
        {
            auto& want = *G2("units").At(k++);
            EXPECT_EQ(InTargetUnits(d.dpi, t), want.At(0)->Bool());
            auto u = ToTargetUnits(d.w, d.h, d.dpi, t);
            auto& wu = *want.At(1);
            EXPECT_EQ(u.width, M(wu, "width").Num());
            EXPECT_EQ(u.height, M(wu, "height").Num());
            EXPECT_EQ(u.dpi, M(wu, "dpi").Num());
            EXPECT_EQ(u.scale, M(wu, "scale").Num());
            EXPECT_EQ(u.fromDpi, M(wu, "fromDpi").Num());
        }
    }

    double aspects[][2] = { { 1179, 2556 }, { 1920, 1080 }, { 1080, 1920 }, { 2064, 2752 }, { 1000, 1000 }, { 2560, 1664 }, { 3440, 1440 }, { 1000, 2000 },
                            { 750, 1334 }, { 1170, 2532 } };
    for (int i = 0; i < 10; i++)
        EXPECT_STR_EQ(AspectName(aspects[i][0], aspects[i][1]), G2("aspects").At(i)->Str()) << i;

    Design frames[] = { { 1080, 1920, std::nullopt }, { 1179, 2112, 460.0 }, { 2000, 1000, std::nullopt } };
    k = 0;
    for (auto& d : frames)
    {
        for (auto& dev : UiDevices())
        {
            auto f = PreviewFrame(d.w, d.h, d.dpi, dev.Screen(), "");
            auto& want = *G2("frames").At(k++);
            EXPECT_EQ(f.w, M(want, "w").Num()) << dev.id.Data();
            EXPECT_EQ(f.h, M(want, "h").Num()) << dev.id.Data();
            EXPECT_EQ(f.scale, M(want, "scale").Num()) << dev.id.Data();
        }
    }
}

TEST(UiDocGolden2, KitPurposesDeviceFieldsAndTheMerge)
{
    auto& kitCase = G2("kit");
    UiKit kit = ParseKit(S(kitCase, "json"));
    EXPECT_STR_EQ(Canon(KitToJson(kit)).Dump(), Canon(ParseJson(S(kitCase, "parsed"))).Dump());
    EXPECT_STR_EQ(KitSummary(kit), M(kitCase, "summary").At(0)->Str());
    EXPECT_STR_EQ(KitSummary(kit, 460.0), M(kitCase, "summary").At(1)->Str());
    EXPECT_STR_EQ(Canon(KitToJson(ParseKit(R"({"name":"k","device":{"w":1000,"h":1500,"dpi":300},"elements":[],"textStyles":[]})"))).Dump(),
                  Canon(ParseJson(S(kitCase, "customDevice"))).Dump());
    EXPECT_STR_EQ(Canon(KitToJson(ParseKit(R"({"name":"k","device":{"w":10,"h":10,"dpi":300},"elements":[],"textStyles":[]})"))).Dump(),
                  Canon(ParseJson(S(kitCase, "badDevice"))).Dump());

    for (auto& c : G2("names").Items())
    {
        String id = c.At(0)->Str();
        for (int i = 0; i < kit.elements.Count(); i++)
            EXPECT_EQ(NameScore(id, kit.elements[i]), (int)c.At(1)->At(i)->Num()) << id.Data() << " " << kit.elements[i].name.Data();
    }

    Json purposes[] = { Json(String("  a\n b  ")), Json(String("")), Json(), Json(5.0), Json(String(std::string(300, 'x'))) };
    for (int i = 0; i < 5; i++)
    {
        String p = CleanPurpose(&purposes[i]);
        EXPECT_STR_EQ(p.IsEmpty() ? String("\"__undef__\"") : Json(p).Dump(), G2("purposes").At(i)->Dump()) << i;
    }

    auto& merge = G2("mergeKit");
    Json config = Json::Object();
    config["items"] = M(merge, "cur");
    Json merged = MergeKitAnalysis(config, M(merge, "answer"));
    EXPECT_STR_EQ(merged.Get("items")->Dump(), S(merge, "merged"));
}

TEST(UiDocGolden2, TheScreenOpWithAScaleTheRootByNameAndPoints)
{
    auto& c = G2("opsScale");
    UiDoc doc = ParseUiml(S(c, "src")).doc;
    for (auto& k : M(c, "cases").Items())
    {
        Vector<UiOp> list;
        for (auto& op : M(k, "ops").Items())
            list.Add(op);
        auto r = ApplyOps(doc, list);
        EXPECT_STR_EQ(DocToJson(r.doc).Dump(), S(k, "doc")) << M(k, "ops").Dump().Data();
        EXPECT_EQ(r.errors, Strings(M(k, "errors"))) << M(k, "ops").Dump().Data();
    }

    auto& pts = G2("points");
    const Json& raw = M(pts, "raw");
    Json notArray(String("x"));
    Json many = Json::Array();
    for (int i = 0; i < 30; i++)
    {
        Json p = Json::Object();
        p["x"] = i;
        p["y"] = i;
        many.Add(p);
    }
    EXPECT_STR_EQ(PointsToJson(CleanPoints(&raw)).Dump(), M(pts, "clean").At(0)->Dump());
    EXPECT_STR_EQ(PointsToJson(CleanPoints(&raw, 1000, 1500)).Dump(), M(pts, "clean").At(1)->Dump());
    EXPECT_STR_EQ(PointsToJson(CleanPoints(&notArray)).Dump(), M(pts, "clean").At(2)->Dump());
    EXPECT_STR_EQ(PointsToJson(CleanPoints(&many)).Dump(), M(pts, "clean").At(3)->Dump());

    const Json& pinPoints = G2("pinsPoints");
    auto points = CleanPoints(&pinPoints);
    for (int i = 0; i < points.Count(); i++)
        points[i].text = S(*pinPoints.At(i), "text");
    for (auto& p : G2("pins").Items())
    {
        String d = S(p, "d");
        Vector<int> named = PinsNamed(d, points.Count());
        Json namedJson = Json::Array();
        for (int n : named)
            namedJson.Add(Json((double)n));
        EXPECT_STR_EQ(namedJson.Dump(), M(p, "named").Dump()) << d.Data();
        EXPECT_STR_EQ(DraftWithNotes(d, points), S(p, "notes")) << d.Data();
        auto req = RequestWithPins(d, points);
        EXPECT_STR_EQ(req.text, S(M(p, "req"), "text")) << d.Data();
        EXPECT_STR_EQ(PointsToJson(req.points).Dump(), M(M(p, "req"), "points").Dump()) << d.Data();
    }
}

TEST(UiDocGolden2, TheLayoutsOwnKitAndItsOutputs)
{
    auto& own = G2("own");
    const Json& config = M(own, "config");
    auto steps = StepsOf(config);
    UiOwnKit ownKit = OwnKitOf(steps);
    Json kitJson = Json::Object();
    Json assets = Json::Array();
    for (auto& a : ownKit.assets)
        assets.Add(OwnAssetToJson(a));
    kitJson["assets"] = assets;
    kitJson["styles"] = StylesJson(ownKit.styles);
    EXPECT_STR_EQ(kitJson.Dump(), S(own, "kit"));

    Json assetsOnly = Json::Array();
    for (auto& a : OwnAssetsOf(steps))
        assetsOnly.Add(OwnAssetToJson(a));
    EXPECT_STR_EQ(assetsOnly.Dump(), S(own, "assets"));
    EXPECT_EQ(OwnUploadIds(steps), Strings(M(own, "uploads")));
    EXPECT_EQ(ConfigUploadIds(config), Strings(M(own, "configUploads")));

    EXPECT_STR_EQ(Canon(KitElementToJson(OwnElement(ownKit.assets[0], "PNG"))).Dump(), Canon(ParseJson(M(own, "element").At(0)->Str())).Dump());
    UiOwnAsset bare;
    bare.name = "n";
    bare.type = "icon";
    bare.assetType = "simple";
    bare.uploadId = "u";
    EXPECT_STR_EQ(Canon(KitElementToJson(OwnElement(bare))).Dump(), Canon(ParseJson(M(own, "element").At(1)->Str())).Dump());

    Json ports = Json::Array();
    for (auto& p : LayoutOwnPorts(config))
    {
        Json j = Json::Object();
        j["name"] = p.name;
        j["asset"] = OwnAssetToJson(p.asset);
        ports.Add(j);
    }
    EXPECT_STR_EQ(ports.Dump(), S(own, "ports"));

    const char* names[] = { "Plate", "My Graphic!", "\xC3\x9Cn\xC3\xAF", "a-b_c" };
    for (int i = 0; i < 4; i++)
        EXPECT_STR_EQ(LayoutOwnPortId(names[i]), M(own, "ids").At(i)->Str());

    // The node's outputs: its schema ones as it has them, then one per graphic (a graphic the node had keeps its port id)
    Json outputs = Json::Array();
    for (auto& [id, name, type] : { std::tuple<String, String, String>{ "image", "image", "image" }, { "layout", "layout", "text" } })
    {
        Json p = Json::Object();
        p["id"] = id;
        p["name"] = name;
        p["type"] = type;
        outputs.Add(p);
    }
    for (auto& p : LayoutOwnOutputs(config, { { "plate", "keep-me" }, { "gone", "gone" } }))
    {
        Json j = Json::Object();
        j["id"] = p.id;
        j["name"] = p.name;
        j["type"] = "image";
        outputs.Add(j);
    }
    EXPECT_STR_EQ(outputs.Dump(), S(own, "outputs"));

    const char* byName[] = { "plate", "PLATE", "image", "image_3", "nope" };
    for (int i = 0; i < 5; i++)
    {
        auto p = LayoutOwnPortByName(config, byName[i]);
        Json got;
        if (p)
        {
            got = Json::Object();
            got["name"] = p->name;
            got["asset"] = OwnAssetToJson(p->asset);
        }
        else
            got = Json(String("__undef__"));
        const Json& want = *M(own, "byName").At(i);
        EXPECT_STR_EQ(got.Dump(), want.IsNull() ? String("\"__undef__\"") : want.Dump()) << byName[i];
    }

    Json clean1 = ParseJson(R"({"name":"x y","uploadId":" id.png ","type":"nope","assetType":"nope","width":0,"height":"x","variant":"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv","dpi":0,"from":"other"})");
    clean1["purpose"] = String(std::string(400, 'p'));
    auto a1 = CleanOwnAsset(clean1);
    ASSERT_TRUE(a1.has_value());
    EXPECT_STR_EQ(OwnAssetToJson(*a1).Dump(), M(own, "clean").At(0)->Dump());
    EXPECT_FALSE(CleanOwnAsset(ParseJson(R"({"name":"!!!","uploadId":"i"})")).has_value());
}

TEST(UiDocGolden2, SeveralKitsAsOne)
{
    UiKit k1 = ParseKit(S(G2("kit"), "json"));
    UiKit k2 = ParseKit(R"({"name":"kit","device":{"w":800,"h":1200,"dpi":200},"elements":[{"name":"btn_main","png":"BBBB"},{"name":"BTN_MAIN","png":"CCCC"}],"textStyles":[{"name":"Title"}]})");
    UiKit k3 = ParseKit(R"({"name":"kit","elements":[{"name":"btn_main_2","png":"DDDD"}],"textStyles":[]})");
    Vector<Vector<const UiKit*>> cases = { { &k1, &k2, &k3 }, { nullptr, &k2 }, { &k3, &k1 }, {}, { nullptr } };
    for (int i = 0; i < cases.Count(); i++)
    {
        auto merged = MergeKits(cases[i]);
        const Json& want = *G2("mergeKits").At(i);
        if (!merged)
            EXPECT_STR_EQ(want.Str(), "null") << i;
        else
            EXPECT_STR_EQ(Canon(KitToJson(*merged)).Dump(), Canon(ParseJson(want.Str())).Dump()) << i;
    }

    EXPECT_STR_EQ(FreeKitName(&k1, "btn_main"), G2("freeNames").At(0)->Str());
    EXPECT_STR_EQ(FreeKitName(&k1, "  new graphic "), G2("freeNames").At(1)->Str());
    EXPECT_STR_EQ(FreeKitName(&k1, "icon_coin", { "icon_coin_2" }), G2("freeNames").At(2)->Str());
    EXPECT_STR_EQ(FreeKitName(&k1, "!!!"), G2("freeNames").At(3)->Str());
}

TEST(UiDocGolden2, TheAgentsKitAdditionsAndFlowWords)
{
    UiDoc doc = ParseUiml("screen Menu 1080x1920\n  button play \"PLAY\"").doc;
    for (auto& c : G2("agentAnswers").Items())
    {
        String a = S(c, "a");
        auto r = ParseAgentAnswer(a, doc);
        EXPECT_STR_EQ(r.reply, S(c, "reply")) << a.Data();
        Json ops = Json::Array();
        for (auto& op : r.ops)
            ops.Add(op);
        EXPECT_STR_EQ(ops.Dump(), S(c, "ops")) << a.Data();
        EXPECT_EQ(r.errors, Strings(M(c, "errors"))) << a.Data();
        EXPECT_STR_EQ(StylesJson(r.styles).Dump(), S(c, "styles")) << a.Data();
        Json assets = Json::Array();
        for (auto& x : r.assets)
            assets.Add(AssetRequestToJson(x));
        EXPECT_STR_EQ(assets.Dump(), S(c, "assets")) << a.Data();
        EXPECT_EQ(r.done, M(c, "done").Bool()) << a.Data();
        EXPECT_EQ(r.markup, M(c, "markup").Bool()) << a.Data();
    }

    int i = 0;
    for (auto& c : G2("agentAnswers").Items())
    {
        auto adds = ParseKitAdditions(S(c, "a"));
        Json got = Json::Object();
        got["rest"] = adds.rest;
        got["styles"] = StylesJson(adds.styles);
        Json assets = Json::Array();
        for (auto& x : adds.assets)
            assets.Add(AssetRequestToJson(x));
        got["assets"] = assets;
        got["errors"] = JsonStrings(adds.errors);
        EXPECT_STR_EQ(got.Dump(), G2("kitAdditions").At(i++)->Str());
    }

    EXPECT_STR_EQ(UiKitAdditionsSpec(false), G2("additionsSpec").At(0)->Str());
    EXPECT_STR_EQ(UiKitAdditionsSpec(true), G2("additionsSpec").At(1)->Str());
}

TEST(UiDocGolden2, TheAgentsPromptsOfTurnsPointsAndDrawing)
{
    UiKit kit = ParseKit(S(G2("kit"), "json"));
    UiDoc doc = ParseUiml("screen Menu 1080x1920 dpi=460\n  panel box anchor=center size=600x400\n    button play anchor=center size=200x100 \"PLAY\"\n"
                          "  text title anchor=top-center pos=0,100 size=600x120 \"Hi\"").doc;
    UiDoc empty = ParseUiml("screen Menu 1179x2556 dpi=460").doc;

    UiAgentContext base;
    base.doc = doc;
    base.kit = &kit;
    base.prompt = " make [1] bigger ";
    base.references = { UiAgentReference{ "ref", "image", "", 768, 1376 }, UiAgentReference{ "notes", "text", "hello" },
                        UiAgentReference{ "ref2", "image" } };

    UiAgentRunState run;
    run.turn = 2;
    run.maxTurns = 6;
    run.log = { "turn 1: moved play", "drafted" };
    run.built = true;
    UiAgentCompare compare;
    compare.score = 0.734;
    compare.gridStep = 100;
    compare.regions = { UiAgentCompareRegion{ UiRect(10.4, 20.6, 300.2, 99.5), 0.3, { "play", "box" } },
                        UiAgentCompareRegion{ UiRect(0, 0, 5, 5), 0.1, {} } };
    compare.zoom = true;
    run.compare = compare;
    run.findings = { "icon_coin unused while coin has none" };
    run.canMarkup = true;

    UiAgentContext c0 = base;
    c0.canDraw = true;
    c0.withScreenshot = true;
    c0.withKitSheet = true;
    c0.points = { UiStepPoint{ 540, 960, "" }, UiStepPoint{ 5, 5, "corner note" }, UiStepPoint{ 540, 160, "" } };
    c0.pinned = true;
    c0.lang = "Russian";

    UiAgentContext c1 = base;
    c1.withScreenshot = true;
    c1.run = run;

    UiAgentContext c2 = base;
    UiAgentRunState last;
    last.turn = 6;
    last.maxTurns = 6;
    c2.run = last;
    c2.points = { UiStepPoint{ 1, 1, "" } };

    UiAgentContext c3 = base;
    c3.doc = empty;
    c3.withKitSheet = true;
    UiAgentRunState first = run;
    first.turn = 1;
    first.compare->regions.Clear();
    first.compare->zoom = false;
    c3.run = first;

    UiAgentContext ctxs[] = { c0, c1, c2, c3 };
    for (int i = 0; i < 4; i++)
        EXPECT_STR_EQ(BuildAgentPrompt(ctxs[i]), G2("turnPrompts").At(i)->Str()) << i;

    double points[][2] = { { 540, 960 }, { 5, 5 }, { 540, 160 }, { 2000, 2000 } };
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(BlocksAt(doc, points[i][0], points[i][1]), Strings(*G2("blocksAt").At(i))) << i;
}

TEST(UiDocGolden2, ThePngSourceTag)
{
    auto& c = G2("png");
    String tiny = Editor::PipelineUtils::Base64Decode(S(c, "tiny"));
    String tagged = Editor::PipelinePngMeta::WithDpi(tiny, 460.0, 1.23456, (int)JsRound(768.4), 1376);
    EXPECT_STR_EQ(Editor::PipelineUtils::Base64Encode(tagged), S(c, "tagged"));
    int w = 0, h = 0;
    ASSERT_TRUE(Editor::PipelinePngMeta::ReadSource(tagged, w, h));
    EXPECT_EQ(w, M(M(c, "source"), "w").Num());
    EXPECT_EQ(h, M(M(c, "source"), "h").Num());
    EXPECT_STR_EQ(Editor::PipelineUtils::Base64Encode(Editor::PipelinePngMeta::WithDpi(tiny, 460.0, std::nullopt, 0, 5)), S(c, "noSource"));

    const char* bad[] = { "", "12x", "x12", "0x5", "12x34x5", "1.5x2", " 12x34" };
    for (int i = 0; i < 7; i++)
    {
        String png = Editor::PipelinePngMeta::WriteText(tiny, { Pair<String, String>("AssetsLine:source", bad[i]) });
        bool ok = Editor::PipelinePngMeta::ReadSource(png, w, h);
        EXPECT_EQ(ok, !IsUndef(*M(c, "bad").At(i))) << bad[i];
    }
}

TEST(UiDocGolden2, EveryFenceIsCutFromTheReplyBeforeTheEmptyOnesAreDropped)
{
    UiDoc doc = ParseUiml("screen Menu 1080x1920\n  button play \"PLAY\"").doc;
    auto r = ParseAgentAnswer("First.\n```\n@markup\n```\nThen renamed it.\n```uiml-edit\n@update play text=\"GO\"\n```\nEnd.", doc);
    EXPECT_STR_EQ(r.reply, "First.\n\nThen renamed it.\n\nEnd.");
    EXPECT_TRUE(r.markup);
    EXPECT_EQ(r.ops.Count(), 1);
}
