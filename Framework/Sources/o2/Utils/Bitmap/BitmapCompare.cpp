#include "o2/stdafx.h"
#include "BitmapCompare.h"

#include "o2/Utils/Math/Math.h"

namespace o2
{
    namespace
    {
        int CompositeChannel(int value, int alpha, int background, bool premultiplied)
        {
            int source = premultiplied ? value*255 : value*alpha;
            return Math::Min((source + background*(255 - alpha) + 127)/255, 255);
        }
    }

    BitmapCompareResult BitmapCompare::Compare(const Bitmap& a, const Bitmap& b, int tolerance /*= 8*/,
                                               const Color4& background /*= Color4::White()*/,
                                               Bitmap* difference /*= nullptr*/)
    {
        BitmapCompareResult res;

        Vec2I size = a.GetSize();
        if (size != b.GetSize() || a.GetFormat() != PixelFormat::R8G8B8A8 || b.GetFormat() != PixelFormat::R8G8B8A8 ||
            !a.GetData() || !b.GetData() || size.x <= 0 || size.y <= 0)
        {
            return res;
        }

        res.comparable = true;

        if (difference)
            difference->Create(PixelFormat::R8G8B8A8, size);

        const UInt8* dataA = a.GetData();
        const UInt8* dataB = b.GetData();
        int backgroundChannels[3] = { background.r, background.g, background.b };

        size_t count = (size_t)size.x*size.y;
        size_t matched = 0;
        double differenceSum = 0;

        for (size_t i = 0; i < count; i++)
        {
            const UInt8* pixelA = dataA + i*4;
            const UInt8* pixelB = dataB + i*4;

            int pixelDifference = 0;
            int luminance = 0;
            for (int channel = 0; channel < 3; channel++)
            {
                int valueA = CompositeChannel(pixelA[channel], pixelA[3], backgroundChannels[channel], false);
                int valueB = CompositeChannel(pixelB[channel], pixelB[3], backgroundChannels[channel], false);
                pixelDifference = Math::Max(pixelDifference, Math::Abs(valueA - valueB));
                luminance += valueA;
            }

            if (pixelDifference <= tolerance)
                matched++;

            differenceSum += pixelDifference;
            res.maxDifference = Math::Max(res.maxDifference, pixelDifference);

            if (difference)
            {
                UInt8* pixel = difference->GetData() + i*4;
                if (pixelDifference <= tolerance)
                {
                    int gray = 128 + luminance/6;
                    pixel[0] = pixel[1] = pixel[2] = (UInt8)gray;
                }
                else
                {
                    pixel[0] = 255;
                    pixel[1] = pixel[2] = (UInt8)(128 - pixelDifference/2);
                }

                pixel[3] = 255;
            }
        }

        res.similarity = (float)((double)matched/(double)count);
        res.meanDifference = (float)(differenceSum/(double)count);
        return res;
    }

    float BitmapCompare::GetSimilarity(const Bitmap& a, const Bitmap& b, int tolerance /*= 8*/,
                                       const Color4& background /*= Color4::White()*/)
    {
        return Compare(a, b, tolerance, background).similarity;
    }

    void BitmapCompare::CompositeOver(Bitmap& bitmap, const Color4& background, bool premultiplied /*= false*/)
    {
        if (bitmap.GetFormat() != PixelFormat::R8G8B8A8 || !bitmap.GetData())
            return;

        Vec2I size = bitmap.GetSize();
        UInt8* data = bitmap.GetData();
        int backgroundChannels[3] = { background.r, background.g, background.b };

        for (size_t i = 0; i < (size_t)size.x*size.y; i++)
        {
            UInt8* pixel = data + i*4;
            for (int channel = 0; channel < 3; channel++)
            {
                pixel[channel] = (UInt8)CompositeChannel(pixel[channel], pixel[3], backgroundChannels[channel],
                                                         premultiplied);
            }

            pixel[3] = 255;
        }
    }
}
