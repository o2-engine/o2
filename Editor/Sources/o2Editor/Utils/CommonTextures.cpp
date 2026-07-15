#include "CommonTextures.h"

namespace Editor
{
    void CommonTextures::Initialize()
    {
        if (checkedBackground)
            return;

        Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
        Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
        backLayerBitmap.Fill(color1);
        backLayerBitmap.FillRect(0, 10, 10, 00, color2);
        backLayerBitmap.FillRect(10, 20, 20, 10, color2);
        checkedBackground = TextureRef(backLayerBitmap);
    }

    TextureRef CommonTextures::checkedBackground;
}