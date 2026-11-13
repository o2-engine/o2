#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Windows/PipelineWindow/PipelineKitCard.h"
#include "o2Editor/Windows/PipelineWindow/PipelineKitLayout.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFx.h"

using namespace Editor;

// The UI kit card's grid (AssetsLine kitLayout.ts) and the Text style card's look edits (TextStyleFx.tsx) without a window:
// the numbers are the web's own (its kitGrid run on the same widths and input counts)

namespace
{
    struct WebGrid
    {
        float width;
        int   n;
        int   cols, rows;
        float cell, cellH;
        int   modelRows;
        float top, h, bodyH;
        float lastX, lastY; // The add cell's corner (body coordinates)
    };
}

TEST(PipelineKitCards, GridMatchesTheWeb)
{
    const WebGrid web[] = {
        { 460, 0, 5, 1, 82, 115, 1, 104, 115, 219, 11, 104 },
        { 460, 7, 5, 2, 82, 115, 1, 104, 236, 340, 187, 225 },
        { 360, 3, 4, 1, 80, 113, 2, 136, 113, 249, 269, 136 },
        { 760, 12, 9, 2, 76, 109, 1, 104, 224, 328, 257, 219 },
        { 200, 2, 2, 2, 86, 119, 2, 136, 244, 380, 11, 261 },
        { 1200, 30, 14, 3, 78, 111, 1, 104, 345, 449, 179, 338 },
    };
    for (auto& w : web)
    {
        auto g = PipelineKitLayout::Of(w.width, w.n);
        EXPECT_EQ(g.cols, w.cols) << w.width << " " << w.n;
        EXPECT_EQ(g.rows, w.rows) << w.width << " " << w.n;
        EXPECT_EQ(g.cell, w.cell) << w.width << " " << w.n;
        EXPECT_EQ(g.cellH, w.cellH) << w.width << " " << w.n;
        EXPECT_EQ(g.modelRows, w.modelRows) << w.width << " " << w.n;
        EXPECT_EQ(g.top, w.top) << w.width << " " << w.n;
        EXPECT_EQ(g.h, w.h) << w.width << " " << w.n;
        EXPECT_EQ(PipelineKitLayout::BodyHeight(w.width, w.n), w.bodyH) << w.width << " " << w.n;
        EXPECT_EQ(PipelineKitLayout::InputPos(g, -1), Vec2F(w.lastX, w.lastY)) << w.width << " " << w.n;
    }

    // An input's port is its cell's top-left corner: in the grid, the body's side padding away from the card's edge
    auto g = PipelineKitLayout::Of(460, 7);
    auto inGrid = PipelineKitLayout::CellInGrid(g, 6);
    auto inBody = PipelineKitLayout::CellBox(g, 6);
    EXPECT_EQ(inBody.x, inGrid.x + PipelineKitLayout::padX);
    EXPECT_EQ(inBody.y, inGrid.y + g.top);
    EXPECT_EQ(PipelineKitLayout::InputPos(g, 6), Vec2F(inBody.x, inBody.y));
}

TEST(PipelineKitCards, KitCardIsTheGridWidthAndTheAiTextStyleHasItsIcon)
{
    auto kit = PipelineNodeRegistry::GetSchema("uiKit");
    ASSERT_TRUE(kit);
    EXPECT_EQ(kit->defaultSize.x, PipelineKitLayout::defaultWidth);
    EXPECT_EQ(PipelineNodeWidget::IconForType("textStyleExtract"), "ui/pipeline/node_textstyleai.svg");
    EXPECT_EQ(PipelineNodeWidget::IconForType("textStyle"), "ui/pipeline/node_textstyle.svg");
}

TEST(PipelineKitCards, DeviceTexts)
{
    auto target = UiDoc::DefaultTarget();
    EXPECT_EQ(PipelineKitCards::TargetName(target), target.name);
    EXPECT_EQ(PipelineKitCards::TargetKind(target), "phone");
    EXPECT_EQ(PipelineKitCards::ScreenText(target.Screen()), "1179\xC3\x97" "2556 \xC2\xB7 460 dpi");
    UiDoc::UiTarget custom;
    custom.w = 800;
    custom.h = 600;
    custom.dpi = 163.5;
    EXPECT_EQ(PipelineKitCards::TargetName(custom), "Custom");
    EXPECT_EQ(PipelineKitCards::TargetKind(custom), "custom");
    EXPECT_EQ(PipelineKitCards::DpiText(163.54), "163.5");
    EXPECT_EQ(PipelineKitCards::TypeName("header"), "header / ribbon");
    EXPECT_EQ(PipelineKitCards::AssetName("sliced"), "9-slice");
}

// Every write of the fill or the effects carries the mirrors the exports and the older editors read
TEST(PipelineKitCards, TextStyleLookWritesTheMirrors)
{
    auto st = UiDoc::TextStyleDefault();
    st.size = 50;
    st.fill = UiDoc::TextGradient{ 90, { UiDoc::GradientStop{ "#FFFFFF", 0 }, UiDoc::GradientStop{ "#FFAA00", 1 } } };
    st.effects.Add(UiDoc::DefaultEffect("stroke", st));
    st.effects.Add(UiDoc::DefaultEffect("shadow", st));

    DataDocument config;
    config.SetObject();
    PipelineTextFxPanel::WriteLook(config, st);
    auto json = UiDoc::FromDataValue(config);
    ASSERT_TRUE(json.Get("fill") && json.Get("fill")->IsObject());
    EXPECT_EQ(json.Get("gradient")->ToJsString(), "#FFAA00");
    EXPECT_EQ(json.Get("color")->ToJsString(), "#FFFFFF");
    ASSERT_TRUE(json.Get("stroke") && json.Get("stroke")->IsObject());
    EXPECT_EQ(json.Get("stroke")->Get("width")->Num(), 3.0) << "the default outline: 6% of the size";
    ASSERT_TRUE(json.Get("shadow") && json.Get("shadow")->IsObject());
    EXPECT_EQ(json.Get("effects")->Count(), 2);

    // The outline switched off: the mirror goes, the effect stays; the fill gone: the colour is its first stop
    st.effects[0].off = true;
    st.color = st.fill->stops[0].color;
    st.fill.reset();
    PipelineTextFxPanel::WriteLook(config, st);
    json = UiDoc::FromDataValue(config);
    EXPECT_TRUE(json.Get("stroke")->IsNull());
    EXPECT_TRUE(json.Get("fill")->IsNull());
    EXPECT_TRUE(json.Get("gradient")->IsNull());
    EXPECT_EQ(json.Get("effects")->Count(), 2);
    auto back = UiDoc::TextStyleFromConfig(config);
    EXPECT_TRUE(back.effects[0].off);
    EXPECT_FALSE(back.fill.has_value());
}

TEST(PipelineKitCards, GradientStopsAndEffectOrder)
{
    UiDoc::TextGradient g{ 90, { UiDoc::GradientStop{ "#000000", 0 }, UiDoc::GradientStop{ "#FFFFFF", 1 } } };
    int index = -1;
    auto added = PipelineTextFxPanel::WithStopAt(g, 0.254, index);
    ASSERT_EQ(added.stops.Count(), 3);
    EXPECT_EQ(index, 1);
    EXPECT_EQ(added.stops[1].at, 0.25);
    EXPECT_EQ(added.stops[1].color, "#404040") << "the colour the gradient has there";
    auto end = PipelineTextFxPanel::WithStopAt(added, 1.5, index);
    EXPECT_EQ(index, 3);
    EXPECT_EQ(end.stops[3].at, 1.0);
    EXPECT_EQ(end.stops[3].color, "#FFFFFF");

    // Before the previous one of its kind: an outline goes closer to the letters, another kind in between stays
    auto st = UiDoc::TextStyleDefault();
    Vector<UiDoc::TextEffect> fx = { UiDoc::DefaultEffect("stroke", st), UiDoc::DefaultEffect("shadow", st) };
    st.effects = fx;
    fx.Add(UiDoc::DefaultEffect("stroke", st));
    fx[2].color = "#123456";
    auto moved = PipelineTextFxPanel::MovedUp(fx, 2);
    EXPECT_EQ(moved[0].color, "#123456");
    EXPECT_EQ(moved[1].type, "shadow");
    EXPECT_EQ(moved[2].color, fx[0].color);
    EXPECT_EQ(PipelineTextFxPanel::MovedUp(fx, 1)[1].type, "shadow") << "no earlier shadow: unchanged";

    // Rows: the fill takes one (two with a gradient), the effects a head and a row or two each
    st.effects = moved;
    EXPECT_EQ(PipelineTextFxPanel::Rows(true, st), 1 + 1 + 2 + 1);
    EXPECT_EQ(PipelineTextFxPanel::Rows(false, st), 1);
    st.fill = g;
    EXPECT_EQ(PipelineTextFxPanel::Rows(false, st), 2);
    EXPECT_EQ(PipelineTextFxPanel::Height(false, st), 2*26.0f + 5.0f);
}
