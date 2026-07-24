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

    // Sanity: the quad center is actually the reddish sprite color, so something really was drawn
    int center = (cy * size.x + cx) * 4;
    EXPECT_GT((int)multiData[center + 0], 150);
    EXPECT_LT((int)multiData[center + 1], 110);
    EXPECT_LT((int)multiData[center + 2], 90);
}
