#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePairViews.h"

using namespace o2;
using namespace Editor;

// Input | result layout of the image-to-image nodes: the extract grid geometry (the same numbers as AssetsLine's
// extractPair / cellCorner), the height rules and the local view setting

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-pair-work-" + (String)(int)Math::Random(0, 1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
        }

        ~WorkDirGuard()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    // The body of a card leaves 10 px on each side
    PipelineExtractGrid GridOfCard(float cardWidth, int parts, float rowHeight = 0.0f)
    {
        return PipelinePairLayout::ExtractGrid(cardWidth - 20.0f, parts, rowHeight);
    }

    void ExpectCorner(const PipelineExtractGrid& grid, int index, float x, float y)
    {
        Vec2F corner = PipelinePairLayout::ExtractCellCorner(grid, index);
        EXPECT_NEAR(corner.x, x, 0.01f) << "part " << index;
        EXPECT_NEAR(corner.y, y, 0.01f) << "part " << index;
    }
}

TEST(PipelinePairLayout, PairNodesAndTheirInputs)
{
    for (auto type : { "imageEdit", "imageExtract", "aiRemoveBg", "removeBackground", "imageOutline", "imageShadow", "imageGradient", "imageColor" })
        EXPECT_TRUE(PipelinePairLayout::IsPairNode(type)) << type;

    for (auto type : { "nanoBananaGen", "sourceImage", "finishImage", "imageComposer", "aiText", "drawImage" })
        EXPECT_FALSE(PipelinePairLayout::IsPairNode(type)) << type;

    EXPECT_EQ(PipelinePairLayout::InputPortOf("removeBackground"), String("white"));
    EXPECT_EQ(PipelinePairLayout::InputPortOf("imageEdit"), String("image"));
    EXPECT_EQ(PipelinePairLayout::InputPortOf("imageColor"), String("image"));

    EXPECT_FLOAT_EQ(PipelinePairLayout::PaneWidth(460.0f), 227.0f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::PairRowHeight(true), 176.0f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::PairRowHeight(false), 64.0f);
}

TEST(PipelinePairLayout, ExtractGridAtWidth480)
{
    auto one = GridOfCard(480.0f, 1);
    EXPECT_FLOAT_EQ(one.paneW, 227.0f);
    EXPECT_EQ(one.cols, 1);
    EXPECT_EQ(one.rows, 1);
    EXPECT_FLOAT_EQ(one.cellW, 227.0f);
    EXPECT_FLOAT_EQ(one.labelH, 0.0f) << "a single part has no caption";
    EXPECT_FLOAT_EQ(one.rowNatural, 176.0f);
    EXPECT_FLOAT_EQ(one.cellH, 176.0f);
    ExpectCorner(one, 0, 460.0f, 176.0f);

    auto three = GridOfCard(480.0f, 3);
    EXPECT_EQ(three.cols, 3);
    EXPECT_EQ(three.rows, 1);
    EXPECT_NEAR(three.cellW, 71.667f, 0.01f);
    EXPECT_FLOAT_EQ(three.labelH, 20.0f);
    EXPECT_FLOAT_EQ(three.rowNatural, 176.0f) << "never lower than the stage";
    EXPECT_FLOAT_EQ(three.cellH, 156.0f) << "the cells stretch to the row";
    ExpectCorner(three, 0, 304.667f, 156.0f);
    ExpectCorner(three, 2, 460.0f, 156.0f);

    auto eight = GridOfCard(480.0f, 8);
    EXPECT_EQ(eight.cols, 3);
    EXPECT_EQ(eight.rows, 3);
    EXPECT_FLOAT_EQ(eight.rowNatural, 288.0f);
    EXPECT_FLOAT_EQ(eight.cellH, 72.0f);
    ExpectCorner(eight, 7, 382.333f, 268.0f);
}

TEST(PipelinePairLayout, ExtractGridAtWidth312)
{
    auto one = GridOfCard(312.0f, 1);
    EXPECT_FLOAT_EQ(one.paneW, 143.0f);
    EXPECT_EQ(one.cols, 1);
    EXPECT_FLOAT_EQ(one.rowNatural, 176.0f);
    EXPECT_FLOAT_EQ(one.cellH, 176.0f);
    ExpectCorner(one, 0, 292.0f, 176.0f);

    auto three = GridOfCard(312.0f, 3);
    EXPECT_EQ(three.cols, 2) << "cells no narrower than 64 px";
    EXPECT_EQ(three.rows, 2);
    EXPECT_FLOAT_EQ(three.cellW, 68.5f);
    EXPECT_FLOAT_EQ(three.rowNatural, 184.0f) << "two rows of 69 px cells with captions";
    EXPECT_FLOAT_EQ(three.cellH, 69.0f);
    ExpectCorner(three, 1, 292.0f, 69.0f);
    ExpectCorner(three, 2, 217.5f, 164.0f);

    auto eight = GridOfCard(312.0f, 8);
    EXPECT_EQ(eight.cols, 2);
    EXPECT_EQ(eight.rows, 4);
    EXPECT_FLOAT_EQ(eight.rowNatural, 374.0f);
    EXPECT_FLOAT_EQ(eight.cellH, 69.0f);
    ExpectCorner(eight, 7, 292.0f, 354.0f);
}

// A hand-sized card gives the row its extra height; the cells take all of it
TEST(PipelinePairLayout, ExtractGridStretchesToATallerRow)
{
    auto tall = GridOfCard(480.0f, 3, 300.0f);
    EXPECT_FLOAT_EQ(tall.rowNatural, 176.0f);
    EXPECT_FLOAT_EQ(tall.rowH, 300.0f);
    EXPECT_FLOAT_EQ(tall.cellH, 280.0f);
    ExpectCorner(tall, 1, 382.333f, 280.0f);

    auto low = GridOfCard(480.0f, 8, 100.0f);
    EXPECT_FLOAT_EQ(low.rowH, 288.0f) << "never lower than the natural height";
}

// Height 0 means "width set, height follows the content", everywhere a height is read or written
TEST(PipelinePairLayout, AZeroHeightIsAutomatic)
{
    EXPECT_TRUE(PipelinePairLayout::IsAutoHeight(Vec2F(480.0f, 0.0f)));
    EXPECT_TRUE(PipelinePairLayout::IsAutoHeight(Vec2F()));
    EXPECT_FALSE(PipelinePairLayout::IsAutoHeight(Vec2F(480.0f, 320.0f)));

    EXPECT_EQ(PipelinePairLayout::NewNodeSize("imageEdit", Vec2F()), Vec2F(480.0f, 0.0f));
    EXPECT_EQ(PipelinePairLayout::NewNodeSize("removeBackground", Vec2F(300.0f, 200.0f)), Vec2F(480.0f, 0.0f));
    EXPECT_EQ(PipelinePairLayout::NewNodeSize("nanoBananaGen", Vec2F(300.0f, 0.0f)), Vec2F(300.0f, 0.0f));

    // A side edge keeps an automatic height; a top or bottom edge sets one
    EXPECT_FLOAT_EQ(PipelinePairLayout::HeightAfterResize(false, 0.0f, 412.0f), 0.0f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::HeightAfterResize(true, 0.0f, 412.0f), 412.0f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::HeightAfterResize(false, 380.0f, 412.0f), 412.0f) << "a set height stays set";
}

TEST(PipelinePairLayout, CompareNeedsBothImagesAndNoCrop)
{
    auto compare = PipelineIoView::Compare;
    EXPECT_TRUE(PipelinePairLayout::ShowsCompare(compare, "imageColor", true, true, false));
    EXPECT_FALSE(PipelinePairLayout::ShowsCompare(PipelineIoView::SideBySide, "imageColor", true, true, false));
    EXPECT_FALSE(PipelinePairLayout::ShowsCompare(compare, "imageColor", false, true, false));
    EXPECT_FALSE(PipelinePairLayout::ShowsCompare(compare, "imageColor", true, false, false));
    EXPECT_FALSE(PipelinePairLayout::ShowsCompare(compare, "aiRemoveBg", true, true, true)) << "a crop frame needs the whole result pane";
    EXPECT_FALSE(PipelinePairLayout::ShowsCompare(compare, "imageExtract", true, true, false)) << "one source, many parts";
}

TEST(PipelinePairLayout, TheViewSettingIsStoredLocally)
{
    WorkDirGuard work;
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::SideBySide);
    EXPECT_TRUE(PipelinePairLayout::GetViewPrefsPath().StartsWith(work.path));

    int version = PipelinePairLayout::GetIoViewVersion();
    PipelinePairLayout::SetIoView(PipelineIoView::Compare);
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::Compare);
    EXPECT_NE(PipelinePairLayout::GetIoViewVersion(), version) << "open editors see the change";
    EXPECT_TRUE(PipelineUtils::ReadFileBytes(PipelinePairLayout::GetViewPrefsPath()).Contains("compare"));

    // Another work folder has its own setting; coming back reads the stored one
    {
        WorkDirGuard other;
        EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::SideBySide);
    }
    PipelineUtils::SetWorkPathOverride(work.path);
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::Compare);

    PipelinePairLayout::SetIoView(PipelineIoView::SideBySide);
    EXPECT_EQ(PipelinePairLayout::GetIoView(), PipelineIoView::SideBySide);
}

// The divider is kept per node for the session, clamped to the view
TEST(PipelinePairLayout, TheDividerIsKeptPerNode)
{
    EXPECT_FLOAT_EQ(PipelinePairLayout::GetDivider("divider-test-a"), 0.5f);
    PipelinePairLayout::SetDivider("divider-test-a", 0.25f);
    PipelinePairLayout::SetDivider("divider-test-b", 1.7f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::GetDivider("divider-test-a"), 0.25f);
    EXPECT_FLOAT_EQ(PipelinePairLayout::GetDivider("divider-test-b"), 1.0f);
}

// The divider grip keeps its screen size at any zoom, but stops growing on the canvas when zoomed far out
TEST(PipelinePairLayout, TheDividerGripIsScreenSized)
{
    RectF box(0.0f, 100.0f, 200.0f, 0.0f);
    for (float pixel : { 1.0f, 1.0f/3.0f })
    {
        RectF grip = PipelinePairDraw::GripRect(box, 50.0f, pixel);
        EXPECT_NEAR(grip.Width()/pixel, 5.0f, 0.01f) << "pixel " << pixel;
        EXPECT_NEAR(grip.Height()/pixel, 18.0f, 0.01f) << "pixel " << pixel;
        EXPECT_NEAR(grip.Center().x, 50.0f, 0.01f);
        EXPECT_NEAR(grip.Center().y, 50.0f, 0.01f);
    }

    RectF far = PipelinePairDraw::GripRect(box, 50.0f, 3.0f);
    EXPECT_NEAR(far.Width(), 7.5f, 0.01f) << "never more than 1.5x on the canvas";
    EXPECT_NEAR(far.Height(), 27.0f, 0.01f);
    EXPECT_FLOAT_EQ(PipelinePairDraw::BadgeScale(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(PipelinePairDraw::BadgeScale(3.0f), 1.5f);
}
