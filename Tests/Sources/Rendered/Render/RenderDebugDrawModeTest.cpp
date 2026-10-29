#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"

using namespace o2;

namespace
{
    Ref<Bitmap> DrawAndCapture(const Function<void()>& draw)
    {
        Ref<Bitmap> captured;
        for (int i = 0; i < 2; i++)
        {
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(0, 0, 0, 255));
            draw();
            o2Render.SetCamera(Camera());
            o2Render.End();
        }

        return captured;
    }

    // Returns the pixel by the point of the default camera, Y up from the frame center
    Color4 PixelAt(const Ref<Bitmap>& frame, int x, int y)
    {
        Vec2I size = frame->GetSize();
        int px = Math::RoundToInt(size.x/2.0f) + x, py = Math::RoundToInt(size.y/2.0f) + y;
        const UInt8* pixel = frame->GetData() + ((size_t)py*size.x + px)*4;
        return Color4((int)pixel[0], (int)pixel[1], (int)pixel[2], (int)pixel[3]);
    }

    void DrawSquare(float left, float bottom, float size, const Color4& color)
    {
        o2Render.DrawFilledPolygon({ Vec2F(left, bottom), Vec2F(left + size, bottom), Vec2F(left + size, bottom + size),
                                     Vec2F(left, bottom + size) }, color);
    }

    struct DebugModeGuard
    {
        ~DebugModeGuard() { o2Render.SetDebugDrawMode(Render::DebugDrawMode::Normal); }
    };
}

TEST(RenderDebugDrawMode, WireframeDrawsTheEdgesOfTheTriangles)
{
    DebugModeGuard guard;

    auto frame = DrawAndCapture([&]()
    {
        o2Render.SetDebugDrawMode(Render::DebugDrawMode::Wireframe, Color4(255, 0, 0));
        DrawSquare(-20, -20, 40, Color4(255, 255, 255));
    });

    ASSERT_TRUE(frame);
    EXPECT_EQ(o2Render.GetDebugDrawMode(), Render::DebugDrawMode::Wireframe);

    // Inside of the triangles, away from the edges and the diagonal, the background stays
    for (auto point : { Vec2I(10, 0), Vec2I(0, 10), Vec2I(-10, 0), Vec2I(0, -10) })
        EXPECT_EQ(PixelAt(frame, point.x, point.y).r, 0) << point.x << " " << point.y;

    int edge = 0;
    for (int x = 16; x <= 23; x++)
        edge += PixelAt(frame, x, 5).r > 128 ? 1 : 0;

    EXPECT_GE(edge, 1) << "the right edge is a line";
    EXPECT_EQ(PixelAt(frame, 5, 5).g, 0) << "the fill color goes nowhere";
}

TEST(RenderDebugDrawMode, OverdrawAddsTheColorUpWhereTrianglesOverlap)
{
    DebugModeGuard guard;

    auto frame = DrawAndCapture([&]()
    {
        o2Render.SetDebugDrawMode(Render::DebugDrawMode::Overdraw, Color4(40, 20, 10));
        DrawSquare(-30, -10, 30, Color4(255, 255, 255));
        DrawSquare(-10, -10, 30, Color4(0, 0, 255, 128));
    });

    ASSERT_TRUE(frame);

    Color4 single = PixelAt(frame, -20, 0), overlap = PixelAt(frame, -5, 0), outside = PixelAt(frame, 40, 0);
    EXPECT_NEAR(single.r, 40, 2);
    EXPECT_NEAR(single.g, 20, 2);
    EXPECT_NEAR(overlap.r, 80, 3) << "two layers add up";
    EXPECT_NEAR(overlap.g, 40, 3);
    EXPECT_EQ(outside.r, 0);
}

TEST(RenderDebugDrawMode, LinesAndNormalModeAreDrawnAsTheyAre)
{
    DebugModeGuard guard;

    auto frame = DrawAndCapture([&]()
    {
        o2Render.SetDebugDrawMode(Render::DebugDrawMode::Wireframe, Color4(255, 0, 0));
        o2Render.DrawLine(Vec2F(-30, 30.5f), Vec2F(30, 30.5f), Color4(0, 255, 0));

        o2Render.SetDebugDrawMode(Render::DebugDrawMode::Normal);
        DrawSquare(-10, -10, 20, Color4(0, 0, 255));
    });

    ASSERT_TRUE(frame);

    int green = 0;
    for (int y = 28; y <= 33; y++)
        green += PixelAt(frame, 0, y).g > 128 ? 1 : 0;

    EXPECT_GE(green, 1) << "a line keeps its color";
    EXPECT_GT(PixelAt(frame, 0, 0).b, 200) << "the normal mode fills the triangles";
}
