#include "o2Editor/stdafx.h"
#include "PipelineEditRegion.h"

#include <cmath>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Pipeline/PipelineValue.h"

namespace Editor::PipelineEditRegion
{
    const String promptSuffix =
        "This image is a close-up crop around the area to change. Make the change in the middle of the crop, and keep the "
        "crop's outer margin exactly as it is - the result is pasted back into the full image, so the edges must still match.";

    bool Read(const DataValue* value, PipelineImageOps::CropRect& region)
    {
        region = PipelineImageOps::CropRect();
        if (!value || !value->IsObject())
            return false;

        double v[4];
        const char* keys[4] = { "x", "y", "w", "h" };
        for (int i = 0; i < 4; i++)
        {
            auto member = value->FindMember(keys[i]);
            if (!member || !member->IsNumber())
                return false;

            v[i] = (double)*member;
            if (!std::isfinite(v[i]))
                return false;
        }

        float x = Math::Clamp((float)v[0], 0.0f, 1.0f), y = Math::Clamp((float)v[1], 0.0f, 1.0f);
        float w = Math::Clamp((float)v[2], 0.0f, 1.0f - x), h = Math::Clamp((float)v[3], 0.0f, 1.0f - y);
        if (w < minSize || h < minSize)
            return false;

        region.x = x; region.y = y; region.w = w; region.h = h;
        return true;
    }

    bool Of(const PipelineNode& node, PipelineImageOps::CropRect& region)
    {
        return node.nodeType == "imageEdit" && Read(node.GetConfigValue("editRegion"), region);
    }

    Boxes PixelBoxes(const Vec2I& imageSize, const PipelineImageOps::CropRect& region)
    {
        int width = Math::Max(1, imageSize.x), height = Math::Max(1, imageSize.y);
        Boxes b;
        b.region.left = Math::Clamp((int)std::lround(region.x*width), 0, width - 1);
        b.region.top = Math::Clamp((int)std::lround(region.y*height), 0, height - 1);
        b.region.right = Math::Max(b.region.left + 1, Math::Min(width, (int)std::lround((region.x + region.w)*width)));
        b.region.bottom = Math::Max(b.region.top + 1, Math::Min(height, (int)std::lround((region.y + region.h)*height)));

        int margin = Math::Max(16, (int)std::lround(0.25*Math::Max(b.region.Width(), b.region.Height())));
        b.box.left = Math::Max(0, b.region.left - margin);
        b.box.top = Math::Max(0, b.region.top - margin);
        b.box.right = Math::Min(width, b.region.right + margin);
        b.box.bottom = Math::Min(height, b.region.bottom + margin);
        return b;
    }

    static float AxisWeight(int p, int lo, int hi, int boxLo, int boxHi)
    {
        if (p < lo)
            return lo > boxLo ? Math::Max(0.0f, 1.0f - (float)(lo - p)/(lo - boxLo)) : 1.0f;
        if (p > hi - 1)
            return boxHi > hi ? Math::Max(0.0f, 1.0f - (float)(p - (hi - 1))/(boxHi - hi)) : 1.0f;
        return 1.0f;
    }

    float Weight(int x, int y, const Boxes& b)
    {
        return AxisWeight(x, b.region.left, b.region.right, b.box.left, b.box.right)*
            AxisWeight(y, b.region.top, b.region.bottom, b.box.top, b.box.bottom);
    }

    Ref<Bitmap> PasteRegionEdit(const Bitmap& input, const Bitmap& edited, const Boxes& b)
    {
        auto base = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(input)));
        auto out = mmake<Bitmap>(*base);
        Vec2I size = out->GetSize();
        int bw = b.box.Width(), bh = b.box.Height();
        if (bw <= 0 || bh <= 0)
            return out;

        auto patch = PipelineImageOps::Resize(edited, Vec2I(bw, bh));
        for (int y = Math::Max(0, b.box.top); y < Math::Min(size.y, b.box.bottom); y++)
        {
            for (int x = Math::Max(0, b.box.left); x < Math::Min(size.x, b.box.right); x++)
            {
                float w = Weight(x, y, b);
                if (w <= 0.0f)
                    continue;

                UInt8* o = PipelineImageOps::Pixel(*out, x, y);
                const UInt8* e = PipelineImageOps::Pixel(*patch, x - b.box.left, y - b.box.top);
                for (int c = 0; c < 4; c++)
                    o[c] = (UInt8)Math::Clamp((int)std::lround(o[c]*(1.0f - w) + e[c]*w), 0, 255);
            }
        }
        return out;
    }
}
