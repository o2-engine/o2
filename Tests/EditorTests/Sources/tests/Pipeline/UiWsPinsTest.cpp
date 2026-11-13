#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPins.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsRunActs.h"

using namespace o2;
using namespace Editor;

#define EXPECT_STR_EQ(a, b) EXPECT_EQ(std::string(a), std::string(b))

// The request's points as tokens of its text (PinInput.tsx): numbered in the order they stand, a deleted token drops its
// point, a token typed past the points is just text; and the words of a run's acts (RunActs.tsx)

TEST(UiWsPins, TokensAreNumberedInTheOrderTheyStand)
{
    auto a = UiWsPins::Settle("move [2] under [1]", 2);
    EXPECT_STR_EQ(a.text, "move [1] under [2]");
    EXPECT_EQ(a.order, Vector<int>({ 2, 1 }));

    // A token deleted from the text: its point goes, the others are numbered anew
    auto b = UiWsPins::Settle("[1] bigger, [3] red", 3);
    EXPECT_STR_EQ(b.text, "[1] bigger, [2] red");
    EXPECT_EQ(b.order, Vector<int>({ 1, 3 }));

    // A number past the points is text; a point named twice keeps its first place
    auto c = UiWsPins::Settle("[5] and [1] then [1]", 1);
    EXPECT_STR_EQ(c.text, "[5] and [1] then [1]");
    EXPECT_EQ(c.order, Vector<int>({ 1 }));

    auto none = UiWsPins::Settle("no points here", 0);
    EXPECT_STR_EQ(none.text, "no points here");
    EXPECT_TRUE(none.order.IsEmpty());
}

TEST(UiWsPins, APointGoesInAtTheCaretWithSpaceAround)
{
    int caret = 0;
    // A space after the caret already: no second one, the caret goes past it
    EXPECT_STR_EQ(UiWsPins::InsertPin("make bigger", 4, 1, caret), "make [1] bigger");
    EXPECT_EQ(caret, 9);
    EXPECT_STR_EQ(UiWsPins::InsertPin("make\tbigger", 4, 1, caret), "make [1]\tbigger");
    EXPECT_EQ(caret, 9);
    EXPECT_STR_EQ(UiWsPins::InsertPin("makebigger", 4, 1, caret), "make [1] bigger");
    EXPECT_EQ(caret, 9);
    EXPECT_STR_EQ(UiWsPins::InsertPin("make ", 5, 2, caret), "make [2] ");
    EXPECT_EQ(caret, 9);
    EXPECT_STR_EQ(UiWsPins::InsertPin("", 0, 1, caret), "[1] ");
    EXPECT_EQ(caret, 4);
    // Past the end: the end; the caret counts characters, not bytes
    EXPECT_STR_EQ(UiWsPins::InsertPin("\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD0\xB0", 99, 3, caret),
                  "\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD0\xB0 [3] ");
    EXPECT_EQ(caret, 11);

    // Put in before the other points, it takes the first number
    String text = UiWsPins::InsertPin("then [1] here", 0, 2, caret);
    auto settled = UiWsPins::Settle(text, 2);
    EXPECT_STR_EQ(settled.text, "[1] then [2] here");
    EXPECT_EQ(settled.order, Vector<int>({ 2, 1 }));
}

TEST(UiWsRunActs, ActsAreWordedAsTheWebWordsThem)
{
    UiDoc::Json acts;
    UiDoc::Json::Parse(R"([
        {"id":"analysis","kind":"analysis","title":"Reading the reference","status":"ok","ms":4200,"data":{"elements":12,"kit":9,"looks":2,"texts":3}},
        {"id":"styles","kind":"styles","title":"Text styles","status":"warn","ms":300},
        {"id":"skill-turn-6","kind":"turn","title":"Turn 6: measure 60 270 650 770","status":"ok","ms":95000,"data":{"skill":1,"action":"measure","step":"turn-6","label":"Turn 6: measure 60 270 650 770"}},
        {"id":"skill-check","kind":"check","title":"The first draft: passed","status":"ok","data":{"skill":1,"step":"check","label":"The first draft: passed","score":0.734}},
        {"id":"turn-2","kind":"turn","title":"Turn 2","status":"ok","data":{"edits":3,"made":1,"styles":2,"done":1,"score":0.81}},
        {"id":"turn-3","kind":"turn","title":"Turn 3","status":"run"}
    ])", acts);
    EXPECT_STR_EQ(UiWsRunActs::ActTitle(*acts.At(0)), "Reading the reference");
    EXPECT_STR_EQ(UiWsRunActs::ActDetail(*acts.At(0)), "12 elements \xC2\xB7 9 drawn by the kit \xC2\xB7 2 to cut out");
    EXPECT_STR_EQ(UiWsRunActs::ActTitle(*acts.At(2)), "Turn 6 \xC2\xB7 Measures on the grid");
    EXPECT_STR_EQ(UiWsRunActs::ActDetail(*acts.At(2)), "60 270 650 770");
    EXPECT_STR_EQ(UiWsRunActs::ActTitle(*acts.At(3)), "Draft checked: diff with the reference");
    EXPECT_STR_EQ(UiWsRunActs::ActDetail(*acts.At(3)), "likeness 73% \xC2\xB7 The first draft: passed");
    EXPECT_STR_EQ(UiWsRunActs::ActDetail(*acts.At(4)), "likeness 81% \xC2\xB7 edits: 3 \xC2\xB7 drawn: 1 \xC2\xB7 styles: 2 \xC2\xB7 done");
    EXPECT_STR_EQ(UiWsRunActs::ActDetail(*acts.At(5)), "");
    EXPECT_STR_EQ(UiWsRunActs::RunNow(acts, 1000, 61000), "Turn 3\xE2\x80\xA6 \xC2\xB7 60 s");
    EXPECT_STR_EQ(UiWsRunActs::Took(95000), "2 min");
    EXPECT_STR_EQ(UiWsRunActs::Took(400), "");
    EXPECT_STR_EQ(UiWsRunActs::Took(3725000), "62 min");
    EXPECT_STR_EQ(UiWsRunActs::Took(6000000), "1 h 40 min");

    UiDoc::Json run;
    UiDoc::Json::Parse(R"({"id":"s1","ms":420000,"turns":44,"acts":[],"built":true,"reused":30,"made":3,"score":0.83,"scoreFrom":0.6,
                          "skill":{"complete":true,"turns":44,"checks":5,"targets":12,"open":[]}})", run);
    EXPECT_STR_EQ(UiWsRunActs::RunSummary(run), "turns: 44 \xC2\xB7 checks against the design: 5 \xC2\xB7 elements compared: 12 \xC2\xB7 likeness 60% "
                  "\xE2\x86\x92 83% \xC2\xB7 blocks drawn by the kit: 30 \xC2\xB7 graphics made: 3 \xC2\xB7 7 min");
}
