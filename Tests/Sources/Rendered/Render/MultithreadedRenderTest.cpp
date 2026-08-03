#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Color.h"

using namespace o2;

namespace
{
    // Renders one deterministic frame (a solid colored quad) and returns the captured back buffer
    Ref<Bitmap> RenderColoredQuad()
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4::Black());
        o2Render.SetCamera(Camera());

        Sprite sprite;
        sprite.SetColor(Color4(200, 60, 30, 255));
        sprite.SetSize(Vec2F(128.0f, 128.0f));
        sprite.SetPosition(Vec2F(0.0f, 0.0f));
        sprite.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        return captured;
    }
}

// The multithreaded render path (record on main, submit on the render thread) must produce the exact
// same image as the single-threaded path — same draw calls, same order, only the submitting thread differs
TEST(MultithreadedRender, ProducesSameOutputAsSingleThreaded)
{
    bool wasEnabled = o2Render.IsMultithreadedRenderEnabled();

    o2Render.SetMultithreadedRenderEnabled(false);
    Ref<Bitmap> single = RenderColoredQuad();
    ASSERT_TRUE(single);

    o2Render.SetMultithreadedRenderEnabled(true);
    Ref<Bitmap> multi = RenderColoredQuad();
    ASSERT_TRUE(multi);

    o2Render.SetMultithreadedRenderEnabled(wasEnabled);

    ASSERT_EQ(single->GetSize(), multi->GetSize());

    Vec2I size = single->GetSize();
    const UInt8* singleData = single->GetData();
    const UInt8* multiData = multi->GetData();

    int cx = size.x / 2;
    int cy = size.y / 2;

    // Sample a grid around the quad and require the two images to match (tiny tolerance for any driver
    // rounding, though identical inputs render identically)
    int mismatches = 0;
    for (int dy = -48; dy <= 48; dy += 8)
    {
        for (int dx = -48; dx <= 48; dx += 8)
        {
            int index = ((cy + dy) * size.x + (cx + dx)) * 4;
            for (int channel = 0; channel < 4; channel++)
            {
                if (Math::Abs((int)singleData[index + channel] - (int)multiData[index + channel]) > 2)
                    mismatches++;
            }
        }
    }
    EXPECT_EQ(mismatches, 0);

    // Sanity: the quad center is actually the reddish sprite color on both paths, so each of them
    // really drew the frame and the comparison above compared something
    int center = (cy * size.x + cx) * 4;
    for (const UInt8* data : { singleData, multiData })
    {
        EXPECT_GT((int)data[center + 0], 150);
        EXPECT_LT((int)data[center + 1], 110);
        EXPECT_LT((int)data[center + 2], 90);
    }
}

namespace
{
    // Renders a frame of several batches with differing state, so the replay has to carry per-command
    // state into the render pass they share
    Ref<Bitmap> RenderMixedScissorFrame()
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4::Black());
        o2Render.SetCamera(Camera());

        Sprite clipped;
        clipped.SetColor(Color4(40, 200, 90, 255));
        clipped.SetSize(Vec2F(200.0f, 200.0f));
        clipped.SetPosition(Vec2F(-200.0f, 0.0f));

        o2Render.EnableScissorTest(RectI(-200, 40, -100, -40));
        clipped.Draw();
        o2Render.DisableScissorTest();

        Sprite unclipped;
        unclipped.SetColor(Color4(200, 60, 30, 255));
        unclipped.SetSize(Vec2F(128.0f, 128.0f));
        unclipped.SetPosition(Vec2F(60.0f, 0.0f));
        unclipped.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        return captured;
    }
}

// Several batches of one frame share a single render pass on the replay side; splitting or reusing it
// wrongly changes what reaches the frame
TEST(MultithreadedRender, MultipleBatchesOfOneFrameMatchSingleThreaded)
{
    bool wasEnabled = o2Render.IsMultithreadedRenderEnabled();

    o2Render.SetMultithreadedRenderEnabled(false);
    Ref<Bitmap> single = RenderMixedScissorFrame();
    ASSERT_TRUE(single);

    o2Render.SetMultithreadedRenderEnabled(true);
    Ref<Bitmap> multi = RenderMixedScissorFrame();
    ASSERT_TRUE(multi);

    o2Render.SetMultithreadedRenderEnabled(wasEnabled);

    ASSERT_EQ(single->GetSize(), multi->GetSize());

    Vec2I size = single->GetSize();
    const UInt8* singleData = single->GetData();
    const UInt8* multiData = multi->GetData();

    int cx = size.x/2;
    int cy = size.y/2;

    int mismatches = 0;
    int greenPixels[2] = { 0, 0 }, redPixels[2] = { 0, 0 };
    const UInt8* datas[2] = { singleData, multiData };

    for (int dy = -80; dy <= 80; dy += 4)
    {
        for (int dx = -240; dx <= 240; dx += 4)
        {
            int index = ((cy + dy)*size.x + (cx + dx))*4;
            for (int channel = 0; channel < 4; channel++)
            {
                if (Math::Abs((int)singleData[index + channel] - (int)multiData[index + channel]) > 2)
                    mismatches++;
            }

            for (int path = 0; path < 2; path++)
            {
                if (datas[path][index + 1] > 150 && datas[path][index + 0] < 100)
                    greenPixels[path]++;

                if (datas[path][index + 0] > 150 && datas[path][index + 1] < 110)
                    redPixels[path]++;
            }
        }
    }
    EXPECT_EQ(mismatches, 0);

    // Sanity: both quads really are on screen on each path, so the batches of one frame reached it
    for (int path = 0; path < 2; path++)
    {
        EXPECT_GT(greenPixels[path], 0);
        EXPECT_GT(redPixels[path], 0);
    }
}
