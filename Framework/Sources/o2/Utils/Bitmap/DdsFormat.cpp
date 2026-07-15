#include "o2/stdafx.h"
#include "DdsFormat.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/Math/Math.h"

#define STB_DXT_STATIC
#define STB_DXT_IMPLEMENTATION
#include "3rdPartyLibs/stb/stb_dxt.h"

#include "3rdPartyLibs/bc7enc/bc7enc.cpp" // single-translation-unit build, like the stb headers

namespace o2
{
    bool SaveDds(const Bitmap& bitmap, const String& fileName, TextureFormat format, int quality /*= 100*/)
    {
        if (format != TextureFormat::DXT1 && format != TextureFormat::DXT5 && format != TextureFormat::BC7)
            return false;

        if (bitmap.GetFormat() != PixelFormat::R8G8B8A8 || !const_cast<Bitmap&>(bitmap).GetData())
            return false;

        Vec2I size = bitmap.GetSize();
        if (size.x <= 0 || size.y <= 0)
            return false;

        quality = Math::Clamp(quality, 0, 100);

        int blockSize = format == TextureFormat::DXT1 ? 8 : 16;
        int blocksX = (size.x + 3)/4;
        int blocksY = (size.y + 3)/4;
        int dataSize = blocksX*blocksY*blockSize;

        // BC7 requires the DX10 extended header after the classic 128-byte one
        bool dx10 = format == TextureFormat::BC7;
        int headerSize = dx10 ? 148 : 128;

        Vector<UInt8> out;
        out.resize(headerSize + dataSize);

        auto writeUInt = [&](int offset, UInt value) { memcpy(out.data() + offset, &value, 4); };

        // DDS_HEADER for a single no-mipmaps surface; the layout matches what Texture::LoadDDS
        // reads back (height @12, width @16, linear size @20, fourCC @84)
        writeUInt(0, 0x20534444);  // "DDS "
        writeUInt(4, 124);         // header size
        writeUInt(8, 0x81007);     // CAPS | HEIGHT | WIDTH | PIXELFORMAT | LINEARSIZE
        writeUInt(12, (UInt)size.y);
        writeUInt(16, (UInt)size.x);
        writeUInt(20, (UInt)dataSize);
        writeUInt(76, 32);         // pixel format size
        writeUInt(80, 0x4);        // DDPF_FOURCC
        writeUInt(108, 0x1000);    // DDSCAPS_TEXTURE

        if (format == TextureFormat::DXT1)
            writeUInt(84, 0x31545844); // "DXT1"
        else if (format == TextureFormat::DXT5)
            writeUInt(84, 0x35545844); // "DXT5"
        else
        {
            writeUInt(84, 0x30315844); // "DX10"
            writeUInt(128, 98);        // DXGI_FORMAT_BC7_UNORM
            writeUInt(132, 3);         // D3D10_RESOURCE_DIMENSION_TEXTURE2D
            writeUInt(136, 0);         // misc flags
            writeUInt(140, 1);         // array size
            writeUInt(144, 0);         // misc flags 2
        }

        bc7enc_compress_block_params bc7Params;
        if (format == TextureFormat::BC7)
        {
            bc7enc_compress_block_init();
            bc7enc_compress_block_params_init(&bc7Params);
            bc7Params.m_uber_level = Math::Clamp(quality*BC7ENC_MAX_UBER_LEVEL/100, 0, BC7ENC_MAX_UBER_LEVEL);
        }

        int stbMode = quality >= 50 ? STB_DXT_HIGHQUAL : STB_DXT_NORMAL;

        // The bitmap stores rows bottom-up; DDS stores the image top-down, so rows are read
        // mirrored. Edge blocks of non-multiple-of-4 images are padded by clamping.
        const UInt8* src = const_cast<Bitmap&>(bitmap).GetData();
        UInt8 block[4*4*4];
        for (int by = 0; by < blocksY; by++)
        {
            for (int bx = 0; bx < blocksX; bx++)
            {
                for (int py = 0; py < 4; py++)
                {
                    int imageY = Math::Min(by*4 + py, size.y - 1);
                    int srcRow = size.y - 1 - imageY;
                    for (int px = 0; px < 4; px++)
                    {
                        int x = Math::Min(bx*4 + px, size.x - 1);
                        memcpy(&block[(py*4 + px)*4], &src[(srcRow*size.x + x)*4], 4);
                    }
                }

                UInt8* dest = out.data() + headerSize + (by*blocksX + bx)*blockSize;
                if (format == TextureFormat::DXT1)
                    stb_compress_dxt_block(dest, block, 0, stbMode);
                else if (format == TextureFormat::DXT5)
                    stb_compress_dxt_block(dest, block, 1, stbMode);
                else
                    bc7enc_compress_block(dest, block, &bc7Params);
            }
        }

        OutFile file(fileName);
        if (!file.IsOpened())
            return false;

        file.WriteData(out.data(), (UInt)out.size());
        return true;
    }
}
