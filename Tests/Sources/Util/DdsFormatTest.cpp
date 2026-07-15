#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/DdsFormat.h"

using namespace o2;

namespace
{
    Vector<UInt8> ReadAll(const char* path)
    {
        Vector<UInt8> data;
        FILE* f = fopen(path, "rb");
        if (!f)
            return data;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        data.resize(size);
        fread(data.data(), 1, size, f);
        fclose(f);
        return data;
    }

    UInt UIntAt(const Vector<UInt8>& data, int offset)
    {
        UInt value;
        memcpy(&value, data.data() + offset, 4);
        return value;
    }

    Vector<UInt8> EncodeToDds(TextureFormat format, const Vec2I& size, int quality = 100)
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, size);
        bitmap.Fill(Color4(255, 0, 0, 255));

        const char* path = "dds_format_test_tmp.dds";
        if (!SaveDds(bitmap, path, format, quality))
            return {};

        auto data = ReadAll(path);
        remove(path);
        return data;
    }
}

TEST(DdsFormat, WritesValidDxt5File)
{
    auto data = EncodeToDds(TextureFormat::DXT5, Vec2I(64, 64));

    // 16x16 blocks of 16 bytes after the 128-byte header
    ASSERT_EQ((int)data.size(), 128 + 16*16*16);
    EXPECT_EQ(UIntAt(data, 0), 0x20534444u);  // "DDS "
    EXPECT_EQ(UIntAt(data, 4), 124u);
    EXPECT_EQ(UIntAt(data, 12), 64u);          // height
    EXPECT_EQ(UIntAt(data, 16), 64u);          // width
    EXPECT_EQ(UIntAt(data, 20), 4096u);        // linear size
    EXPECT_EQ(UIntAt(data, 84), 0x35545844u);  // fourCC "DXT5"
}

TEST(DdsFormat, WritesValidDxt1File)
{
    auto data = EncodeToDds(TextureFormat::DXT1, Vec2I(64, 64));

    // DXT1 blocks are 8 bytes
    ASSERT_EQ((int)data.size(), 128 + 16*16*8);
    EXPECT_EQ(UIntAt(data, 20), 2048u);        // linear size
    EXPECT_EQ(UIntAt(data, 84), 0x31545844u);  // fourCC "DXT1"
}

TEST(DdsFormat, WritesValidBc7FileWithDx10Header)
{
    auto data = EncodeToDds(TextureFormat::BC7, Vec2I(64, 64), 50);

    // BC7 carries the 20-byte DX10 extended header after the classic one
    ASSERT_EQ((int)data.size(), 148 + 16*16*16);
    EXPECT_EQ(UIntAt(data, 84), 0x30315844u);  // fourCC "DX10"
    EXPECT_EQ(UIntAt(data, 128), 98u);         // DXGI_FORMAT_BC7_UNORM
    EXPECT_EQ(UIntAt(data, 132), 3u);          // texture2d dimension
    EXPECT_EQ(UIntAt(data, 140), 1u);          // array size
}

TEST(DdsFormat, PadsNonMultipleOfFourSizes)
{
    auto data = EncodeToDds(TextureFormat::DXT5, Vec2I(10, 6));

    // 3x2 blocks
    ASSERT_EQ((int)data.size(), 128 + 3*2*16);
    EXPECT_EQ(UIntAt(data, 12), 6u);
    EXPECT_EQ(UIntAt(data, 16), 10u);
    EXPECT_EQ(UIntAt(data, 20), 96u);
}

TEST(DdsFormat, RejectsInvalidInput)
{
    Bitmap empty;
    EXPECT_FALSE(SaveDds(empty, "never_written.dds", TextureFormat::DXT5));

    Bitmap valid(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    EXPECT_FALSE(SaveDds(valid, "never_written.dds", TextureFormat::R8G8B8A8)); // not a compression
    EXPECT_FALSE(SaveDds(valid, "never_written.dds", TextureFormat::ASTC4x4));  // not a DDS format
}
