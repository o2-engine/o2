#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "PipelinePairUiFixture.h"

// "Show the whole pipeline": the camera flies to fit every card, never zooming in past the natural size

namespace
{
    struct PipelineFitViewFixture : PipelinePairUiFixture
    {
        String WorkDir() const override { return "../../Work/Pipelines/uitest-fitview/"; }

        void Open(const Vector<Vec2F>& positions)
        {
            PipelineGraph graph;
            graph.id = "uitest-fitview";
            for (auto& p : positions)
                graph.nodes.Add(PipelineNodeRegistry::CreateNode("sourceText", p));
            graph.SaveToAsset(*asset);
            editor->SetAsset(asset);
            Step(4);
        }

        RectF Bounds() const
        {
            RectF bounds;
            bool first = true;
            for (auto& node : editor->GetGraph()->nodes)
            {
                RectF r = editor->GetNodeWidget(node->id)->GetCardRect();
                bounds = first ? r : bounds.Expand(r);
                first = false;
            }
            return bounds;
        }
    };
}

TEST_F(PipelineFitViewFixture, TheCameraFliesToShowEveryCard)
{
    Open({ Vec2F(0, 0), Vec2F(3000, 0), Vec2F(0, -2400), Vec2F(3000, -2400) });
    editor->SetView(Vec2F(-5000, 3000), 0.5f);
    Step(2);
    Vec2F startPos = editor->GetCamera().GetPosition2D();

    editor->FitView();
    Step(6);
    float midScale = editor->GetCamera().GetScale2D().x;
    Vec2F midPos = editor->GetCamera().GetPosition2D();
    EXPECT_GT(midScale, 0.5f) << "on its way";
    EXPECT_NE(midPos, startPos);

    Step(30);
    RectF bounds = Bounds();
    Vec2F pos = editor->GetCamera().GetPosition2D();
    EXPECT_NEAR(pos.x, bounds.Center().x, 1.0f);
    EXPECT_NEAR(pos.y, bounds.Center().y, 1.0f);
    float scale = editor->GetCamera().GetScale2D().x;
    Vec2F view = editor->layout->GetSize2D();
    EXPECT_LE(bounds.Width()/scale, view.x - 100.0f) << "the cards fit with room around them";
    EXPECT_LE(bounds.Height()/scale, view.y - 100.0f);

    // A small pipeline is not blown up past its natural size
    Open({ Vec2F(0, 0) });
    editor->SetView(Vec2F(800, 800), 3.0f);
    Step(2);
    editor->FitView();
    Step(40);
    EXPECT_FLOAT_EQ(editor->GetCamera().GetScale2D().x, 1.0f);
    EXPECT_NEAR(editor->GetCamera().GetPosition2D().x, Bounds().Center().x, 1.0f);
}
