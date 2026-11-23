#include "o2/stdafx.h"
#include "Bitmap.h"

#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2/Utils/Bitmap/StbImageFormat.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Reflection/Reflection.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace o2
{
    Bitmap::Bitmap():
        mFormat(PixelFormat::R8G8B8A8), mData(nullptr)
    {}

    Bitmap::Bitmap(PixelFormat format, const Vec2I& size) :
        mFormat(format), mSize(size), mData(nullptr)
    {
        Create(format, size);
    }

    Bitmap::Bitmap(const String& fileName, ImageType type /*= IT_AUTO*/):
        mFormat(PixelFormat::R8G8B8A8), mData(nullptr)
    {
        Load(fileName, type);
    }

    Bitmap::Bitmap(const Bitmap& other)
    {
        short bpp[] ={ 4, 3 };

        mFormat = other.mFormat;
        mSize = other.mSize;

        UInt dataSize = mSize.x*mSize.y*bpp[(int)mFormat];
        mData = mnew unsigned char[dataSize];
        memcpy(mData, other.mData, dataSize);
    }

    Bitmap::~Bitmap()
    {
        if (mData)
            delete[] mData;
    }

    Bitmap& Bitmap::operator=(const Bitmap& other)
    {
        if (mData)
            delete[] mData;

        short bpp[] ={ 4, 3 };

        mFormat = other.mFormat;
        mSize = other.mSize;

        UInt dataSize = mSize.x*mSize.y*bpp[(int)mFormat];
        mData = mnew unsigned char[dataSize];
        memcpy(mData, other.mData, dataSize);

        return *this;
    }

    Bitmap* Bitmap::Clone() const
    {
        return mnew Bitmap(*this);
    }

    void Bitmap::Create(PixelFormat format, const Vec2I& size)
    {
        if (mData)
            delete[] mData;

        short bpp[] ={ 4, 3 };

        mFormat = format;
        mSize = size;

        mData = mnew unsigned char[size.x*size.y*bpp[(int)format]];
    }

    bool Bitmap::Load(const String& fileName, ImageType type)
    {
        PROFILE_SAMPLE("Decode image");
        PROFILE_INFO(fileName);

        mFilename = fileName;

        if (type == ImageType::Png)
            return LoadPngImage(fileName, this, true);
        else
        {
            if (LoadPngImage(fileName, this, false))
                return true;

            if (LoadStbImage(fileName, this, false))
                return true;

            o2Debug.LogError("Can't load image '" + fileName + "': unknown format");
        }

        mFilename = "";

        return false;
    }

    bool Bitmap::LoadFromMemory(const UInt8* data, UInt size, ImageType type /*= ImageType::Auto*/)
    {
        mFilename = "";

        if (type == ImageType::Png)
            return LoadPngImageFromMemory(data, size, this, true);

        if (LoadPngImageFromMemory(data, size, this, false))
            return true;

        if (LoadStbImageFromMemory(data, size, this, false))
            return true;

        o2Debug.LogError("Can't load image from memory: unknown format");
        return false;
    }

    bool Bitmap::Save(const String& fileName, ImageType type) const
    {
        if (type == ImageType::Png || type == ImageType::Auto)
        {
            return SavePngImage(fileName, this);
        }

        o2Debug.LogError("Can't save image to '" + fileName + "': unknown format specified");

        return false;
    }

    void Bitmap::Clear(const Color4& color)
    {
        Fill(color);
    }

    unsigned char* Bitmap::GetData()
    {
        return mData;
    }

    Vec2I Bitmap::GetSize() const
    {
        return mSize;
    }

    PixelFormat Bitmap::GetFormat() const
    {
        return mFormat;
    }

    const unsigned char* Bitmap::GetData() const
    {
        return mData;
    }

    const String& Bitmap::GetFilename() const
    {
        return mFilename;
    }

    void Bitmap::CopyImage(const Bitmap& img, const Vec2I& position /*= Vec2I()*/, const RectI& imgSrc /*= RectI()*/)
    {
        if (mFormat != img.mFormat)
            return;

        RectI imgSrcRect = imgSrc;

        if (imgSrcRect.Width() == 0)
            imgSrcRect.Set(Vec2I(), img.GetSize());

        int bpp[] ={ 4, 3 };
        int curbpp = bpp[(int)mFormat];
        int pixelSize = curbpp;

        for (int x = 0; x < imgSrcRect.right - imgSrcRect.left; x++)
        {
            if (x + position.x >= mSize.x)
                break;

            if (x + position.x < 0)
                continue;

            for (int y = 0; y < imgSrcRect.top - imgSrcRect.bottom; y++)
            {
                if (y + position.y >= mSize.y)
                    break;

                if (y + position.y < 0)
                    continue;

                UInt srcIdx = (img.mSize.y - (y + imgSrcRect.bottom) - 1)*img.mSize.x + x + imgSrcRect.left;
                UInt dstIdx = (mSize.y - 1 - (y + position.y))*mSize.x + x + position.x;

                *(int*)&mData[dstIdx*pixelSize] = *(int*)&img.mData[srcIdx*pixelSize];
            }
        }
    }

    void Bitmap::BlendImage(const Bitmap& img, const Vec2I& position /*= Vec2I()*/, const RectI& imgSrc /*= RectI()*/)
    {
        if (mFormat != img.mFormat)
            return;

        RectI imgSrcRect = imgSrc;

        if (imgSrcRect.Width() == 0)
            imgSrcRect.Set(Vec2I(), img.GetSize());

        int bpp[] ={ 4, 3 };
        int curbpp = bpp[(int)mFormat];
        int pixelSize = curbpp;

        for (int x = 0; x < imgSrcRect.right - imgSrcRect.left; x++)
        {
            if (x + position.x >= mSize.x)
                break;

            if (x + position.x < 0)
                continue;

            for (int y = 0; y < imgSrcRect.top - imgSrcRect.bottom; y++)
            {
                if (y + position.y >= mSize.y)
                    break;

                if (y + position.y < 0)
                    continue;

                UInt srcIdx = (img.mSize.y - (y + imgSrcRect.bottom) - 1)*img.mSize.x + x + imgSrcRect.left;
                UInt dstIdx = (mSize.y - 1 - (y + position.y))*mSize.x + x + position.x;

                Color4 srcColor, src1Color;
                srcColor.SetABGR(*(Color32Bit*)(mData + dstIdx*pixelSize));
                src1Color.SetABGR(*(Color32Bit*)(img.mData + srcIdx*pixelSize));

                Color4 resColor = srcColor.BlendByAlpha(src1Color);
                Color32Bit uresColor = resColor.ABGR();

                memcpy(mData + dstIdx*pixelSize, &uresColor, pixelSize);
            }
        }
    }

    void Bitmap::Colorise(const Color4& color)
    {
        int bpp[] ={ 4, 3 };
        int curbpp = bpp[(int)mFormat];

        for (int x = 0; x < mSize.x*mSize.y; x++)
        {
            Color4 c;
            c.SetABGR(*(Color32Bit*)(mData + x*curbpp));
            c *= color;
            *(Color32Bit*)(mData + x*curbpp) = c.ABGR();
        }
    }

    void Bitmap::GradientByAlpha(const Color4& color1, const Color4& color4, float angle /*= 0*/, float size /*= 0*/,
                                 Vec2F origin /*= Vec2F()*/)
    {
        int bpp[] ={ 4, 3 };
        int curbpp = bpp[(int)mFormat];

        Vec2F dir = Vec2F::Rotated(Math::Deg2rad(angle + 90.0f));
        if (Math::Equals(size, 0))
            size = ((Vec2F)mSize).Dot(dir);

        float invSize = 1.0f / size;

        Vec2F pxorigin = origin*(Vec2F)mSize;

        for (int x = 0; x < mSize.x; x++)
        {
            for (int y = 0; y < mSize.y; y++)
            {
                Vec2F p = Vec2F((float)x, (float)y) - pxorigin;
                float proj = p.Dot(dir);
                float coef = Math::Clamp01(proj*invSize);
                Color32Bit offs = (y*mSize.x + x)*curbpp;

                Color4 c;
                c.SetABGR(*(Color32Bit*)(mData + offs));
                c *= Math::Lerp(color1, color4, coef);
                *(Color32Bit*)(mData + offs) = c.ABGR();
            }
        }
    }

    void Bitmap::Fill(const Color4& color)
    {
        unsigned long colrDw = color.ARGB();
        int bpp[] ={ 4, 3 };
        int curbpp = bpp[(int)mFormat];

        for (int x = 0; x < mSize.x*mSize.y; x++)
            memcpy(mData + x*curbpp, &colrDw, curbpp);
    }

    void Bitmap::FillRect(int rtLeft, int rtTop, int rtRight, int rtBottom, const Color4& color)
    {
        unsigned long colrDw = color.ARGB();
        int bpp[] = { 4, 3 };
        int curbpp = bpp[(int)mFormat];

        for (int x = Math::Max(rtLeft, 0); x < Math::Min(mSize.x, rtRight); x++)
        {
            for (int y = Math::Max(rtBottom, 0); y < Math::Min(mSize.y, rtTop); y++)
                memcpy(&mData[(y*mSize.x + x)*curbpp], &colrDw, curbpp);
        }
    }

    // The straightforward blur: every channel of every pixel gathered from the whole square around it
    static void BlurGeneric(UInt8* mData, const Vec2I& mSize, PixelFormat mFormat, float radius)
    {
        int mapSize = Math::CeilToInt(radius);
        int fullmapSize = mapSize * 2 + 1;

        float** weightMap = mnew float*[fullmapSize];
        for (int i = 0; i < fullmapSize; i++)
        {
            weightMap[i] = mnew float[fullmapSize];

            for (int j = 0; j < fullmapSize; j++)
            {
                float x = (float)(i - mapSize), y = (float)(j - mapSize);
                float dst = Math::Sqrt(x*x + y*y);
                weightMap[i][j] = Math::Clamp01(1.0f - dst / radius);
            }
        }

        int bpp[] = { 4, 3 };
        int curbpp = bpp[(int)mFormat];
        UInt8* srcData = mnew UInt8[mSize.x*mSize.y*curbpp];
        memcpy(srcData, mData, mSize.x*mSize.y*curbpp);

        for (int x = 0; x < mSize.x; x++)
        {
            for (int y = 0; y < mSize.y; y++)
            {
                Color4 c;
                Color4 csum(0, 0, 0, 0);
                float wSum = 0;

                for (int ox = 0; ox < fullmapSize; ox++)
                {
                    for (int oy = 0; oy < fullmapSize; oy++)
                    {
                        int cx = x + (ox - mapSize), cy = y + (oy - mapSize);

                        if (cx < 0 || cx >= mSize.x || cy < 0 || cy >= mSize.y)
                            continue;

                        c.SetARGB(*(Color32Bit*)&srcData[(cy*mSize.x + cx)*curbpp]);
                        float w = weightMap[ox][oy];
                        csum += c*w;
                        wSum += w;
                    }
                }

                csum /= wSum;
                Color32Bit ucsum = csum.ARGB();
                memcpy(&mData[(y*mSize.x + x)*curbpp], &ucsum, curbpp);
            }
        }

        for (int i = 0; i < fullmapSize; i++)
            delete[] weightMap[i];

        delete[] weightMap;
        delete[] srcData;
    }

    // The straightforward outline: the solid pixels counted in the whole square around every pixel
    static void OutlineGeneric(UInt8* mData, const Vec2I& mSize, PixelFormat mFormat, float radius, const Color4& color, int threshold)
    {
        int mapSize = Math::CeilToInt(radius);
        int fullmapSize = mapSize * 2 + 1;
        int alphaThreshold = threshold;

        int bpp[] = { 4, 3 };
        int curbpp = bpp[(int)mFormat];

        UInt8* srcData = mnew UInt8[mSize.x*mSize.y*curbpp];
        memcpy(srcData, mData, mSize.x*mSize.y*curbpp);

        for (int x = 0; x < mSize.x; x++)
        {
            for (int y = 0; y < mSize.y; y++)
            {
                float sqrDist = 0;
                int count = 0;
                Color4 pc;
                UInt offs = (y*mSize.x + x)*curbpp;
                pc.SetABGR(*(Color32Bit*)&srcData[offs]);

                for (int ox = 0; ox < fullmapSize; ox++)
                {
                    for (int oy = 0; oy < fullmapSize; oy++)
                    {
                        int fx = ox - mapSize, fy = oy - mapSize;
                        int cx = x + (ox - mapSize), cy = y + (oy - mapSize);

                        if (cx < 0 || cx >= mSize.x || cy < 0 || cy >= mSize.y)
                            continue;

                        Color4 c;
                        c.SetABGR(*(Color32Bit*)&srcData[(cy*mSize.x + cx)*curbpp]);
                        if (c.a > alphaThreshold)
                        {
                            int dst = fx*fx + fy*fy;
                            sqrDist += dst;
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
                    memcpy(&mData[offs], &unewColor, curbpp);
                }
            }
        }

        delete[] srcData;
    }

    // Weight of the blur at an offset: a cone from 1 in the middle to 0 at the radius (as BlurGeneric computes it)
    static float BlurWeight(int dx, int dy, float radius)
    {
        float x = (float)dx, y = (float)dy;
        float dst = Math::Sqrt(x*x + y*y);
        return Math::Clamp01(1.0f - dst / radius);
    }

    void Bitmap::Blur(float radius)
    {
        PROFILE_SAMPLE("Bitmap::Blur");

        int mapSize = Math::CeilToInt(radius);
        const int w = mSize.x, h = mSize.y, m = mapSize;
        if (mFormat != PixelFormat::R8G8B8A8 || w <= 0 || h <= 0)
        {
            BlurGeneric(mData, mSize, mFormat, radius);
            return;
        }

        // The fast path takes the bitmaps a font shadow blurs: one colour, the coverage in the alpha. Then the
        // colour sums depend only on which part of the square fits the bitmap, and the alpha is spread from the
        // covered pixels alone. Every sum is the one BlurGeneric makes, term for term: the same result to a bit
        UInt32* pixels = (UInt32*)mData;
        const UInt32 rgbMask = 0x00FFFFFF;
        const UInt32 rgb = pixels[0] & rgbMask;
        for (int i = 1; i < w*h; i++)
        {
            if ((pixels[i] & rgbMask) != rgb)
            {
                BlurGeneric(mData, mSize, mFormat, radius);
                return;
            }
        }

        // the square's taps with a weight, in the order BlurGeneric adds them (dx outer, dy inner)
        struct Tap { int dx, dy, weightIndex; };
        std::vector<Tap> taps;
        std::vector<float> weights;
        for (int dx = -m; dx <= m; dx++)
        {
            for (int dy = -m; dy <= m; dy++)
            {
                float weight = BlurWeight(dx, dy, radius);
                if (weight <= 0.0f)
                    continue;
                int index = (int)(std::find(weights.begin(), weights.end(), weight) - weights.begin());
                if (index == (int)weights.size())
                    weights.push_back(weight);
                taps.push_back({ dx, dy, index });
            }
        }

        // a weighted channel, truncated per tap as Color4*float does
        std::vector<int> table(weights.size()*256);
        for (int i = 0; i < (int)weights.size(); i++)
            for (int v = 0; v < 256; v++)
                table[i*256 + v] = (int)((float)v*weights[i]);

        // the colour: sums and the weight of the taps that fit, by the part of the square that fits
        const int colour[3] = { (int)(rgb & 0xFF), (int)((rgb >> 8) & 0xFF), (int)((rgb >> 16) & 0xFF) };
        struct Part { float weightSum = 0; int sums[3] = { 0, 0, 0 }; };
        std::unordered_map<UInt64, Part> parts;
        auto partOf = [&](int x, int y) -> const Part& {
            int dxMin = Math::Max(-m, -x), dxMax = Math::Min(m, w - 1 - x);
            int dyMin = Math::Max(-m, -y), dyMax = Math::Min(m, h - 1 - y);
            UInt64 key = ((UInt64)(dxMin + m) << 48) | ((UInt64)(dxMax + m) << 32) | ((UInt64)(dyMin + m) << 16) | (UInt64)(dyMax + m);
            auto found = parts.find(key);
            if (found != parts.end())
                return found->second;

            Part part;
            for (auto& tap : taps)
            {
                if (tap.dx < dxMin || tap.dx > dxMax || tap.dy < dyMin || tap.dy > dyMax)
                    continue;
                part.weightSum += weights[tap.weightIndex];
                for (int c = 0; c < 3; c++)
                    part.sums[c] += table[tap.weightIndex*256 + colour[c]];
            }
            return parts[key] = part;
        };

        // the alpha: every covered pixel adds its weighted coverage to the pixels whose square holds it
        std::vector<int> alphaSums(w*h, 0);
        for (int sy = 0; sy < h; sy++)
        {
            for (int sx = 0; sx < w; sx++)
            {
                int alpha = (int)(pixels[sy*w + sx] >> 24);
                if (alpha == 0)
                    continue;

                const int* row = table.data() + alpha;
                bool inside = sx - m >= 0 && sx + m < w && sy - m >= 0 && sy + m < h;
                for (auto& tap : taps)
                {
                    int tx = sx - tap.dx, ty = sy - tap.dy;
                    if (!inside && (tx < 0 || tx >= w || ty < 0 || ty >= h))
                        continue;
                    alphaSums[ty*w + tx] += row[tap.weightIndex*256];
                }
            }
        }

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const Part& part = partOf(x, y);
                float inverse = 1.0f/part.weightSum;
                UInt32 r = (UInt32)(int)((float)part.sums[0]*inverse);
                UInt32 g = (UInt32)(int)((float)part.sums[1]*inverse);
                UInt32 b = (UInt32)(int)((float)part.sums[2]*inverse);
                UInt32 a = (UInt32)(int)((float)alphaSums[y*w + x]*inverse);
                pixels[y*w + x] = r | (g << 8) | (b << 16) | (a << 24);
            }
        }
    }

    void Bitmap::Outline(float radius, const Color4& color, int threshold /*= 100*/)
    {
        PROFILE_SAMPLE("Bitmap::Outline");

        const int w = mSize.x, h = mSize.y, m = Math::CeilToInt(radius);
        if (mFormat != PixelFormat::R8G8B8A8 || w <= 0 || h <= 0)
        {
            OutlineGeneric(mData, mSize, mFormat, radius, color, threshold);
            return;
        }

        // Per pixel the outline needs how many solid pixels (alpha over the threshold) the square around it holds
        // and the sum of their squared distances: both come from summed-area tables of the solid pixels, of their
        // coordinates and of the squares of those - the same integers OutlineGeneric counts, in O(1) a pixel
        std::vector<UInt8> source(mData, mData + w*h*4);

        const int stride = w + 1;
        const size_t cells = (size_t)(w + 1)*(h + 1);
        std::vector<Int64> count(cells, 0), sumX(cells, 0), sumY(cells, 0), sumXX(cells, 0), sumYY(cells, 0);
        for (int y = 0; y < h; y++)
        {
            int row = (y + 1)*stride;
            Int64 lineCount = 0, lineX = 0, lineY = 0, lineXX = 0, lineYY = 0;
            for (int x = 0; x < w; x++)
            {
                if (source[(y*w + x)*4 + 3] > threshold)
                {
                    lineCount++; lineX += x; lineY += y; lineXX += (Int64)x*x; lineYY += (Int64)y*y;
                }
                int at = row + x + 1, above = at - stride;
                count[at] = count[above] + lineCount;
                sumX[at] = sumX[above] + lineX;
                sumY[at] = sumY[above] + lineY;
                sumXX[at] = sumXX[above] + lineXX;
                sumYY[at] = sumYY[above] + lineYY;
            }
        }
        auto area = [&](const std::vector<Int64>& table, int x0, int y0, int x1, int y1) {
            return table[(y1 + 1)*stride + x1 + 1] - table[y0*stride + x1 + 1] - table[(y1 + 1)*stride + x0] + table[y0*stride + x0];
        };

        for (int x = 0; x < w; x++)
        {
            for (int y = 0; y < h; y++)
            {
                int x0 = Math::Max(0, x - m), x1 = Math::Min(w - 1, x + m);
                int y0 = Math::Max(0, y - m), y1 = Math::Min(h - 1, y + m);
                Int64 n = area(count, x0, y0, x1, y1);
                if (n == 0)
                    continue;

                Int64 distances = area(sumXX, x0, y0, x1, y1) - 2*(Int64)x*area(sumX, x0, y0, x1, y1) + n*x*x +
                                  area(sumYY, x0, y0, x1, y1) - 2*(Int64)y*area(sumY, x0, y0, x1, y1) + n*y*y;

                // as OutlineGeneric: a float sum of whole numbers (exact below 2^24), then the mean
                float sqrDist = (float)distances;
                sqrDist /= (float)n;
                float distance = Math::Sqrt((float)sqrDist);
                if (distance < radius + 1.0f)
                {
                    UInt offs = (y*w + x)*4;
                    Color4 pc;
                    pc.SetABGR(*(Color32Bit*)&source[offs]);

                    Color4 stroke = color;
                    stroke.a = (int)((float)stroke.a*Math::Clamp01(1.0f - (distance - radius)));

                    Color4 newColor = pc.BlendByAlpha(stroke);
                    Color32Bit unewColor = newColor.ABGR();
                    memcpy(&mData[offs], &unewColor, 4);
                }
            }
        }
    }

    Ref<Bitmap> Bitmap::Resized(const Vec2I& newSize) const
    {
        int bpp[] = { 4, 3 };
        int curbpp = bpp[(int)mFormat];

        auto result = mmake<Bitmap>(mFormat, newSize);
        UInt8* dst = result->GetData();

        Vec2F scale((float)mSize.x/(float)newSize.x, (float)mSize.y/(float)newSize.y);
        for (int y = 0; y < newSize.y; y++)
        {
            float srcY = ((float)y + 0.5f)*scale.y - 0.5f;
            int y0 = Math::Clamp(Math::FloorToInt(srcY), 0, mSize.y - 1);
            int y1 = Math::Min(y0 + 1, mSize.y - 1);
            float fy = Math::Clamp01(srcY - (float)y0);

            for (int x = 0; x < newSize.x; x++)
            {
                float srcX = ((float)x + 0.5f)*scale.x - 0.5f;
                int x0 = Math::Clamp(Math::FloorToInt(srcX), 0, mSize.x - 1);
                int x1 = Math::Min(x0 + 1, mSize.x - 1);
                float fx = Math::Clamp01(srcX - (float)x0);

                for (int c = 0; c < curbpp; c++)
                {
                    float v00 = mData[(y0*mSize.x + x0)*curbpp + c];
                    float v10 = mData[(y0*mSize.x + x1)*curbpp + c];
                    float v01 = mData[(y1*mSize.x + x0)*curbpp + c];
                    float v11 = mData[(y1*mSize.x + x1)*curbpp + c];
                    float v = Math::Lerp(Math::Lerp(v00, v10, fx), Math::Lerp(v01, v11, fx), fy);
                    dst[(y*newSize.x + x)*curbpp + c] = (UInt8)Math::Clamp(v + 0.5f, 0.0f, 255.0f);
                }
            }
        }

        return result;
    }
}
// --- META ---

ENUM_META(o2::Bitmap::ImageType, o2__Bitmap__ImageType)
{
    ENUM_ENTRY(Auto);
    ENUM_ENTRY(Png);
}
END_ENUM_META;
// --- END META ---
