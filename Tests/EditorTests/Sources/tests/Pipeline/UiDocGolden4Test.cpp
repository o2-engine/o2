#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgentRun.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutCompare.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMarkup.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutSkillBuild.h"

#include "UiDocGolden4.inc"

// Round 4 of the golden checks: the layout agent's run - the markup's kit rules, styles prompt and styles answer, the skill's
// first draft and task note, the check of an edit, the measuring grid and the blocks of a place; the web's outputs for the same
// inputs, made by UiDocGolden/cases4.mts
namespace
{
    using namespace UiDocTest;
    namespace L = Editor::UiLayout;

    const Json& G4(const char* key)
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGolden4Parts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        static const Json none;
        const Json* v = golden.Get(key);
        return v ? *v : none;
    }

    // The kit of the cases: its own graphic marked as the layout node marks it
    const UiKit& Kit()
    {
        static const UiKit kit = []() {
            UiKit k = ParseKit(G4("kit").Str());
            for (auto& e : k.elements)
            {
                if (e.name == "cut_banner")
                    e.own = true;
            }
            return k;
        }();
        return kit;
    }

    UiDoc DocOf(const String& uiml)
    {
        return ParseUiml(uiml).doc;
    }

    L::Box BoxOf(const Json& b)
    {
        return L::Box(b.Get("x")->Num(), b.Get("y")->Num(), b.Get("w")->Num(), b.Get("h")->Num());
    }
}

TEST(UiDocGolden4, MarkupKitRulesAreTheWebs)
{
    ASSERT_EQ(G4("kitRules").Count(), 2);
    for (auto& c : G4("kitRules").Items())
        EXPECT_STR_EQ(L::KitMarkupRules(Kit(), c.Get("dpi")->Num(), (int)c.Get("sheet")->Num()), c.Get("text")->Str());
}

TEST(UiDocGolden4, MarkupStylesPromptIsTheWebs)
{
    UiKit empty = ParseKit(R"({"name":"empty","elements":[],"textStyles":[]})");
    int i = 0;
    for (auto& c : G4("stylesPrompt").Items())
    {
        Vector<L::MarkupLabel> labels;
        for (auto& l : c.Get("labels")->Items())
            labels.Add({ l.Get("text")->Str(), BoxOf(*l.Get("box")) });
        const UiKit* kit = c.Get("kit")->Bool() ? &Kit() : i == 2 ? &empty : nullptr;
        EXPECT_STR_EQ(L::MarkupStylesPrompt(labels, kit, (int)c.Get("w")->Num(), (int)c.Get("h")->Num()), c.Get("text")->Str()) << "case " << i;
        i++;
    }
}

TEST(UiDocGolden4, MarkupStylesAnswersReadAsTheWebReadsThem)
{
    int i = 0;
    for (auto& c : G4("parseStyles").Items())
    {
        L::MarkupStyles got = L::ParseMarkupStyles(c.Get("answer")->Str(), c.Get("kit")->Bool() ? &Kit() : nullptr, 5);
        Json styles = Json::Array();
        for (auto& s : got.styles)
            styles.Add(TextStyleToJson(s));
        EXPECT_STR_EQ(styles.Dump(), c.Get("styles")->Str()) << "case " << i;
        Json byLabel = Json::Array();
        for (auto& s : got.byLabel)
            byLabel.Add(s.IsEmpty() ? Json() : Json(s));
        EXPECT_STR_EQ(byLabel.Dump(), c.Get("byLabel")->Dump()) << "case " << i;
        i++;
    }
}

TEST(UiDocGolden4, SkillSeedIsTheDraftsOwnExport)
{
    UiDoc draft = DocOf(G4("draftUiml").Str());
    for (auto& c : G4("seed").Items())
        EXPECT_STR_EQ(L::SeedStructure(draft, &Kit(), c.Get("dpi")->Num()), c.Get("text")->Str()) << "dpi " << c.Get("dpi")->Num();
    EXPECT_STR_EQ(L::SeedStructure(draft, nullptr, 460), G4("seedNoKit").Str());
}

TEST(UiDocGolden4, SkillNoteIsTheWebs)
{
    const Json& cases = G4("skillNote");
    ASSERT_EQ(cases.Count(), 3);
    L::SkillInputs withOwn;
    withOwn.ownFiles = { "cut_banner.png", "bg_backdrop.png" };
    withOwn.kitFiles = { "btn_green.png" };
    EXPECT_STR_EQ(L::SkillNote(withOwn, &Kit(), Kit().textStyles), cases.At(0)->Get("text")->Str());
    EXPECT_STR_EQ(L::SkillNote(L::SkillInputs(), &Kit(), {}), cases.At(1)->Get("text")->Str());
    Json raw;
    Json::Parse(R"({"name":"x","font":"Bangers","weight":400,"size":30,"color":"#123456"})", raw);
    TextStyle bangers = NormalizeTextStyle(raw, "x");
    EXPECT_STR_EQ(L::SkillNote(L::SkillInputs(), nullptr, { bangers }), cases.At(2)->Get("text")->Str());
}

TEST(UiDocGolden4, CheckOfAnEditFindsWhatTheWebFinds)
{
    int i = 0;
    for (auto& c : G4("findings").Items())
    {
        Vector<String> got = L::CheckFindings(DocOf(c.Get("uiml")->Str()), c.Get("kit")->Bool() ? &Kit() : nullptr);
        Json list = Json::Array();
        for (auto& f : got)
            list.Add(f);
        EXPECT_STR_EQ(list.Dump(), c.Get("findings")->Dump()) << "case " << i;
        i++;
    }
}

TEST(UiDocGolden4, GridStepsAndBlocksOfAPlaceAreTheWebs)
{
    for (auto& c : G4("gridSteps").Items())
        EXPECT_EQ(L::GridStep(c.Get("w")->Num(), c.Get("h")->Num()), c.Get("step")->Num());

    UiDoc doc = DocOf(G4("draftUiml").Str());
    int i = 0;
    for (auto& c : G4("blocksIn").Items())
    {
        Vector<String> got = L::BlocksIn(doc, BoxOf(*c.Get("box")), (int)c.Get("max")->Num());
        Json list = Json::Array();
        for (auto& id : got)
            list.Add(id);
        EXPECT_STR_EQ(list.Dump(), c.Get("blocks")->Dump()) << "case " << i;
        i++;
    }
}
