#include "o2/stdafx.h"
#include "StbImageFormat.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#include "3rdPartyLibs/stb/stb_image.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/File.h"

#include <cstring>

namespace o2
{
    bool LoadStbImageFromMemory(const UInt8* data, UInt size, Bitmap* image, bool errors /*= true*/)
    {
        int width = 0, height = 0, components = 0;
        stbi_uc* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &components, 4);
        if (!pixels)
        {
            if (errors)
                o2Debug.LogError(String("Can't decode image: ") + stbi_failure_reason());
            return false;
        }

        image->Create(PixelFormat::R8G8B8A8, Vec2I(width, height));
        UInt8* target = image->GetData();
        size_t rowBytes = (size_t)width * 4;
        for (int y = 0; y < height; y++)
            std::memcpy(target + (size_t)(height - 1 - y) * rowBytes, pixels + (size_t)y * rowBytes, rowBytes);

        stbi_image_free(pixels);
        return true;
    }

    bool LoadStbImage(const String& fileName, Bitmap* image, bool errors /*= true*/)
    {
        InFile file(fileName);
        if (!file.IsOpened())
        {
            if (errors)
                o2Debug.LogError("Can't open image file '" + fileName + "'");
            return false;
        }

        UInt size = file.GetDataSize();
        Vector<UInt8> bytes;
        bytes.Resize((int)size);
        file.ReadData(bytes.Data(), size);
        return LoadStbImageFromMemory(bytes.Data(), size, image, errors);
    }
}
