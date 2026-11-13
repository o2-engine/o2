#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

#include "o2Editor/Pipeline/UiDoc/UiDocFromStructure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutDraft.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"

#include "UiDocGolden3.inc"

// Round 3 of the golden checks: ui_structure.json -> layout (fromStructure.ts), the markup's analysis -> layout (draft.ts) and the
// analysis fields it reads (parse.ts); the web's outputs for the same inputs, made by UiDocGolden/cases3.mts
namespace
{
    using namespace UiDocTest;
    using Analysis = Editor::UiLayout::UiDoc;

    const Json& Golden3()
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGolden3Parts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        return golden;
    }

    const Json& G3(const char* key)
    {
        static const Json none;
        const Json* v = Golden3().Get(key);
        return v ? *v : none;
    }

    Json ParseJson(const String& text)
    {
        Json out;
        Json::Parse(text, out);
        return out;
    }

    // A JSON value with every object's members sorted by key: for values whose key order is not the point
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

    // The web's document as this model writes it (both keep `rotation`)
    String WebDoc(const String& text)
    {
        return ParseJson(text).Dump();
    }

    Json BoxJson(const Editor::UiLayout::Box& b)
    {
        Json out = Json::Object();
        out["x"] = b.x;
        out["y"] = b.y;
        out["w"] = b.w;
        out["h"] = b.h;
        return out;
    }

    // The analysis as JSON.stringify writes the web's (members of unset fields left out)
    Json AnalysisJson(const Analysis& an)
    {
        Json out = Json::Object();
        out["w"] = an.w;
        out["h"] = an.h;
        Json bg = Json::Object();
        bg["kind"] = an.background.kind;
        bg["color"] = an.background.color;
        bg["describe"] = an.background.describe;
        out["background"] = bg;
        Json looks = Json::Object();
        for (auto& l : an.looks)
            looks[l.first] = l.second;
        out["looks"] = looks;

        Json els = Json::Array();
        for (auto& e : an.els)
        {
            Json o = Json::Object();
            auto text = [&](const char* key, const String& v) {
                if (!v.IsEmpty())
                    o[key] = v;
            };
            auto num = [&](const char* key, const auto& v) {
                if (v)
                    o[key] = (double)*v;
            };
            o["id"] = e->id;
            o["parent"] = e->parent;
            o["kind"] = e->kind;
            o["box"] = BoxJson(e->box);
            text("look", e->look);
            text("kit", e->kit);
            text("text", e->text);
            text("color", e->color);
            text("weight", e->weight);
            text("align", e->align);
            text("stroke", e->stroke);
            num("strokeWidth", e->strokeWidth);
            num("fontSize", e->fontSize);
            if (e->ink)
                o["ink"] = BoxJson(*e->ink);
            num("opacity", e->opacity);
            num("rotation", e->rotation);
            num("value", e->value);
            num("rows", e->rows);
            num("cols", e->cols);
            if (e->cell)
            {
                Json cell = Json::Object();
                cell["of"] = e->cell->of;
                cell["row"] = e->cell->row;
                cell["col"] = e->cell->col;
                o["cell"] = cell;
            }
            num("slot", e->slot);
            if (e->rough)
                o["rough"] = BoxJson(*e->rough);
            text("measured", e->measured);
            els.Add(o);
        }
        out["els"] = els;
        return out;
    }

    // A draft case's analysis: the web's parse of the answer, with the font sizes the measurement would give
    Analysis AnalysisOf(const Json& c)
    {
        int w = (int)M(c, "w").Num(), h = (int)M(c, "h").Num();
        Editor::UiLayout::AnalysisFrame frame = Editor::UiLayout::AnalysisFrameOf(S(c, "model"), w, h);
        Analysis an;
        String error;
        EXPECT_TRUE(Editor::UiLayout::ParseAnalysis(S(c, "answer"), w, h, an, &error, &frame)) << error.Data();
        for (auto& e : an.els)
        {
            if (const Json* f = M(c, "fontSizes").Get(e->id))
                e->fontSize = f->Num();
        }
        return an;
    }

    Map<String, String> PairsMap(const Json& pairs)
    {
        Map<String, String> out;
        for (auto& p : pairs.Items())
            out[p.At(0)->Str()] = p.At(1)->Str();
        return out;
    }

    void ExpectBoxes(const Map<String, UiRect>& got, const Json& want, const String& name)
    {
        EXPECT_EQ((int)got.size(), want.Count()) << name.Data();
        for (auto& entry : want.Items())
        {
            String id = entry.At(0)->Str();
            auto it = got.find(id);
            ASSERT_TRUE(it != got.end()) << name.Data() << ": " << id.Data();
            EXPECT_STR_EQ(RectJson(it->second).Dump(), entry.At(1)->Dump()) << name.Data() << ": " << id.Data();
        }
    }
}

TEST(UiDocGolden3, ParsesAndHasEverySection)
{
    for (const char* key : { "kit", "structures", "legends", "elementNodes", "drafts", "anchors" })
        EXPECT_TRUE(Golden3().Has(key)) << key;
    EXPECT_GE(G3("structures").Count(), 13);
    EXPECT_GE(G3("drafts").Count(), 4);
}

TEST(UiDocGolden3, ElementNodeTypes)
{
    for (auto& pair : G3("elementNodes").Items())
        EXPECT_STR_EQ(ElementNodeType(pair.At(0)->Str()), pair.At(1)->Str()) << pair.At(0)->Str().Data();
}

TEST(UiDocGolden3, LegendBoxes)
{
    int k = 0;
    for (auto& c : G3("legends").Items())
        ExpectBoxes(LegendBoxes(M(c, "legend")), M(c, "boxes"), "legend " + JsNum(k++));
}

TEST(UiDocGolden3, StructuresToLayouts)
{
    auto& cases = G3("structures");
    ASSERT_GE(cases.Count(), 13);
    for (auto& c : cases.Items())
    {
        String name = S(c, "name");
        FromStructureInput in;
        in.structure = M(c, "structure");
        if (!M(c, "legend").IsNull())
            in.boxes = LegendBoxes(M(c, "legend"));
        UiKit kit;
        if (M(c, "kit").IsString())
        {
            kit = ParseKit(S(c, "kit"));
            in.kit = &kit;
        }
        in.assetNames = PairsMap(M(c, "assetNames"));
        if (M(c, "dpi").IsNumber())
            in.dpi = M(c, "dpi").Num();

        FromStructureResult r = FromUiStructure(in);
        EXPECT_STR_EQ(DocToJson(r.doc).Dump(), WebDoc(S(c, "doc"))) << name.Data();

        auto& styles = M(c, "styles");
        ASSERT_EQ(r.styles.Count(), styles.Count()) << name.Data();
        for (int i = 0; i < r.styles.Count(); i++)
            EXPECT_STR_EQ(TextStyleToJson(r.styles[i]).Dump(), styles.At(i)->Str()) << name.Data() << " style " << i;

        EXPECT_STR_EQ(JsonStrings(r.warnings).Dump(), M(c, "warnings").Dump()) << name.Data();
        auto& stats = M(c, "stats");
        EXPECT_EQ(r.stats.blocks, (int)M(stats, "blocks").Num()) << name.Data();
        EXPECT_EQ(r.stats.layouts, (int)M(stats, "layouts").Num()) << name.Data();
        EXPECT_EQ(r.stats.layoutsFlattened, (int)M(stats, "layoutsFlattened").Num()) << name.Data();
        EXPECT_EQ(r.stats.buttons, (int)M(stats, "buttons").Num()) << name.Data();
        EXPECT_EQ(r.stats.texts, (int)M(stats, "texts").Num()) << name.Data();
        EXPECT_STR_EQ(JsonStrings(r.stats.assetsMissing).Dump(), M(stats, "assetsMissing").Dump()) << name.Data();
    }
}

TEST(UiDocGolden3, AnalysisFieldsOfTheMarkup)
{
    for (auto& c : G3("drafts").Items())
    {
        Analysis an = AnalysisOf(c);
        EXPECT_STR_EQ(Canon(AnalysisJson(an)).Dump(), Canon(M(c, "analysis")).Dump()) << S(c, "name").Data();
    }
}

TEST(UiDocGolden3, DraftsFromAnalyses)
{
    UiKit kit = ParseKit(G3("kit").Str());
    for (auto& c : G3("drafts").Items())
    {
        String name = S(c, "name");
        Analysis an = AnalysisOf(c);
        Editor::UiLayout::DraftOptions o;
        if (!M(c, "noKit").Bool())
        {
            o.kit = &kit;
            o.styles = kit.textStyles;
        }
        for (auto& s : M(c, "extra").Items())
            o.styles.Add(NormalizeTextStyle(s, "x"));
        o.assetOf = PairsMap(M(c, "assetOf"));
        o.styleOf = PairsMap(M(c, "styleOf"));
        if (M(c, "dpi").IsNumber())
            o.dpi = M(c, "dpi").Num();
        o.name = S(c, "docName");
        o.backgroundKit = S(c, "backgroundKit");
        o.backgroundColor = S(c, "backgroundColor");

        Editor::UiLayout::DraftResult r = Editor::UiLayout::DraftFromAnalysis(an, o);
        EXPECT_STR_EQ(DocToJson(r.doc).Dump(), WebDoc(S(c, "doc"))) << name.Data();
        auto& styles = M(c, "styles");
        ASSERT_EQ(r.styles.Count(), styles.Count()) << name.Data();
        for (int i = 0; i < r.styles.Count(); i++)
            EXPECT_STR_EQ(TextStyleToJson(r.styles[i]).Dump(), styles.At(i)->Str()) << name.Data() << " style " << i;
        ExpectBoxes(r.boxes, M(c, "boxes"), name);
        EXPECT_EQ(r.blocks, (int)M(c, "blocks").Num()) << name.Data();
    }
}

TEST(UiDocGolden3, AnchorsOfDraftBlocks)
{
    for (auto& c : G3("anchors").Items())
    {
        Anchor a = Editor::UiLayout::AnchorFor(RectOf(*c.At(0)), RectOf(*c.At(1)));
        EXPECT_STR_EQ(a.h, S(*c.At(2), "h")) << c.Dump().Data();
        EXPECT_STR_EQ(a.v, S(*c.At(2), "v")) << c.Dump().Data();
    }
}

TEST(UiDocGolden3, TheLayoutModelKeepsRotation)
{
    // Both converters write `rotation` (a tilted element), and the model keeps it cleaned (UiDocRotate.h CleanTurn)
    EXPECT_STR_EQ(NodeToJson(NormalizeNode(ParseJson(R"({"id":"a","type":"image","rotation":372.34})"))).Dump(),
                  R"({"id":"a","type":"image","rotation":12.3})");
}
