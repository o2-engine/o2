#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <random>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Math.h"

using namespace o2;

// Bitmap::Blur and Bitmap::Outline (the font effects: shadows, strokes) against the straightforward algorithms they
// replaced: the fast paths must give the same bytes, and they must be fast enough for big styled glyphs
namespace
{
    // The blur as it was: every channel of every pixel gathered from the whole square, Color4 arithmetic
    void ReferenceBlur(Bitmap& bitmap, float radius)
    {
        UInt8* data = bitmap.GetData();
        Vec2I size = bitmap.GetSize();
        int mapSize = Math::CeilToInt(radius);
        int fullmapSize = mapSize*2 + 1;
        std::vector<std::vector<float>> weightMap(fullmapSize, std::vector<float>(fullmapSize));
        for (int i = 0; i < fullmapSize; i++)
        {
            for (int j = 0; j < fullmapSize; j++)
            {
                float x = (float)(i - mapSize), y = (float)(j - mapSize);
                float dst = Math::Sqrt(x*x + y*y);
                weightMap[i][j] = Math::Clamp01(1.0f - dst / radius);
            }
        }

        std::vector<UInt8> src(data, data + size.x*size.y*4);
        for (int x = 0; x < size.x; x++)
        {
            for (int y = 0; y < size.y; y++)
            {
                Color4 c;
                Color4 csum(0, 0, 0, 0);
                float wSum = 0;
                for (int ox = 0; ox < fullmapSize; ox++)
                {
                    for (int oy = 0; oy < fullmapSize; oy++)
                    {
                        int cx = x + (ox - mapSize), cy = y + (oy - mapSize);
                        if (cx < 0 || cx >= size.x || cy < 0 || cy >= size.y)
                            continue;
                        c.SetARGB(*(Color32Bit*)&src[(cy*size.x + cx)*4]);
                        float w = weightMap[ox][oy];
                        csum += c*w;
                        wSum += w;
                    }
                }
                csum /= wSum;
                Color32Bit ucsum = csum.ARGB();
                memcpy(&data[(y*size.x + x)*4], &ucsum, 4);
            }
        }
    }

    // The outline as it was: the solid pixels counted in the whole square around every pixel
    void ReferenceOutline(Bitmap& bitmap, float radius, const Color4& color, int threshold)
    {
        UInt8* data = bitmap.GetData();
        Vec2I size = bitmap.GetSize();
        int mapSize = Math::CeilToInt(radius);
        int fullmapSize = mapSize*2 + 1;
        std::vector<UInt8> src(data, data + size.x*size.y*4);
        for (int x = 0; x < size.x; x++)
        {
            for (int y = 0; y < size.y; y++)
            {
                float sqrDist = 0;
                int count = 0;
                Color4 pc;
                UInt offs = (y*size.x + x)*4;
                pc.SetABGR(*(Color32Bit*)&src[offs]);
                for (int ox = 0; ox < fullmapSize; ox++)
                {
                    for (int oy = 0; oy < fullmapSize; oy++)
                    {
                        int fx = ox - mapSize, fy = oy - mapSize;
                        int cx = x + fx, cy = y + fy;
                        if (cx < 0 || cx >= size.x || cy < 0 || cy >= size.y)
                            continue;
                        Color4 c;
                        c.SetABGR(*(Color32Bit*)&src[(cy*size.x + cx)*4]);
                        if (c.a > threshold)
                        {
                            sqrDist += fx*fx + fy*fy;
                            count++;
                        }
                    }
                }
                if (count == 0)
                    continue;
                sqrDist /= (float)count;
                float distance = Math::Sqrt((float)sqrDist);
                if (distance < radius + 1.0f)
                {
                    Color4 stroke = color;
                    stroke.a = (int)((float)stroke.a*Math::Clamp01(1.0f - (distance - radius)));
                    Color4 newColor = pc.BlendByAlpha(stroke);
                    Color32Bit unewColor = newColor.ABGR();
                    memcpy(&data[offs], &unewColor, 4);
                }
            }
        }
    }

    // A glyph-like coverage: a ring and a bar with soft edges, inside a transparent border
    Bitmap GlyphLike(const Vec2I& size, const Color4& colour, int seed)
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, size);
        std::mt19937 random(seed);
        float cx = size.x*0.5f, cy = size.y*0.5f, outer = Math::Min(size.x, size.y)*0.32f, inner = outer*0.55f;
        UInt8* data = bitmap.GetData();
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                float d = Math::Sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));
                float ring = Math::Clamp01(outer - d)*Math::Clamp01(d - inner);
                float bar = (Math::Abs(x - cx*0.6f) < size.x*0.06f && y > size.y*0.2f && y < size.y*0.8f) ? 1.0f : 0.0f;
                float coverage = Math::Max(ring, bar);
                if (coverage > 0 && random() % 7 == 0)
                    coverage *= 0.6f; // a little noise inside, as antialiasing and effects leave
                Color4 c = colour;
                c.a = (int)(coverage*colour.a);
                Color32Bit packed = c.ABGR();
                memcpy(&data[(y*size.x + x)*4], &packed, 4);
            }
        }
        return bitmap;
    }

    Bitmap Noise(const Vec2I& size, int seed)
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, size);
        std::mt19937 random(seed);
        for (int i = 0; i < size.x*size.y*4; i++)
            bitmap.GetData()[i] = (UInt8)(random() & 0xFF);
        return bitmap;
    }

    bool SameBytes(const Bitmap& a, const Bitmap& b)
    {
        return a.GetSize() == b.GetSize() && memcmp(a.GetData(), b.GetData(), a.GetSize().x*a.GetSize().y*4) == 0;
    }
}

TEST(BitmapEffects, ShadowBlurGivesTheSameBytesAsTheGatheringBlur)
{
    const float radii[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.5f, 5.0f, 10.0f };
    const Vec2I sizes[] = { Vec2I(64, 80), Vec2I(7, 9), Vec2I(3, 40), Vec2I(130, 61) };
    int seed = 1;
    for (float radius : radii)
    {
        for (const Vec2I& size : sizes)
        {
            Bitmap fast = GlyphLike(size, Color4(66, 43, 24, 128), seed++);
            Bitmap reference(fast);
            fast.Blur(radius);
            ReferenceBlur(reference, radius);
            EXPECT_TRUE(SameBytes(fast, reference)) << "radius " << radius << " size " << size.x << "x" << size.y;
        }
    }
}

TEST(BitmapEffects, BlurOfManyColoursStaysTheGatheringBlur)
{
    Bitmap fast = Noise(Vec2I(37, 29), 7);
    Bitmap reference(fast);
    fast.Blur(3.0f);
    ReferenceBlur(reference, 3.0f);
    EXPECT_TRUE(SameBytes(fast, reference));
}

TEST(BitmapEffects, OutlineGivesTheSameBytesAsTheCountingOutline)
{
    const float radii[] = { 1.0f, 2.0f, 3.0f, 4.5f, 7.0f };
    const Vec2I sizes[] = { Vec2I(64, 80), Vec2I(7, 9), Vec2I(3, 40), Vec2I(130, 61) };
    const int thresholds[] = { 0, 100, 200 };
    int seed = 100;
    for (float radius : radii)
    {
        for (const Vec2I& size : sizes)
        {
            for (int threshold : thresholds)
            {
                Bitmap fast = GlyphLike(size, Color4(255, 240, 189, 255), seed++);
                Bitmap reference(fast);
                fast.Outline(radius, Color4(89, 53, 15, 255), threshold);
                ReferenceOutline(reference, radius, Color4(89, 53, 15, 255), threshold);
                EXPECT_TRUE(SameBytes(fast, reference)) << "radius " << radius << " size " << size.x << "x" << size.y
                    << " threshold " << threshold;
            }
        }
        Bitmap fast = Noise(Vec2I(33, 21), seed++);
        Bitmap reference(fast);
        fast.Outline(radius, Color4(10, 20, 30, 200), 128);
        ReferenceOutline(reference, radius, Color4(10, 20, 30, 200), 128);
        EXPECT_TRUE(SameBytes(fast, reference)) << "noise, radius " << radius;
    }
}

// The glyph of a big styled label (the victory window's reward: about 300x360 with its border) took ~40 ms in the
// shadow blur and ~12 ms in the stroke; a frame has 16
TEST(BitmapEffects, BigGlyphEffectsFitAFrame)
{
    using Clock = std::chrono::steady_clock;
    Bitmap glyph = GlyphLike(Vec2I(300, 360), Color4(66, 43, 24, 128), 3);
    auto start = Clock::now();
    glyph.Blur(10.0f);
    double blurMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

    Bitmap stroked = GlyphLike(Vec2I(300, 360), Color4(255, 240, 189, 255), 4);
    start = Clock::now();
    stroked.Outline(7.0f, Color4(89, 53, 15, 255), 100);
    double outlineMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

    printf("BitmapEffects: blur r10 %.1f ms, outline r7 %.1f ms (300x360)\n", blurMs, outlineMs);
#ifdef NDEBUG
    // Shared CI runners are slower and noisy: twice the budget there
    double budget = std::getenv("CI") ? 2.0 : 1.0;
    EXPECT_LT(blurMs, 15.0*budget);
    EXPECT_LT(outlineMs, 5.0*budget);
#endif
}
