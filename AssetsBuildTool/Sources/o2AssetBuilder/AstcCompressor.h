#pragma once

#include "o2/Utils/Types/String.h"

namespace o2
{
    class Bitmap;

    // Encodes the R8G8B8A8 bitmap into a 4x4-block ASTC file (.astc container) with the ARM
    // astcenc encoder built into the assets builder — works on any host platform. quality is
    // 0..100 and maps to the astcenc effort presets. The engine's ASTC loader
    // (Texture::LoadASTC) reads the produced files. Returns false on errors.
    bool SaveAstc4x4(const Bitmap& bitmap, const String& fileName, int quality = 60);
}
