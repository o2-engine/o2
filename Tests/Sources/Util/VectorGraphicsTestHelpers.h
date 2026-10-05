#pragma once

#include <gtest/gtest.h>

#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"

namespace VectorGraphicsTest
{
    using namespace o2;

    inline String Svg(int width, int height, const String& body)
    {
        return String("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"") +
            (String)width + "\" height=\"" + (String)height + "\">" + body + "</svg>";
    }

    inline VectorImage Parse(const String& svg)
    {
        VectorImage image;
        String error;
        Vector<String> warnings;
        EXPECT_TRUE(SvgParser::Parse(svg, image, error, warnings)) << error.Data();
        return image;
    }

    inline Ref<Bitmap> RenderSvg(const String& svg, float scale = 1.0f, bool antialiasing = true)
    {
        return VectorRasterizer::Rasterize(Parse(svg), scale, Color4(0, 0, 0, 0), antialiasing);
    }

    inline int Alpha(const Ref<Bitmap>& bitmap, int x, int y)
    {
        return VectorRasterizer::GetPixel(*bitmap, x, y).a;
    }

    inline void ExpectSamePixel(const Ref<Bitmap>& a, int ax, int ay, const Ref<Bitmap>& b, int bx, int by, int tolerance = 1)
    {
        Color4 pixelA = VectorRasterizer::GetPixel(*a, ax, ay), pixelB = VectorRasterizer::GetPixel(*b, bx, by);
        EXPECT_NEAR(pixelA.r, pixelB.r, tolerance) << ax << "," << ay;
        EXPECT_NEAR(pixelA.g, pixelB.g, tolerance) << ax << "," << ay;
        EXPECT_NEAR(pixelA.b, pixelB.b, tolerance) << ax << "," << ay;
        EXPECT_NEAR(pixelA.a, pixelB.a, tolerance) << ax << "," << ay;
    }

    inline int CountAlpha(const Ref<Bitmap>& bitmap, int minAlpha, int maxAlpha)
    {
        int count = 0;
        Vec2I size = bitmap->GetSize();
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                int alpha = Alpha(bitmap, x, y);
                if (alpha >= minAlpha && alpha <= maxAlpha)
                    count++;
            }
        }

        return count;
    }

    // Maximum alpha difference between the pixels mirrored around the vertical and horizontal lines through center
    inline int MirrorDifference(const Ref<Bitmap>& bitmap, float centerX, float centerY)
    {
        int res = 0;
        Vec2I size = bitmap->GetSize();
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                int mirrorX = Math::RoundToInt(centerX*2.0f) - 1 - x, mirrorY = Math::RoundToInt(centerY*2.0f) - 1 - y;
                if (mirrorX >= 0 && mirrorX < size.x)
                    res = Math::Max(res, Math::Abs(Alpha(bitmap, x, y) - Alpha(bitmap, mirrorX, y)));

                if (mirrorY >= 0 && mirrorY < size.y)
                    res = Math::Max(res, Math::Abs(Alpha(bitmap, x, y) - Alpha(bitmap, x, mirrorY)));
            }
        }

        return res;
    }

    inline double AlphaSum(const Ref<Bitmap>& bitmap)
    {
        double sum = 0;
        Vec2I size = bitmap->GetSize();
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
                sum += Alpha(bitmap, x, y)/255.0;
        }

        return sum;
    }

    struct Fidelity
    {
        BitmapCompareResult result;
        UInt                triangles = 0;
        Vector<String>      warnings;
        Ref<Bitmap>         rendered;
        Ref<Bitmap>         truth;
    };

    // Anti-aliased render against the box filter ground truth: hard edges render at samples scale, averaged down
    inline Fidelity MeasureFidelity(const String& svg, int tolerance = 24, int samples = 16)
    {

        Fidelity res;

        VectorImage image;
        String error;
        EXPECT_TRUE(SvgParser::Parse(svg, image, error, res.warnings)) << error.Data();

        Color4 background(255, 255, 255, 255);

        VectorMesh mesh;
        VectorTessellator::Tessellate(image, mesh);
        res.triangles = mesh.GetTrianglesCount();
        res.rendered = VectorRasterizer::Rasterize(mesh, 1.0f, background);

        Ref<Bitmap> large = VectorRasterizer::Rasterize(image, (float)samples, background, false);
        Vec2I size = res.rendered->GetSize();
        EXPECT_EQ(large->GetSize(), size*samples);

        res.truth = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                int sums[4] = { 0, 0, 0, 0 };
                for (int sy = 0; sy < samples; sy++)
                {
                    const UInt8* row = large->GetData() + ((size_t)(y*samples + sy)*size.x*samples + x*samples)*4;
                    for (int i = 0; i < samples*4; i++)
                        sums[i%4] += row[i];
                }

                UInt8* pixel = res.truth->GetData() + ((size_t)y*size.x + x)*4;
                for (int channel = 0; channel < 4; channel++)
                    pixel[channel] = (UInt8)((sums[channel] + samples*samples/2)/(samples*samples));
            }
        }

        res.result = BitmapCompare::Compare(*res.rendered, *res.truth, tolerance, background);
        return res;
    }

    // Prints the pixels that differ from the ground truth by more than tolerance
    inline void ExpectMaxDifference(const String& svg, int tolerance, int samples = 16)
    {
        Fidelity fidelity = MeasureFidelity(svg, tolerance, samples);
        EXPECT_TRUE(fidelity.result.comparable);
        EXPECT_LE(fidelity.result.maxDifference, tolerance);
        if (fidelity.result.maxDifference <= tolerance)
            return;

        Vec2I size = fidelity.rendered->GetSize();
        int printed = 0;
        for (int y = 0; y < size.y && printed < 8; y++)
        {
            for (int x = 0; x < size.x && printed < 8; x++)
            {
                Color4 got = VectorRasterizer::GetPixel(*fidelity.rendered, x, y);
                Color4 expected = VectorRasterizer::GetPixel(*fidelity.truth, x, y);
                if (Math::Abs(got.r - expected.r) > tolerance)
                {
                    printf("  pixel %d,%d: %d, expected %d\n", x, y, got.r, expected.r);
                    printed++;
                }
            }
        }
    }
}
