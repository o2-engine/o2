#include "CommonTextures.h"

#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{
    void CommonTextures::Initialize()
    {
        static int themeVersion = -1;
        if (checkedBackground && themeVersion == EditorTheme::GetVersion())
            return;

        themeVersion = EditorTheme::GetVersion();

        auto& palette = EditorTheme::Get();
        Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
        backLayerBitmap.Fill(palette.checkerA);
        backLayerBitmap.FillRect(0, 10, 10, 00, palette.checkerB);
        backLayerBitmap.FillRect(10, 20, 20, 10, palette.checkerB);
        checkedBackground = TextureRef(backLayerBitmap);
    }

    TextureRef CommonTextures::checkedBackground;
}