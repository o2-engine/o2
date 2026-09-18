#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<PipelineNode> MakeExtractNode()
    {
        return PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    }

    PipelineExtractRegion MakeRegion(const String& id, const String& name, float x, float y, float w, float h)
    {
        PipelineExtractRegion region;
        region.id = id;
        region.name = name;
        region.x = x; region.y = y; region.w = w; region.h = h;
        return region;
    }
}

TEST(PipelineRegions, NodeWithoutRegionsKeepsItsPromptAndRoiAsOnePart)
{
    auto node = MakeExtractNode();
    ASSERT_EQ(node->outputs.Count(), 1);
    node->SetConfigString("prompt", "gold coin");
    node->RemoveConfig("roi");
    auto& roi = node->config["roi"];
    roi.SetObject();
    roi["x"] = 0.25f;
    roi["y"] = 0.5f;
    roi["w"] = 0.25f;
    roi["h"] = 0.25f;

    auto regions = PipelineRegions::Read(*node);
    ASSERT_EQ(regions.Count(), 1);
    // The legacy part keeps the existing port id, so the links of an imported pipeline survive
    EXPECT_EQ(regions[0].id, node->outputs[0].id);
    EXPECT_EQ(regions[0].name, "gold coin");
    EXPECT_NEAR(regions[0].x, 0.25f, 0.0001f);
    EXPECT_NEAR(regions[0].h, 0.25f, 0.0001f);
}

TEST(PipelineRegions, WrittenRegionsComeBackAndNameThePortsUniquely)
{
    auto node = MakeExtractNode();
    Vector<PipelineExtractRegion> regions = {
        MakeRegion("a", "coin", 0.0f, 0.0f, 0.5f, 0.5f),
        MakeRegion("b", "coin", 0.5f, 0.0f, 0.5f, 0.5f),
        MakeRegion("c", "", 0.0f, 0.5f, 0.5f, 0.5f)
    };
    PipelineRegions::Write(*node, regions);

    auto read = PipelineRegions::Read(*node);
    ASSERT_EQ(read.Count(), 3);
    EXPECT_EQ(read[1].id, "b");
    EXPECT_NEAR(read[1].x, 0.5f, 0.0001f);

    auto names = PipelineRegions::PortNames(read);
    ASSERT_EQ(names.Count(), 3);
    EXPECT_EQ(names[0], "coin");
    EXPECT_EQ(names[1], "coin 2");
    EXPECT_EQ(names[2], "part 3");
}

TEST(PipelineRegions, SyncPortsKeepsPortIdsWhenAPartIsRenamed)
{
    auto node = MakeExtractNode();
    PipelineRegions::Write(*node, { MakeRegion("a", "coin", 0, 0, 0.5f, 1), MakeRegion("b", "chest", 0.5f, 0, 0.5f, 1) });
    PipelineRegions::SyncPorts(*node);

    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[0].id, "a");
    EXPECT_EQ(node->outputs[1].name, "chest");

    auto regions = PipelineRegions::Read(*node);
    regions[1].name = "treasure chest";
    PipelineRegions::Write(*node, regions);
    PipelineRegions::SyncPorts(*node);

    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[1].id, "b");
    EXPECT_EQ(node->outputs[1].name, "treasure chest");
}

TEST(PipelineRegions, RegionOfPortFallsBackToTheFirstPart)
{
    auto node = MakeExtractNode();
    PipelineRegions::Write(*node, { MakeRegion("a", "coin", 0, 0, 0.5f, 1), MakeRegion("b", "chest", 0.5f, 0, 0.5f, 1) });
    PipelineRegions::SyncPorts(*node);

    EXPECT_EQ(PipelineRegions::RegionOfPort(*node, "b").name, "chest");
    EXPECT_EQ(PipelineRegions::RegionOfPort(*node, "gone").name, "coin");
}

TEST(PipelineRegions, ParsesGeminiBoxesPadsThemAndSkipsWholeImageParts)
{
    String answer =
        "```json\n"
        "[{\"name\": \"gold coin\", \"box_2d\": [100, 200, 300, 500]},"
        " {\"name\": \"whole screen\", \"box_2d\": [0, 0, 1000, 1000]},"
        " {\"name\": \"\", \"box_2d\": [10, 10, 20, 20]},"
        " {\"name\": \"health bar\", \"box_2d\": [700, 100, 760, 400]}]\n"
        "```";

    auto regions = PipelineRegions::ParseAutoSplit(answer);
    ASSERT_EQ(regions.Count(), 2);
    EXPECT_EQ(regions[0].name, "gold coin");
    EXPECT_EQ(regions[1].name, "health bar");
    EXPECT_FALSE(regions[0].id.IsEmpty());
    EXPECT_NE(regions[0].id, regions[1].id);

    // box_2d is [ymin, xmin, ymax, xmax] on a 0..1000 grid, grown by a share of the part's own size
    EXPECT_GT(regions[0].x, 0.18f);
    EXPECT_LT(regions[0].x, 0.2f);
    EXPECT_GT(regions[0].y, 0.08f);
    EXPECT_LT(regions[0].y, 0.1f);
    EXPECT_GT(regions[0].w, 0.3f);
    EXPECT_LT(regions[0].w, 0.34f);
}

TEST(PipelineRegions, RefusesAnAnswerThatIsNotARegionList)
{
    EXPECT_TRUE(PipelineRegions::ParseAutoSplit("I could not find any parts").IsEmpty());
    EXPECT_TRUE(PipelineRegions::ParseAutoSplit("[]").IsEmpty());
}

// A long non-ASCII part name used to be cut in the middle of a letter, and the card built from it threw
TEST(PipelineRegions, LongNonAsciiNamesAreCutOnCharacterBoundaries)
{
    auto node = MakeExtractNode();
    String name = "UI подложка под кол-во очков, с пустым прогресс-баром. Без текста";
    node->SetConfigString("prompt", name);

    auto regions = PipelineRegions::Read(*node);
    ASSERT_EQ(regions.Count(), 1);
    EXPECT_EQ(regions[0].name, name);

    auto names = PipelineRegions::PortNames(regions);
    ASSERT_EQ(names.Count(), 1);
    EXPECT_LE(names[0].Length(), 40);
    EXPECT_TRUE(name.StartsWith(names[0]));
    EXPECT_NO_THROW({ WString wide = names[0]; (void)wide; });

    PipelineRegions::SyncPorts(*node);
    ASSERT_EQ(node->outputs.Count(), 1);
    EXPECT_NO_THROW({ WString wide = node->outputs[0].name; (void)wide; });

    // The same cut in the auto-split answer
    auto parsed = PipelineRegions::ParseAutoSplit(
        R"([{"name": "очень длинное название элемента интерфейса для проверки обрезки", "box_2d": [100, 100, 300, 300]}])");
    ASSERT_EQ(parsed.Count(), 1);
    EXPECT_LE(parsed[0].name.Length(), 60);
    EXPECT_NO_THROW({ WString wide = parsed[0].name; (void)wide; });
}
