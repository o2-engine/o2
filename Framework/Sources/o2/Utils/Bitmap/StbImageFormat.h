#pragma once

#include "o2/Utils/Types/String.h"

namespace o2
{
    class Bitmap;

    // Decodes JPEG (and other stb_image formats) from memory into an RGBA bitmap, rows bottom-up like the PNG loader
    bool LoadStbImageFromMemory(const UInt8* data, UInt size, Bitmap* image, bool errors = true);

    // Reads the file and decodes it like LoadStbImageFromMemory, logs a message on failure when errors is true
    bool LoadStbImage(const String& fileName, Bitmap* image, bool errors = true);
}
