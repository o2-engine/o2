#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include <cmath>

#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelinePartFit.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineRegions.h"

using namespace o2;
using namespace Editor;
using UiLayout::UiImage;

// A cut-out part's density is measured against the picture it was cut from (AssetsLine backend/test/partFit.test.ts):
// the model's frame against the crop is only what the generation claims

namespace
{
    // Fills a rounded rectangle, antialiased (4 x 4 samples per pixel), source-over
    void RoundRect(UiImage& img, double x, double y, double w, double h, double r, UInt32 rgb)
    {
        r = std::min(r, std::min(w, h)/2);
        auto inside = [&](double px, double py) {
            if (px < x || py < y || px > x + w || py > y + h)
                return false;
            double cx = std::clamp(px, x + r, x + w - r), cy = std::clamp(py, y + r, y + h - r);
            return (px - cx)*(px - cx) + (py - cy)*(py - cy) <= r*r;
        };
        int x0 = std::max(0, (int)std::floor(x)), x1 = std::min(img.w - 1, (int)std::ceil(x + w));
        int y0 = std::max(0, (int)std::floor(y)), y1 = std::min(img.h - 1, (int)std::ceil(y + h));
        for (int py = y0; py <= y1; py++)
        {
            for (int px = x0; px <= x1; px++)
            {
                int hits = 0;
                for (int sy = 0; sy < 4; sy++)
                    for (int sx = 0; sx < 4; sx++)
                        hits += inside(px + (sx + 0.5)/4, py + (sy + 0.5)/4) ? 1 : 0;
                if (!hits)
                    continue;
                double a = hits/16.0;
                UInt8* p = img.At(px, py);
                double da = p[3]/255.0, oa = a + da*(1 - a);
                UInt8 c[3] = { (UInt8)(rgb >> 16), (UInt8)(rgb >> 8), (UInt8)rgb };
                for (int k = 0; k < 3; k++)
                    p[k] = (UInt8)std::lround(oa > 0 ? (c[k]*a + p[k]*da*(1 - a))/oa : 0);
                p[3] = (UInt8)std::lround(oa*255);
            }
        }
    }

    // A rounded button with a rim, a gloss and (unless bare) a label
    void Button(UiImage& img, double x, double y, double w, double h, bool label = true)
    {
        double r = h*0.32, rim = h*0.07;
        RoundRect(img, x, y, w, h, r, 0x6B4A1E);
        RoundRect(img, x + rim, y + rim, w - 2*rim, h - 2*rim, r - rim, 0x3FB52A);
        RoundRect(img, x + rim*2, y + rim*1.6, w - 4*rim, h*0.3, h*0.15, 0x7BDD55);
        if (label)
            RoundRect(img, x + w*0.2, y + h*0.4, w*0.6, h*0.22, 4, 0xFFFFFF);
    }

    // A parchment panel with a button at (60, 40), 230 x 96
    UiImage Screen()
    {
        UiImage img = UiImage::Blank(400, 200, 0xF2, 0xDF, 0xB4, 255);
        RoundRect(img, 12, 10, 376, 180, 18, 0xEBCF96);
        Button(img, 60, 40, 230, 96);
        return img;
    }

    // The bare button as a model draws it: px wide, transparent around
    UiImage BareButton(int px)
    {
        int h = (int)std::lround(px*96.0/230);
        UiImage img = UiImage::Blank(px, h, 0, 0, 0, 0);
        Button(img, 0, 0, px, h, false);
        return img;
    }

    UiImage Crop()
    {
        return Screen().Crop(40, 22, 270, 132);
    }
}

TEST(PipelinePartFit, APartDrawnAtTheSizeItsFrameClaimsStaysAsItIs)
{
    UiImage crop = Crop();
    EXPECT_FALSE(PipelinePartFit::FitPartOnCrop(crop, BareButton(1150), 5).has_value());
    // A few percent off is not worth a correction
    EXPECT_FALSE(PipelinePartFit::FitPartOnCrop(crop, BareButton(1080), 5).has_value());
}

TEST(PipelinePartFit, APartTheModelDrewMuchSmallerIsGivenItsRealScale)
{
    UiImage crop = Crop();
    // The frame is 5 x the crop, but the button was drawn only 760 px wide: 3.3 x the 230 px it has in the source
    auto fit = PipelinePartFit::FitPartOnCrop(crop, BareButton(760), 5);
    ASSERT_TRUE(fit.has_value());
    EXPECT_LT(std::fabs(fit->scale - 760.0/230), 0.2) << fit->scale;
    EXPECT_LT(std::fabs(fit->x - 20), 8) << fit->x;
    EXPECT_LT(std::fabs(fit->y - 18), 8) << fit->y;
    EXPECT_LT(std::fabs(fit->w - 230), 14) << fit->w;
    // ...and one drawn larger than its frame's ratio says
    auto big = PipelinePartFit::FitPartOnCrop(crop, BareButton(900), 2.8);
    ASSERT_TRUE(big.has_value());
    EXPECT_LT(std::fabs(big->scale - 900.0/230), 0.25) << big->scale;
}

TEST(PipelinePartFit, NestedShapesDoNotFoolItAndAPlateIsNotMeasured)
{
    auto panel = [](UiImage& img, double x, double y, double w, double h) {
        RoundRect(img, x, y, w, h, w*0.08, 0x8A5A2B);
        RoundRect(img, x + w*0.12, y + h*0.12, w*0.76, h*0.76, w*0.06, 0xE9CF9A);
    };
    UiImage src = UiImage::Blank(300, 300, 0x2A, 0x22, 0x33, 255);
    panel(src, 30, 30, 240, 240);
    UiImage crop = src.Crop(20, 20, 260, 260);
    UiImage part = UiImage::Blank(960, 960, 0, 0, 0, 0);
    panel(part, 0, 0, 960, 960);
    EXPECT_FALSE(PipelinePartFit::FitPartOnCrop(crop, part, 4).has_value());
    UiImage plate = UiImage::Blank(520, 520, 0x2A, 0x22, 0x33, 255);
    EXPECT_FALSE(PipelinePartFit::FitPartOnCrop(crop, plate, 2).has_value());
}

TEST(PipelinePartFit, ThePartsDpiFollowsTheMeasuredScaleAndCarriesItsSource)
{
    auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    DataDocument regions;
    regions.LoadFromData(R"json([{"id":"p1","name":"the button","x":0.1,"y":0.11,"w":0.675,"h":0.66}])json");
    node->config["regions"] = static_cast<const DataValue&>(regions);
    node->SetConfigNumber("sourceDpi", 480);
    PipelineRegions::SyncPorts(*node);

    String source = Screen().Png();
    String shrunk = BareButton(760).Png();
    // What the cache holds: the part with the scale its generation claims (frame 1350 px against a 270 px crop)
    String stored = PipelinePngMeta::WithDpi(shrunk, std::nullopt, 5.0);
    String out = PipelineRegions::TagExtractPart(*node, "p1", source, stored, shrunk, false);
    double scale = PipelinePngMeta::ReadScale(out).value_or(0);
    EXPECT_LT(std::fabs(scale - 760.0/230), 0.2) << scale;
    EXPECT_LT(std::fabs(PipelinePngMeta::ReadDpi(out).value_or(0) - 480*scale), 1);
    int w = 0, h = 0;
    ASSERT_TRUE(PipelinePngMeta::ReadSource(out, w, h));
    EXPECT_EQ(w, 400);
    EXPECT_EQ(h, 200);

    // Kept in place: the claimed scale stands
    String kept = BareButton(1150).Png();
    EXPECT_EQ(PipelinePngMeta::ReadScale(PipelineRegions::TagExtractPart(*node, "p1", source, PipelinePngMeta::WithDpi(kept, std::nullopt, 5.0),
                                                                         kept, false)).value_or(0), 5.0);
    EXPECT_FALSE(PipelineRegions::MeasuredPartScale("not a picture", PipelineRegions::RegionOfPort(*node, "p1"), shrunk, 5).has_value());
    // Without the source there is nothing to measure against, and no source size
    String bare = PipelineRegions::TagExtractPart(*node, "p1", "", stored, shrunk, false);
    EXPECT_EQ(PipelinePngMeta::ReadScale(bare).value_or(0), 5.0);
    EXPECT_FALSE(PipelinePngMeta::ReadSource(bare, w, h));
}
