#include "o2/stdafx.h"
#include "AstcCompressor.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/Math/Math.h"

#include "astcenc.h"

#include <thread>

namespace o2
{
    bool SaveAstc4x4(const Bitmap& bitmap, const String& fileName, int quality /*= 60*/)
    {
        if (bitmap.GetFormat() != PixelFormat::R8G8B8A8 || !const_cast<Bitmap&>(bitmap).GetData())
            return false;

        Vec2I size = bitmap.GetSize();
        if (size.x <= 0 || size.y <= 0)
            return false;

        // astcenc expects top-down rows; the bitmap stores them bottom-up
        Vector<UInt8> pixels;
        pixels.resize(size.x*size.y*4);
        const UInt8* src = const_cast<Bitmap&>(bitmap).GetData();
        for (int y = 0; y < size.y; y++)
            memcpy(pixels.data() + y*size.x*4, src + (size.y - 1 - y)*size.x*4, size.x*4);

        // quality 0..100 maps to FASTEST..THOROUGH; the exhaustive presets above THOROUGH
        // take minutes per atlas page and are never worth it for game textures
        float clamped = (float)Math::Clamp(quality, 0, 100);
        float effort = clamped <= 60.0f ? clamped : Math::Lerp(60.0f, 98.0f, (clamped - 60.0f)/40.0f);

        astcenc_config config;
        if (astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, effort, 0, &config) != ASTCENC_SUCCESS)
            return false;

        unsigned int threadCount = Math::Max(1u, std::thread::hardware_concurrency());

        astcenc_context* context = nullptr;
        if (astcenc_context_alloc(&config, threadCount, &context) != ASTCENC_SUCCESS)
            return false;

        void* slice = pixels.data();
        astcenc_image image;
        image.dim_x = (unsigned int)size.x;
        image.dim_y = (unsigned int)size.y;
        image.dim_z = 1;
        image.data_type = ASTCENC_TYPE_U8;
        image.data = &slice;

        astcenc_swizzle swizzle { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        int blocksX = (size.x + 3)/4;
        int blocksY = (size.y + 3)/4;
        size_t dataSize = (size_t)blocksX*blocksY*16;

        Vector<UInt8> out;
        out.resize(16 + dataSize);

        // astcenc splits the work between threads itself; every thread joins the same compression
        Vector<astcenc_error> errors;
        errors.resize(threadCount);

        Vector<std::thread> workers;
        for (unsigned int i = 0; i < threadCount; i++)
        {
            workers.emplace_back([&, i] {
                errors[i] = astcenc_compress_image(context, &image, &swizzle,
                                                   out.data() + 16, dataSize, i);
            });
        }

        for (auto& worker : workers)
            worker.join();

        astcenc_context_free(context);

        for (auto error : errors)
        {
            if (error != ASTCENC_SUCCESS)
                return false;
        }

        // .astc container: magic, block dims, then 24-bit little-endian image dims
        auto writeUInt24 = [&](int offset, int value) {
            out[offset] = (UInt8)(value & 0xFF);
            out[offset + 1] = (UInt8)((value >> 8) & 0xFF);
            out[offset + 2] = (UInt8)((value >> 16) & 0xFF);
        };

        out[0] = 0x13; out[1] = 0xAB; out[2] = 0xA1; out[3] = 0x5C;
        out[4] = 4; out[5] = 4; out[6] = 1;
        writeUInt24(7, size.x);
        writeUInt24(10, size.y);
        writeUInt24(13, 1);

        OutFile file(fileName);
        if (!file.IsOpened())
            return false;

        file.WriteData(out.data(), (UInt)out.size());
        return true;
    }
}
