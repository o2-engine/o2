#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineComposerLayout.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

// Rules of the composer card shared with AssetsLine (shared/composer.ts, composerLayout.ts) and the Unity plugin: the
// rows the input ports sit on, a layer's export size, where a new layer lands and the screen-sized frame

namespace
{
    PipelineNode Composer(const Vector<String>& imageInputs)
    {
        PipelineNode node;
        node.nodeType = "composer";
        for (auto& id : imageInputs)
            node.inputs.Add(PipelinePort(id, id, PipelinePortType::Image, true));
        return node;
    }

    Vector<String> StoredOrder(const PipelineNode& node)
    {
        Vector<String> ids;
        if (auto order = node.GetConfigValue("layerOrder"))
        {
            for (auto& id : *order)
                ids.Add(PipelineUtils::ValueToString(id));
        }
        return ids;
    }

    void SetOrder(PipelineNode& node, const Vector<String>& ids)
    {
        auto& order = node.config["layerOrder"];
        order.SetArray();
        for (auto& id : ids)
            order.AddElement() = id;
    }
}

TEST(PipelineComposerLayout, ExportSizeNeedsBothSides)
{
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(120.4f, 60.6f, 0.0f, 0.0f), Vec2I(120, 61)) << "no export size: the layer's own";
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(120.0f, 60.0f, 512.0f, 256.0f), Vec2I(512, 256));
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(120.0f, 60.0f, 300.4f, 99.5f), Vec2I(300, 100)) << "whole pixels";
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(120.0f, 60.0f, 512.0f, 0.0f), Vec2I(120, 60)) << "one side alone is no size";
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(120.0f, 60.0f, 0.5f, 256.0f), Vec2I(120, 60)) << "below a pixel is no size";
    EXPECT_EQ(PipelineComposerLayout::LayerExportSize(0.2f, 0.3f, 0.0f, 0.0f), Vec2I(1, 1)) << "at least a pixel";

    EXPECT_TRUE(PipelineComposerLayout::HasExportSize(1.0f, 1.0f));
    EXPECT_FALSE(PipelineComposerLayout::HasExportSize(0.0f, 64.0f));
    EXPECT_FALSE(PipelineComposerLayout::HasExportSize(64.0f, -1.0f));
}

TEST(PipelineComposerLayout, TypedExportSideFillsTheOtherFromTheLayer)
{
    // Nothing set: the other side follows the layer's 2:1 proportions, locked or not
    float w = 0.0f, h = 0.0f;
    PipelineComposerLayout::TypeExportSide(200.0f, 100.0f, false, true, 512.0f, w, h);
    EXPECT_FLOAT_EQ(w, 512.0f);
    EXPECT_FLOAT_EQ(h, 256.0f);

    // A size is set and the aspect is free: only the typed side changes
    PipelineComposerLayout::TypeExportSide(200.0f, 100.0f, false, false, 300.0f, w, h);
    EXPECT_FLOAT_EQ(w, 512.0f);
    EXPECT_FLOAT_EQ(h, 300.0f);

    // Locked: the other side follows again
    PipelineComposerLayout::TypeExportSide(200.0f, 100.0f, true, false, 64.0f, w, h);
    EXPECT_FLOAT_EQ(w, 128.0f);
    EXPECT_FLOAT_EQ(h, 64.0f);

    PipelineComposerLayout::TypeExportSide(200.0f, 3.0f, true, false, 1.0f, w, h);
    EXPECT_FLOAT_EQ(w, 67.0f);
    PipelineComposerLayout::TypeExportSide(3.0f, 200.0f, true, false, 2.0f, w, h);
    EXPECT_FLOAT_EQ(w, 1.0f) << "the filled side is at least a pixel";

    // Below 1, an emptied field included, clears both
    PipelineComposerLayout::TypeExportSide(200.0f, 100.0f, true, true, 0.0f, w, h);
    EXPECT_FLOAT_EQ(w, 0.0f);
    EXPECT_FLOAT_EQ(h, 0.0f);
}

TEST(PipelineComposerLayout, ANewLayerGoesInFrontOrBehindAndIsSelected)
{
    // Resolved order without "layerOrder": the inputs, back to front
    auto node = Composer({ "a", "b", "c" });
    EXPECT_EQ(PipelineComposerLayout::OrderWithNewLayer(node, "n", PipelineLayerPlace::Front), (Vector<String>{ "a", "b", "c", "n" }));
    EXPECT_EQ(PipelineComposerLayout::OrderWithNewLayer(node, "n", PipelineLayerPlace::Back), (Vector<String>{ "n", "a", "b", "c" }));

    // The stored order is kept; the new input, already a port, is not listed twice
    SetOrder(node, { "c", "a", "b" });
    node.inputs.Add(PipelinePort("n", "", PipelinePortType::Image, true));
    EXPECT_EQ(PipelineComposerLayout::OrderWithNewLayer(node, "n", PipelineLayerPlace::Back), (Vector<String>{ "n", "c", "a", "b" }));

    PipelineComposerLayout::PlaceNewLayer(node, "n", PipelinePortType::Image, PipelineLayerPlace::Back);
    EXPECT_EQ(StoredOrder(node), (Vector<String>{ "n", "c", "a", "b" }));
    EXPECT_EQ(node.GetConfigString("selectedLayer", ""), String("n"));
    auto resolved = ResolveComposerLayers(node);
    ASSERT_EQ(resolved.Count(), 4);
    EXPECT_EQ(resolved[0].id, String("n")) << "drawn first: behind every layer";

    node.inputs.Add(PipelinePort("m", "", PipelinePortType::Image, true));
    PipelineComposerLayout::PlaceNewLayer(node, "m", PipelinePortType::Image, PipelineLayerPlace::Front);
    EXPECT_EQ(StoredOrder(node), (Vector<String>{ "n", "c", "a", "b", "m" }));
    EXPECT_EQ(node.GetConfigString("selectedLayer", ""), String("m"));
}

TEST(PipelineComposerLayout, OnlyAComposerImageInputIsALayer)
{
    auto node = Composer({ "a" });
    node.inputs.Add(PipelinePort("t", "", PipelinePortType::Text, true));
    PipelineComposerLayout::PlaceNewLayer(node, "t", PipelinePortType::Text, PipelineLayerPlace::Front);
    EXPECT_FALSE(node.GetConfigValue("layerOrder"));
    EXPECT_EQ(node.GetConfigString("selectedLayer", ""), String());

    PipelineNode other;
    other.nodeType = "imageEdit";
    other.inputs.Add(PipelinePort("r", "", PipelinePortType::Image, true));
    PipelineComposerLayout::PlaceNewLayer(other, "r", PipelinePortType::Image, PipelineLayerPlace::Back);
    EXPECT_FALSE(other.GetConfigValue("layerOrder"));
    EXPECT_FALSE(other.GetConfigValue("selectedLayer"));
}

// The list: "+ layer on top", the rows, "+ layer below"; the input ports sit on the middles of the rows
TEST(PipelineComposerLayout, RowsStartUnderTheTopAddRow)
{
    using namespace PipelineComposerLayout;
    const float listTop = border + headHeight + listPadding;
    EXPECT_FLOAT_EQ(listTop, 27.0f);

    auto rows = Rows(3);
    EXPECT_FLOAT_EQ(rows.addTopRow, listTop);
    ASSERT_EQ(rows.rowTops.Count(), 3);
    EXPECT_FLOAT_EQ(rows.rowTops[0], listTop + addRowHeight + rowGap) << "the first row comes after the top add row";
    EXPECT_FLOAT_EQ(rows.rowTops[0], 54.0f);
    EXPECT_FLOAT_EQ(rows.rowTops[1], 85.0f);
    EXPECT_FLOAT_EQ(rows.rowTops[2], 116.0f);
    EXPECT_FLOAT_EQ(rows.addBottomRow, 147.0f);
    EXPECT_FLOAT_EQ(rows.height, 147.0f + addRowHeight + listPadding + border);

    // Open settings push the rows below them: one line more than before the export size
    EXPECT_FLOAT_EQ(SettingsHeight(false), 101.0f);
    EXPECT_FLOAT_EQ(SettingsHeight(true), 170.0f);
    auto open = Rows(3, 1, false);
    EXPECT_FLOAT_EQ(open.rowTops[1], 85.0f);
    EXPECT_FLOAT_EQ(open.settingsTop, 113.0f);
    EXPECT_FLOAT_EQ(open.rowTops[2], 116.0f + 101.0f);
    EXPECT_FLOAT_EQ(open.addBottomRow, 147.0f + 101.0f);
    EXPECT_FLOAT_EQ(Rows(3, 1, true).addBottomRow, 147.0f + 170.0f);

    // No layers: the hint stands between the two add rows
    auto empty = Rows(0);
    EXPECT_TRUE(empty.rowTops.IsEmpty());
    EXPECT_FLOAT_EQ(empty.emptyTop, 54.0f);
    EXPECT_FLOAT_EQ(empty.addBottomRow, 54.0f + rowHeight + rowGap);
}

// k = clamp(1 / camera zoom, 0.5, 4): the frame keeps its size on screen between zoom 1/4 and 2
TEST(PipelineComposerLayout, FrameScaleFollowsTheZoomWithinLimits)
{
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(1.0f/1.6f), 0.625f) << "zoomed in: smaller on the canvas, the same on screen";
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(2.5f), 2.5f) << "zoomed out: larger on the canvas";
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(1.0f/8.0f), 0.5f) << "past zoom 2 it grows on screen";
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(10.0f), 4.0f) << "past zoom 1/4 it shrinks on screen";
    EXPECT_FLOAT_EQ(PipelineComposerLayout::FrameScale(0.0f), 1.0f) << "no camera yet";
}
