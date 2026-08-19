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
    // Сценарий редакторской обводки: сцена рисуется в таргет, затем для маски силуэта
    // биндится другой таргет, чистится и — если у выделенного объекта нет рисуемой
    // геометрии — в него ничего не попадает. После возврата к таргету сцены clear маски
    // не должен утекать на него следующим батчем
    Ref<Bitmap> RenderSceneWithEmptyMaskPass(bool multithreaded)
    {
        bool wasEnabled = o2Render.IsMultithreadedRenderEnabled();
        o2Render.SetMultithreadedRenderEnabled(multithreaded);

        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4::Black());
        o2Render.SetCamera(Camera());

        TextureRef sceneTarget(Vec2I(256, 256), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
        TextureRef maskTarget(Vec2I(256, 256), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);

        o2Render.PushRenderTargets({ sceneTarget });
        o2Render.Clear(Color4::Green());
        o2Render.SetCamera(Camera());

        Sprite content;
        content.SetColor(Color4::Red());
        content.SetSize(Vec2F(128.0f, 128.0f));
        content.SetPosition(Vec2F(0.0f, 0.0f));
        content.Draw();

        o2Render.PushRenderTargets({ maskTarget });
        o2Render.Clear(Color4(0, 0, 0, 0));
        // рисуемых компонентов нет — в маску ничего не попало
        o2Render.PopRenderTargets();

        // батч после возврата (в редакторе — квад обводки и рамки выделения)
        Sprite frame;
        frame.SetColor(Color4::Blue());
        frame.SetSize(Vec2F(24.0f, 24.0f));
        frame.SetPosition(Vec2F(100.0f, 100.0f));
        frame.Draw();

        o2Render.PopRenderTargets();
        o2Render.SetCamera(Camera());

        Sprite composite(sceneTarget, RectI(0, 0, 256, 256));
        composite.SetSize(Vec2F(256.0f, 256.0f));
        composite.SetPosition(Vec2F(0.0f, 0.0f));
        composite.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        o2Render.SetMultithreadedRenderEnabled(wasEnabled);
        return captured;
    }

    void ExpectScenePreserved(const Ref<Bitmap>& captured)
    {
        ASSERT_TRUE(captured);

        Vec2I size = captured->GetSize();
        const UInt8* data = captured->GetData();
        auto pixel = [&](int x, int y)
        {
            const UInt8* p = &data[(y*size.x + x)*4];
            return Color4((int)p[0], (int)p[1], (int)p[2], (int)p[3]);
        };

        int cx = size.x/2, cy = size.y/2;

        // капча может быть в физических пикселях (retina) — офсеты в логических единицах
        float scale = (float)size.x/(float)o2Render.GetResolution().x;
        int bgOffset = (int)(100.0f*scale); // фон: вне красного контента (64), внутри композита (128)

        // центр — красный контент, нарисованный до прохода маски
        Color4 center = pixel(cx, cy);
        EXPECT_GT(center.r, 150) << "содержимое таргета сцены стёрто утёкшим clear";
        EXPECT_LT(center.g, 100);

        // фон таргета сцены по бокам от контента — зелёная заливка
        Color4 left = pixel(cx - bgOffset, cy);
        Color4 right = pixel(cx + bgOffset, cy);
        EXPECT_GT(left.g, 150) << "фон таргета сцены стёрт утёкшим clear";
        EXPECT_LT(left.r, 100);
        EXPECT_GT(right.g, 150);
        EXPECT_LT(right.r, 100);
    }
}

TEST(PendingClearOnTargetSwitch, SceneTargetSurvivesEmptyMaskPass)
{
    ExpectScenePreserved(RenderSceneWithEmptyMaskPass(false));
}

TEST(PendingClearOnTargetSwitch, SceneTargetSurvivesEmptyMaskPassMultithreaded)
{
    if (!Render::IsMultithreadedRenderSupported())
        GTEST_SKIP();

    ExpectScenePreserved(RenderSceneWithEmptyMaskPass(true));
}
