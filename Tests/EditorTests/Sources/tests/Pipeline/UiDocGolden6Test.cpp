#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

#include "o2Editor/Pipeline/UiDoc/UiStyleCheck.h"

#include "UiDocGolden6.inc"

// Round 6 of the golden checks (AssetsLine ea79bbc): a style's effects as a layer stack, the gradient outline and
// ui_structure.json's text extras and fit, made by UiDocGolden/cases6.mts
namespace
{
    using namespace UiDocTest;

    const Json& Golden6()
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGolden6Parts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        return golden;
    }

    const Json& G6(const char* key)
    {
        static const Json none;
        const Json* v = Golden6().Get(key);
        return v ? *v : none;
    }

    Json ParseJson(const String& text)
    {
        Json out;
        Json::Parse(text, out);
        return out;
    }

    Json EffectsJson(const Vector<TextEffect>& effects)
    {
        Json out = Json::Array();
        for (auto& e : effects)
            out.Add(TextEffectToJson(e));
        return out;
    }

    Vector<TextStyle> GoldenStyles()
    {
        Vector<TextStyle> out;
        for (auto& c : G6("styles").Items())
            out.Add(NormalizeTextStyle(M(c, "raw"), "fb"));
        return out;
    }
}

TEST(UiDocGolden6, TheCanonicalStackAndWhereANewEffectGoes)
{
    auto& stack = G6("stack");
    ASSERT_GT(stack.Count(), 10);
    for (int i = 0; i < stack.Count(); i++)
    {
        auto& c = *stack.At(i);
        Json list = ParseJson(S(c, "list"));
        auto fx = NormalizeEffects(&list);
        EXPECT_STR_EQ(EffectsJson(StackEffects(fx)).Dump(), S(c, "stacked")) << S(c, "list").Data();
        for (int n = 0; n < fx.Count(); n++)
            EXPECT_EQ(IsInnerEffect(fx[n]), M(c, "inner").At(n)->Bool());

        auto& inserts = *G6("insert").At(i);
        int k = 0;
        for (auto& type : TextEffectTypes())
        {
            auto added = DefaultEffect(type, TextStyleDefault());
            added.color = "#ABCDEF";
            EXPECT_STR_EQ(EffectsJson(InsertEffect(fx, added)).Dump(), inserts.At(k++)->Str()) << S(c, "list").Data() << " + " << type.Data();
        }
    }
}

TEST(UiDocGolden6, StylesKeepTheirStackOnlyWithFxStackAndTakeGradientOutlines)
{
    auto& cases = G6("styles");
    for (auto& c : cases.Items())
        EXPECT_STR_EQ(TextStyleToJson(NormalizeTextStyle(M(c, "raw"), "fb")).Dump(), S(c, "style")) << M(c, "raw").Dump().Data();

    auto styles = GoldenStyles();
    int k = 0;
    for (int i = 0; i < styles.Count(); i++)
    {
        EXPECT_STR_EQ(StyleLook(styles[i]), G6("looks").At(i)->Str()) << i;
        TextReach r = EffectsReach(styles[i]);
        auto& want = *G6("reach").At(i);
        EXPECT_EQ(r.left, M(want, "left").Num());
        EXPECT_EQ(r.bottom, M(want, "bottom").Num());
        for (double f : { 0.5, 2.731 })
            EXPECT_STR_EQ(TextStyleToJson(ScaleTextStyle(styles[i], f)).Dump(), G6("scaled").At(k++)->Str()) << i << " x" << f;
    }

    for (auto& c : G6("merges").Items())
    {
        const TextStyle& cur = styles[(int)M(c, "cur").Num()];
        EXPECT_STR_EQ(TextStyleToJson(MergeTextStyle(cur, M(c, "raw"))).Dump(), S(c, "style")) << cur.name.Data() << " " << M(c, "raw").Dump().Data();
    }

    for (auto& c : G6("configs").Items())
        EXPECT_STR_EQ(TextStyleToJson(TextStyleFromConfig(M(c, "config"))).Dump(), S(c, "style")) << M(c, "config").Dump().Data();
}

TEST(UiDocGolden6, TheModelIsShownTheStackWithoutFxStackAndTheCheckNamesGradientOutlines)
{
    auto styles = GoldenStyles();
    ASSERT_TRUE(styles[8].fxStack);
    EXPECT_STR_EQ(BuildStylePrompt(styles[8], "more gold", 0), G6("prompt").Str());

    std::pair<int, int> pairs[] = { { 0, 5 }, { 5, 8 }, { 1, 0 } };
    for (int i = 0; i < 3; i++)
    {
        auto changes = StyleChanges(styles[pairs[i].first], styles[pairs[i].second]);
        EXPECT_EQ(changes, Strings(*G6("changes").At(i))) << i;
    }
}

TEST(UiDocGolden6, TheStructureCarriesTheTextLookAndHowPicturesFit)
{
    auto& c = G6("structure");
    UiKit kit = ParseKit(S(c, "kit"));
    UiDoc doc = ParseUiml(S(c, "src")).doc;
    auto st = ToUiStructure(doc, &kit);
    EXPECT_STR_EQ(st.structure.Dump(), S(c, "json"));
    EXPECT_EQ(st.warnings, Strings(M(c, "warnings")));
}
