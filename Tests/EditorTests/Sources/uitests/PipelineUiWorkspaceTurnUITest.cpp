#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "UiWorkspaceUiFixture.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"

// Turned blocks in the AI UI layout workspace (AssetsLine uiLayout/Stage.tsx, UiWorkspace.tsx): the knob above the
// selection turns it (Shift: 15 degree steps, near 0 / 90 / 180 it snaps), a turned block is picked, handled, moved and
// resized along its own axes where it is drawn, a resize of a turned block keeps the side not pulled where it is on the
// screen, and the inspector's Rotation row (a field, a quarter turn either way)

namespace
{
    // The screen of the tests: a card turned a quarter with a button in it, a tile turned on its own
    const char* const turnedScreen =
        "screen Menu 1080x1920\n"
        "  panel card anchor=center size=760x900 rotation=90\n"
        "    button play anchor=bottom-center pos=0,60 size=400x120 \"PLAY\"\n"
        "  panel tile anchor=top-left pos=100,100 size=200x100 rotation=90\n"
        "  icon coin anchor=top-right pos=24,24 size=96x96\n";

    struct UiWorkspaceTurnFixture: UiWorkspaceFixture
    {
        UiDoc::UiRect RectOf(const String& id) const { return UiDoc::SolveLayout(DocNow()).Get(id); }

        std::optional<double> TurnOf(const String& id) const
        {
            auto doc = DocNow();
            const UiDoc::UiNode* n = UiDoc::FindNode(doc, id);
            return n ? n->rotation : std::nullopt;
        }

        // Where a point of a block's own box (fractions of its size) is on the stage (its turns and its parents' applied), world
        Vec2F OnScreen(const String& id, double fx, double fy) const
        {
            const auto& doc = Ctx()->Doc();
            const auto& rects = Ctx()->rects;
            UiDoc::UiRect r = rects.Get(id);
            UiDoc::UiPoint p = UiDoc::Apply(UiDoc::WorldMatrix(doc, rects, id), { r.x + r.w*fx, r.y + r.h*fy });
            return Window()->GetStage()->DesignToWorld(p.x, p.y);
        }

        // The world offset of a design offset at the stage's zoom (y down in design, up in the world)
        Vec2F WorldDelta(float dx, float dy) const
        {
            float k = Window()->GetStage()->GetZoom();
            return Vec2F(dx*k, -dy*k);
        }
    };
}

// The knob turns the block to face the pointer about its centre, as one hand action; Shift turns by 15 degrees, a turn near
// upright snaps to it and drops the key
TEST_F(UiWorkspaceTurnFixture, TheKnobTurnsTheBlock)
{
    OpenWorkspace(false);
    auto stage = Window()->GetStage();
    Click(stage->BlockRect("coin").Center());
    ASSERT_EQ(Ctx()->selected, String("coin"));
    Vec2F knob;
    ASSERT_TRUE(stage->KnobPoint(knob));
    Vec2F centre = stage->BlockRect("coin").Center();
    EXPECT_NEAR(knob.x, centre.x, 0.5f) << "above the top edge's middle";
    EXPECT_GT(knob.y, stage->BlockRect("coin").top) << "above the block";

    // To the right of the centre: a quarter turn clockwise
    Drag(knob, centre + Vec2F(120, 2));
    EXPECT_EQ(TurnOf("coin"), std::optional<double>(90));
    EXPECT_EQ(LastLabel(), String("coin: rotation"));
    auto after = RectOf("coin");
    EXPECT_EQ(after.w, 96);
    EXPECT_EQ(after.h, 96) << "the box does not change";

    // The knob now stands right of the block; a pull to 37 degrees below the right with Shift: 15-degree steps
    ASSERT_TRUE(stage->KnobPoint(knob));
    EXPECT_GT(knob.x, centre.x + 40.0f);
    float a = 37.0f*Math::PI()/180.0f;
    DragWith(knob, centre + Vec2F(std::cos(a), -std::sin(a))*120.0f, { VK_SHIFT });
    EXPECT_EQ(TurnOf("coin"), std::optional<double>(120)) << "90 + 37 is 127: the nearest 15-degree step is 120";

    // Near upright (3 degrees off) it snaps, and an upright block has no rotation key
    ASSERT_TRUE(stage->KnobPoint(knob));
    float b = 87.0f*Math::PI()/180.0f;
    Drag(knob, centre + Vec2F(std::cos(b), std::sin(b))*120.0f);
    EXPECT_EQ(TurnOf("coin"), std::nullopt);
}

// In a turned parent the block is picked where it is drawn, its handles sit on its turned box, and a drag moves it along
// the parent's axes (not snapped)
TEST_F(UiWorkspaceTurnFixture, ABlockInATurnedParentIsHandledWhereItIsDrawn)
{
    OpenWorkspace(false, turnedScreen);
    auto stage = Window()->GetStage();
    auto before = RectOf("play");
    // The card turned a quarter about its centre: the button below its middle is drawn left of it, standing
    Vec2F drawn = OnScreen("play", 0.5, 0.5);
    RectF card = stage->BlockRect("card");
    EXPECT_NEAR(drawn.x, card.Center().x - 330.0f*stage->GetZoom(), 1.0f) << "330 design pixels left of the card's centre";
    EXPECT_NEAR(drawn.y, card.Center().y, 0.5f);
    Click(drawn);
    ASSERT_EQ(Ctx()->selected, String("play"));
    Vec2F e = stage->HandlePoint("e");
    Vec2F want = OnScreen("play", 1, 0.5);
    EXPECT_NEAR(e.x, want.x, 0.5f);
    EXPECT_NEAR(e.y, want.y, 0.5f);
    EXPECT_NEAR(e.x, drawn.x, 0.5f) << "its east edge's middle is under its centre on the screen";
    EXPECT_LT(e.y, drawn.y);

    // Down the screen is along the button's own x: it moves right in its parent's frame
    Vec2F at = OnScreen("play", 0.5, 0.5);
    Drag(at, at + WorldDelta(0, 50));
    auto moved = RectOf("play");
    EXPECT_NEAR(moved.x, before.x + 50, 1.0);
    EXPECT_NEAR(moved.y, before.y, 1.0);
    EXPECT_EQ(LastLabel(), String("play: move"));

    // The east edge pulled down the screen widens it; the west side stays
    e = stage->HandlePoint("e");
    Drag(e, e + WorldDelta(0, 40));
    auto wide = RectOf("play");
    EXPECT_NEAR(wide.w, before.w + 40, 1.0);
    EXPECT_NEAR(wide.x, moved.x, 1.0);
}

// A turned block resized from one side keeps the other side where it is on the screen ((R - I)(c' - c))
TEST_F(UiWorkspaceTurnFixture, AResizeOfATurnedBlockKeepsTheOtherSideOnTheScreen)
{
    OpenWorkspace(false, turnedScreen);
    auto stage = Window()->GetStage();
    // The tile 200 x 100 at (100, 100) turned a quarter about (200, 150): drawn at 150..250 x 50..250
    Click(OnScreen("tile", 0.85, 0.5));
    ASSERT_EQ(Ctx()->selected, String("tile"));
    Vec2F west = OnScreen("tile", 0, 0.5);
    Vec2F e = stage->HandlePoint("e");
    Vec2F want = OnScreen("tile", 1, 0.5);
    EXPECT_NEAR(e.x, want.x, 0.5f);
    EXPECT_NEAR(e.y, want.y, 0.5f);

    Drag(e, e + WorldDelta(0, 60));
    auto after = RectOf("tile");
    EXPECT_NEAR(after.w, 260, 1.0);
    EXPECT_NEAR(after.h, 100, 1.0);
    Vec2F westAfter = OnScreen("tile", 0, 0.5);
    EXPECT_NEAR(westAfter.x, west.x, 1.0f) << "the side not pulled stays where it is on the screen";
    EXPECT_NEAR(westAfter.y, west.y, 1.0f);
    EXPECT_EQ(TurnOf("tile"), std::optional<double>(90));
    EXPECT_EQ(LastLabel(), String("tile: size"));
}

// The inspector's Rotation row: typed degrees (cleaned; 0 drops the key) and a quarter turn either way
TEST_F(UiWorkspaceTurnFixture, TheInspectorRotationRow)
{
    OpenWorkspace(false);
    Ctx()->Select("coin");
    Window()->GetSide()->OpenTab("inspect");
    Step(3);
    auto inspector = Window()->GetSide()->GetInspector();
    auto field = inspector->GetField("num:rotation");
    ASSERT_TRUE(field);
    field->SetText("370");
    Step();
    EXPECT_EQ(TurnOf("coin"), std::optional<double>(10));
    EXPECT_EQ(LastLabel(), String("coin: rotation"));

    ASSERT_FALSE(inspector->HotRect("turn:right").IsZero());
    inspector->ClickHot("turn:right");
    Step(2);
    EXPECT_EQ(TurnOf("coin"), std::optional<double>(100));
    inspector->ClickHot("turn:left");
    inspector->ClickHot("turn:left");
    Step(2);
    EXPECT_EQ(TurnOf("coin"), std::optional<double>(-80));

    field = inspector->GetField("num:rotation");
    ASSERT_TRUE(field);
    field->SetText("0");
    Step();
    EXPECT_EQ(TurnOf("coin"), std::nullopt) << "upright: no key";

    // The screen itself has no Rotation row
    Ctx()->Select("");
    Step(3);
    EXPECT_FALSE(inspector->GetField("num:rotation"));
}

// A shot of the turned selection, its handles, the knob and the turned parent's frame (review material)
TEST_F(UiWorkspaceTurnFixture, TurnedSelectionShot)
{
    OpenWorkspace(true, turnedScreen);
    Click(OnScreen("play", 0.5, 0.5));
    ASSERT_EQ(Ctx()->selected, String("play"));
    Step(3);
    Shot("turned-selection");
}
