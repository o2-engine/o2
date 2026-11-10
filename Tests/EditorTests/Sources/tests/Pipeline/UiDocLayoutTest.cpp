#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

using namespace UiDocTest;

namespace
{
    UiRects Rects2(const UiDoc& d) { return SolveLayout(d, 1000, 2000); }

    UiDoc Apply2(const Vector<UiOp>& ops) { return ApplyOps(Parse(Screen2()), ops).doc; }

    UiOp Op(const String& id, const UiPatch& patch)
    {
        UiOp op;
        EXPECT_TRUE(PatchOp(id, patch, op));
        return op;
    }

    Json SlotJson(bool found, const UiSlot& s)
    {
        if (!found)
            return Json();
        Json out = Json::Object();
        if (!s.before.IsEmpty())
            out["before"] = s.before;
        else
            out["after"] = s.after;
        return out;
    }

    Json SnapJson(const UiSnap& s)
    {
        Json out = Json::Object();
        out["rect"] = RectJson(s.rect);
        Json lines = Json::Array();
        for (auto& l : s.lines)
        {
            Json line = Json::Object();
            line["axis"] = String(std::string(1, l.axis));
            line["at"] = l.at;
            line["from"] = l.from;
            line["to"] = l.to;
            lines.Add(line);
        }
        out["lines"] = lines;
        return out;
    }

    Json NumbersJson(const Vector<double>& v)
    {
        Json out = Json::Array();
        for (double d : v)
            out.Add(Json(d));
        return out;
    }

    Anchor AnchorOf(const Json& j)
    {
        Anchor a;
        a.h = S(j, "h");
        a.v = S(j, "v");
        return a;
    }
}

// --- the web's tests (backend/test/uiLayout.test.ts: the solver) -------------------------------------------------------

TEST(UiDocLayout, AnchorsCornersKeepSizeStretchedAxesFillMinusMargins)
{
    UiRects r = SolveLayout(Parse(Screen()));
    EXPECT_EQ(r.Get("close"), UiRect(1080 - 36 - 96, 36, 96, 96));
    EXPECT_EQ(r.Get("title"), UiRect(60, 160, 960, 140));
    EXPECT_EQ(r.Get("bg"), UiRect(0, 0, 1080, 1920));
    EXPECT_EQ(r.Get("bar"), UiRect(0, 1920 - 180, 1080, 180));
}

TEST(UiDocLayout, ColumnStretchesChildrenAcrossRowSplitsFreeSpaceByGrow)
{
    UiRects r = SolveLayout(Parse(Screen()));
    EXPECT_EQ(r.Get("play"), UiRect(220, 660, 640, 140));
    EXPECT_EQ(r.Get("settings").y, 660 + 140 + 30);
    EXPECT_EQ(r.Get("a").w, (1080 - 40 - 20)/2.0);
    EXPECT_EQ(r.Get("b").x, 20 + (1080 - 40 - 20)/2.0 + 20);
}

TEST(UiDocLayout, RowJustifyCenterWithAlignCenterKeepsSizesAndCentresThem)
{
    UiRects r = SolveLayout(Parse(Screen()));
    UiRect coins = r.Get("coins"), coin = r.Get("coin");
    EXPECT_EQ(coin.w, 64);
    EXPECT_EQ(coin.y, coins.y + (80 - 64)/2.0);
    EXPECT_EQ(coin.x, coins.x + (640 - (64 + 16 + 220))/2.0);
}

TEST(UiDocLayout, GridColumnsOfEqualCellsRowByRow)
{
    UiRects r = SolveLayout(Parse(Screen()));
    UiRect g = r.Get("slots");
    double cw = (g.w - 3*16)/4;
    EXPECT_EQ(r.Get("s1"), UiRect(g.x, g.y, cw, cw));
    EXPECT_EQ(r.Get("s5"), UiRect(g.x, g.y + cw + 16, cw, cw));
}

TEST(UiDocLayout, AnotherScreenSizeMovesAnchoredBlocksWithTheirEdges)
{
    UiRects wide = SolveLayout(Parse(Screen()), 1920, 1080);
    EXPECT_EQ(wide.Get("close"), UiRect(1920 - 36 - 96, 36, 96, 96));
    EXPECT_EQ(wide.Get("bar").w, 1920);
    EXPECT_EQ(wide.Get("menu").x, (1920 - 640)/2.0);
}

// --- the web's tests (backend/test/uiEdit.test.ts: placing a block by hand) --------------------------------------------

TEST(UiDocLayout, AMoveKeepsTheAnchorOffsetsFromTheAnchoredEdges)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    const UiNode& n = *FindNode(d, "corner");
    UiRect p = rects.Get("box"), r = rects.Get("corner");
    UiPatch patch = PlaceAt(n, p, UiRect(r.x - 40, r.y + 10, r.w, r.h));
    EXPECT_STR_EQ(patch.set.Dump(), "{\"x\":60,\"y\":20}");
    // Same place on a wider screen: still 60 px from the panel's right edge
    UiRects wide = SolveLayout(Apply2({ Op("corner", patch) }), 1400, 2000);
    UiRect pr = wide.Get("box"), cr = wide.Get("corner");
    EXPECT_EQ(pr.x + pr.w - (cr.x + cr.w), 60);
}

TEST(UiDocLayout, AStretchedAxisTakesMarginsAResizeWritesTheSize)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    const UiNode& n = *FindNode(d, "bar");
    UiRect p = rects.Get("box"), r = rects.Get("bar");
    UiPatch moved = PlaceAt(n, p, UiRect(r.x + 5, r.y, r.w - 20, 60), nullptr, true);
    EXPECT_STR_EQ(moved.set.Dump(), "{\"h\":60,\"margin\":[0,30,0,30]}");
    UiRect after = SolveLayout(Apply2({ Op("bar", moved) }), 1000, 2000).Get("bar");
    EXPECT_EQ(after.x, r.x + 5);
    EXPECT_EQ(after.w, r.w - 20);
}

TEST(UiDocLayout, ANewAnchorKeepsTheBlockWhereItIs)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    const UiNode& n = *FindNode(d, "corner");
    UiRect p = rects.Get("box"), r = rects.Get("corner");
    for (auto& a : { Anchor{ "left", "top" }, Anchor{ "center", "middle" }, Anchor{ "stretch", "bottom" }, Anchor{ "stretch", "stretch" } })
    {
        UiRect again = SolveLayout(Apply2({ Op("corner", PlaceAt(n, p, r, &a)) }), 1000, 2000).Get("corner");
        EXPECT_EQ(again, r) << a.h.Data() << " " << a.v.Data();
    }
    // From stretched back to a point: the size is written (the default would differ)
    Anchor stretch{ "stretch", "stretch" }, topLeft{ "left", "top" };
    UiDoc stretched = Apply2({ Op("corner", PlaceAt(n, p, r, &stretch)) });
    UiPatch back = PlaceAt(*FindNode(stretched, "corner"), p, r, &topLeft);
    EXPECT_EQ(back.set.Get("w")->Num(), 100);
    EXPECT_EQ(back.set.Get("h")->Num(), 50);
    EXPECT_TRUE(back.unset.Contains("margin"));
}

TEST(UiDocLayout, TheAnchorPointNearestThePointer)
{
    UiRect p(0, 0, 400, 400);
    EXPECT_EQ(AnchorAt(p, 10, 390), (Anchor{ "left", "bottom" }));
    EXPECT_EQ(AnchorAt(p, 210, 180), (Anchor{ "center", "middle" }));
    EXPECT_EQ(AnchorAt(p, 380, 5), (Anchor{ "right", "top" }));
}

TEST(UiDocLayout, InsideARowADragReordersAndAResizeDropsTheGrowShare)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    const UiNode& row = *FindNode(d, "r");
    UiRect c = rects.Get("c");
    UiSlot slot;
    ASSERT_TRUE(LayoutSlot(row, rects, "a", c.x + c.w - 5, c.y + 5, slot));
    EXPECT_STR_EQ(slot.after, "c");
    ASSERT_TRUE(LayoutSlot(row, rects, "c", rects.Get("a").x + 2, c.y, slot));
    EXPECT_STR_EQ(slot.before, "a");
    UiPatch sized = SizeInLayout(*FindNode(d, "c"), row, c, UiRect(c.x, c.y, 200, c.h));
    EXPECT_STR_EQ(PatchJson(sized).Dump(), "{\"set\":{\"w\":200},\"unset\":[\"grow\"]}");
}

TEST(UiDocLayout, SnappingToTheParentAndSiblings)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    UiSnapTargets t = SnapTargets(d, rects, "corner");
    UiRect p = rects.Get("box"), r = rects.Get("corner");
    UiSnap s = SnapMove(UiRect(p.x + p.w - r.w + 3, r.y, r.w, r.h), t, 6);
    EXPECT_EQ(s.rect.x + s.rect.w, p.x + p.w);
    ASSERT_FALSE(s.lines.IsEmpty());
    EXPECT_EQ(s.lines[0].axis, 'x');
}

TEST(UiDocLayout, ArrowsNudgeANewParentKeepsThePlaceOnTheScreen)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    UiOp op;
    ASSERT_TRUE(NudgeOp(d, rects, "corner", -1, 0, op));
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"corner\",\"set\":{\"x\":21}}");
    ASSERT_TRUE(NudgeOp(d, rects, "a", 0, 2, op));
    EXPECT_STR_EQ(op.Dump(), "{\"op\":\"update\",\"id\":\"a\",\"set\":{\"y\":2}}");
    UiRect before = rects.Get("corner");
    auto ops = ReparentOps(d, rects, "corner", "root");
    ASSERT_EQ(ops.Count(), 2);
    EXPECT_EQ(SolveLayout(Apply2(ops), 1000, 2000).Get("corner"), before);
    // Into a row: offsets would only push it off its slot
    EXPECT_STR_EQ(ReparentOps(d, rects, "corner", "r")[1].Dump(), "{\"op\":\"update\",\"id\":\"corner\",\"unset\":[\"x\",\"y\"]}");
}

// --- golden ------------------------------------------------------------------------------------------------------

TEST(UiDocLayout, GoldenSolverBoxesAndHits)
{
    for (auto& c : G("solve").Items())
    {
        UiDoc doc = Parse(S(c, "src"));
        for (auto& size : M(c, "sizes").Items())
        {
            SCOPED_TRACE((S(c, "name") + " at " + M(size, "w").Dump() + "x" + M(size, "h").Dump()).Data());
            UiRects rects = SolveLayout(doc, M(size, "w").Num(), M(size, "h").Num());
            Json list = Json::Array();
            for (auto& id : rects.Ids())
                list.Add(RectEntry(id, rects.Get(id)));
            EXPECT_STR_EQ(list.Dump(), M(size, "rects").Dump());
            for (auto& h : M(size, "hits").Items())
            {
                String hit = HitTest(doc, rects, h.At(0)->Num(), h.At(1)->Num());
                EXPECT_STR_EQ(hit, h.At(2)->IsNull() ? String() : h.At(2)->Str()) << h.Dump().Data();
            }
        }
    }

    UiDoc tricky = Parse(S(*G("uiml").At(3), "src"));
    for (auto& c : G("grid").Items())
    {
        UiGridGeometry g = GridGeometry(*FindNode(tricky, "g2"), UiRect(0, 0, M(c, "w").Num(), 100), (int)M(c, "n").Num());
        Json j = Json::Object();
        j["cols"] = g.cols; j["cw"] = g.cw; j["ch"] = g.ch; j["gx"] = g.gx; j["gy"] = g.gy; j["gridW"] = g.gridW; j["gridH"] = g.gridH;
        EXPECT_STR_EQ(j.Dump(), M(c, "g").Dump());
    }
}

TEST(UiDocLayout, GoldenPlaceAtSlotsAndSizes)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    for (auto& c : G("placeAt").Items())
    {
        String id = S(c, "id");
        const UiNode& n = *FindNode(d, id);
        UiRect p = rects.Get(FindParent(d, id)->id), r = rects.Get(id);
        const Json& dd = M(c, "d");
        UiRect rr(r.x + dd.At(0)->Num(), r.y + dd.At(1)->Num(), r.w + dd.At(2)->Num(), r.h + dd.At(3)->Num());
        Anchor a = M(c, "a").IsNull() ? Anchor() : AnchorOf(M(c, "a"));
        UiPatch patch = PlaceAt(n, p, rr, M(c, "a").IsNull() ? nullptr : &a, M(c, "size").Bool());
        EXPECT_STR_EQ(PatchJson(patch).Dump(), M(c, "patch").Dump()) << c.Dump().Data();
    }

    const UiNode& row = *FindNode(d, "r");
    for (auto& c : G("slots").Items())
    {
        UiSlot slot;
        bool found = LayoutSlot(row, rects, S(c, "d"), M(c, "x").Num(), M(c, "y").Num(), slot);
        EXPECT_STR_EQ(SlotJson(found, slot).Dump(), M(c, "s").Dump()) << c.Dump().Data();
    }

    UiDoc tdoc = Parse(S(*G("uiml").At(3), "src"));
    UiRects trects = SolveLayout(tdoc);
    for (auto& c : G("gridSlots").Items())
    {
        UiSlot slot;
        bool found = LayoutSlot(*FindNode(tdoc, "g2"), trects, "q1", M(c, "x").Num(), M(c, "y").Num(), slot);
        EXPECT_STR_EQ(SlotJson(found, slot).Dump(), M(c, "s").Dump()) << c.Dump().Data();
    }

    for (auto& c : G("sizeInLayout").Items())
    {
        UiRect from = rects.Get(S(c, "id"));
        UiPatch p = SizeInLayout(*FindNode(d, S(c, "id")), *FindNode(d, S(c, "pid")), from,
                                 UiRect(from.x, from.y, M(c, "w").Num(), M(c, "h").Num()));
        EXPECT_STR_EQ(PatchJson(p).Dump(), M(c, "p").Dump()) << c.Dump().Data();
    }

    for (auto& c : G("anchorAt").Items())
    {
        EXPECT_STR_EQ(AnchorJson(AnchorAt(UiRect(0, 0, 400, 400), c.At(0)->Num(), c.At(1)->Num())).Dump(), c.At(2)->Dump());
        EXPECT_STR_EQ(AnchorJson(AnchorAt(UiRect(0, 0, 0, 0), c.At(0)->Num(), c.At(1)->Num())).Dump(), c.At(3)->Dump());
    }

    for (auto& c : G("anchorBox").Items())
    {
        String id = c.At(0)->Str();
        UiAnchorBox b = AnchorBox(*FindNode(d, id), rects.Get(FindParent(d, id)->id));
        const Json& e = *c.At(1);
        EXPECT_EQ(b.x0, M(e, "x0").Num()) << id.Data();
        EXPECT_EQ(b.x1, M(e, "x1").Num()) << id.Data();
        EXPECT_EQ(b.y0, M(e, "y0").Num()) << id.Data();
        EXPECT_EQ(b.y1, M(e, "y1").Num()) << id.Data();
    }
}

TEST(UiDocLayout, GoldenSnapNudgeReparentAndNoops)
{
    UiDoc d = Parse(Screen2());
    UiRects rects = Rects2(d);
    for (auto& c : G("snap").Items())
    {
        String id = S(c, "id");
        SCOPED_TRACE(id.Data());
        UiSnapTargets t = SnapTargets(d, rects, id);
        Json tj = Json::Object();
        tj["xs"] = NumbersJson(t.xs);
        tj["ys"] = NumbersJson(t.ys);
        tj["span"] = RectJson(t.span);
        EXPECT_STR_EQ(tj.Dump(), M(c, "t").Dump());

        UiRect r = rects.Get(id);
        const double moves[4][2] = { { 3, 0 }, { -4, 5 }, { 7.5, -6.2 }, { 100, 100 } };
        for (int i = 0; i < 4; i++)
        {
            UiSnap s = SnapMove(UiRect(r.x + moves[i][0], r.y + moves[i][1], r.w, r.h), t, 6);
            EXPECT_STR_EQ(SnapJson(s).Dump(), M(c, "moves").At(i)->Dump());
        }
        UiEdges edges[3];
        edges[0].e = true;
        edges[1].w = true;
        edges[1].n = true;
        edges[2].s = true;
        for (int i = 0; i < 3; i++)
        {
            UiSnap s = SnapResize(UiRect(r.x, r.y, r.w + 4, r.h - 3), edges[i], t, 6);
            EXPECT_STR_EQ(SnapJson(s).Dump(), M(c, "resizes").At(i)->Dump());
        }
    }

    for (auto& c : G("nudge").Items())
    {
        UiOp op;
        bool made = NudgeOp(d, rects, S(c, "id"), M(c, "dx").Num(), M(c, "dy").Num(), op);
        EXPECT_STR_EQ(made ? op.Dump() : String("null"), M(c, "op").Dump()) << c.Dump().Data();
    }

    UiDoc n2 = Parse(Screen2() + "  text hid anchor=center hidden \"h\"\n");
    UiRects r2 = SolveLayout(n2, 1000, 2000);
    for (auto& c : G("reparent").Items())
    {
        auto ops = ReparentOps(n2, r2, S(c, "id"), S(c, "parent"), S(c, "before"), S(c, "after"));
        Json list = Json::Array();
        for (auto& op : ops)
            list.Add(op);
        EXPECT_STR_EQ(list.Dump(), M(c, "ops").Dump()) << c.Dump().Data();
    }

    for (auto& c : G("noop").Items())
        EXPECT_EQ(MoveIsNoop(d, *c.At(0)), c.At(1)->Bool()) << c.Dump().Data();
}
