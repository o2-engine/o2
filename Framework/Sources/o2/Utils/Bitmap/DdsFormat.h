#pragma once

#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Bitmap;

    // Encodes the R8G8B8A8 bitmap into a block-compressed DDS file with the built-in encoders
    // (stb_dxt for DXT1/DXT5, bc7enc for BC7) — works on any host platform, no external tools.
    // BC7 files get the DX10 extended header. quality is 0..100 and maps to the encoder effort.
    // The engine's DDS loader (Texture::LoadDDS) reads the produced files.
    // Returns false on IO errors or an unsupported format.
    bool SaveDds(const Bitmap& bitmap, const String& fileName, TextureFormat format, int quality = 100);
}
