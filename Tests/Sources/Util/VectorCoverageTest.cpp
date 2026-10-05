#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <cmath>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    String Shape(const char* body)
    {
        return Svg(40, 40, body);
    }

    Ref<Bitmap> RenderOverWhite(const String& svg)
    {
        return VectorRasterizer::Rasterize(Parse(svg), 1.0f, Color4(255, 255, 255, 255));
    }

    int Red(const Ref<Bitmap>& bitmap, int x, int y)
    {
        return VectorRasterizer::GetPixel(*bitmap, x, y).r;
    }

    struct AlphaStop
    {
        double offset, opacity;
    };

    // Maximum difference of the alpha in columns and rows [begin, end) from the radial ramp at the centers of pixels
    double RampDifference(const Ref<Bitmap>& bitmap, const Vec2I& begin, const Vec2I& end, double centerX, double centerY,
                          double radius, const Vector<AlphaStop>& stops)
    {
        double res = 0;
        for (int y = begin.y; y < end.y; y++)
        {
            for (int x = begin.x; x < end.x; x++)
            {
                double offset = sqrt((x + 0.5 - centerX)*(x + 0.5 - centerX) + (y + 0.5 - centerY)*(y + 0.5 - centerY))/radius;
                double opacity = offset <= stops[0].offset ? stops[0].opacity : stops.back().opacity;
                for (int i = 1; i < stops.Count(); i++)
                {
                    if (offset > stops[i - 1].offset && offset <= stops[i].offset)
                    {
                        opacity = stops[i - 1].opacity + (stops[i].opacity - stops[i - 1].opacity)*
                            (offset - stops[i - 1].offset)/(stops[i].offset - stops[i - 1].offset);
                    }
                }

                res = Math::Max(res, fabs(Alpha(bitmap, x, y) - opacity*255.0));
            }
        }

        return res;
    }

    String RadialGradient(double centerX, double centerY, double radius, const Vector<AlphaStop>& stops)
    {
        String res = String("<defs><radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"") + (String)(float)centerX +
            "\" cy=\"" + (String)(float)centerY + "\" r=\"" + (String)(float)radius + "\">";

        for (const AlphaStop& stop : stops)
        {
            res += String("<stop offset=\"") + (String)(float)stop.offset + "\" stop-color=\"#000\" stop-opacity=\"" +
                (String)(float)stop.opacity + "\"/>";
        }

        return res + "</radialGradient></defs>";
    }
}

TEST(VectorCoverage, RadialGradientAwayFromTheShapeCenter)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Svg(100, 100,
        "<defs><radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"30\" cy=\"65\" r=\"10\">"
        "<stop offset=\"0\" stop-color=\"#f00\"/><stop offset=\"1\" stop-color=\"#00f\"/></radialGradient></defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#g)\"/>"));

    Color4 center = VectorRasterizer::GetPixel(*bitmap, 30, 65);
    EXPECT_GT(center.r, 225);
    EXPECT_LT(center.b, 30);

    Color4 outside = VectorRasterizer::GetPixel(*bitmap, 60, 20);
    EXPECT_EQ(outside.r, 0);
    EXPECT_EQ(outside.b, 255);
}

TEST(VectorCoverage, RadialGradientMatchesTheRamp)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Svg(100, 60,
        "<defs><radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"30\" r=\"25\">"
        "<stop offset=\"0\" stop-color=\"#f00\"/><stop offset=\"0.5\" stop-color=\"#0f0\"/>"
        "<stop offset=\"1\" stop-color=\"#00f\"/></radialGradient></defs>"
        "<rect width=\"100\" height=\"60\" fill=\"url(#g)\"/>"));

    int maxDifference = 0;
    for (int y = 0; y < 60; y++)
    {
        for (int x = 0; x < 100; x++)
        {
            double offset = Math::Min(sqrt((x + 0.5 - 50.0)*(x + 0.5 - 50.0) + (y + 0.5 - 30.0)*(y + 0.5 - 30.0))/25.0, 1.0);
            double red = offset < 0.5 ? 255.0*(1.0 - offset*2.0) : 0.0;
            double green = offset < 0.5 ? 255.0*offset*2.0 : 255.0*(2.0 - offset*2.0);
            double blue = offset < 0.5 ? 0.0 : 255.0*(offset*2.0 - 1.0);

            Color4 pixel = VectorRasterizer::GetPixel(*bitmap, x, y);
            maxDifference = Math::Max(maxDifference, (int)fabs(pixel.r - red));
            maxDifference = Math::Max(maxDifference, (int)fabs(pixel.g - green));
            maxDifference = Math::Max(maxDifference, (int)fabs(pixel.b - blue));
        }
    }

    EXPECT_LE(maxDifference, 10);
}

TEST(VectorCoverage, NeedleTipIsNotOpaque)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Shape("<path d=\"M5,5.5 L35,5 L35,7 Z\"/>"));
    EXPECT_GT(Red(bitmap, 5, 5), 235);
    EXPECT_NEAR(Red(bitmap, 6, 5), 231, 16);

    ExpectMaxDifference(Shape("<path d=\"M5,5.5 L35,5 L35,7 Z\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M9.3,7.2 L22,20 L31,31 Z\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M18.5,5 L36.5,32 L32,26.5 Z\"/>"), 20);
}

TEST(VectorCoverage, RectCornerAtFractionalPosition)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Shape("<path d=\"M8.25,8.6 h24 v24 h-24 Z\"/>"));
    EXPECT_NEAR(Red(bitmap, 8, 8), 255 - 0.3*255, 5);
    EXPECT_NEAR(Red(bitmap, 32, 32), 255 - 0.25*0.6*255, 5);

    ExpectMaxDifference(Shape("<path d=\"M8.25,8.6 h24 v24 h-24 Z\"/>"), 8);
    ExpectMaxDifference(Shape("<rect x=\"7.3125\" y=\"9.6875\" width=\"21.375\" height=\"13.3125\"/>"), 8);
    ExpectMaxDifference(Shape("<path d=\"M8.25,8.6 h24 v24 h-12 v-10.3 h-12 Z\"/>"), 8);
}

TEST(VectorCoverage, RotatedShapesStayWithinDiagonalRampError)
{
    ExpectMaxDifference(Shape("<rect x=\"10\" y=\"12\" width=\"18\" height=\"11\" transform=\"rotate(17 20 20)\"/>"), 16);
    ExpectMaxDifference(Shape("<rect x=\"10\" y=\"12\" width=\"18\" height=\"11\" transform=\"rotate(45 20 20)\"/>"), 16);
    ExpectMaxDifference(Shape("<path d=\"M8,8 L32,10 L28,32 L10,28 Z\"/>"), 16);
    ExpectMaxDifference(Shape("<path d=\"M20.3,4.1 L35.2,33.7 L5.4,30.2 Z\"/>"), 16);
    ExpectMaxDifference(Shape("<circle cx=\"20\" cy=\"20\" r=\"12.3\"/>"), 16);
}

TEST(VectorCoverage, SubPixelRectangles)
{
    Ref<Bitmap> square = RenderSvg(Shape("<rect x=\"5.5\" y=\"10.5\" width=\"1\" height=\"1\"/>"));
    EXPECT_NEAR(Alpha(square, 5, 10), 64, 2);
    EXPECT_NEAR(Alpha(square, 6, 10), 64, 2);
    EXPECT_NEAR(Alpha(square, 5, 11), 64, 2);
    EXPECT_NEAR(Alpha(square, 6, 11), 64, 2);

    Ref<Bitmap> sliver = RenderSvg(Shape("<rect x=\"9.75\" y=\"3.25\" width=\"3.75\" height=\"0.5\"/>"));
    EXPECT_NEAR(Alpha(sliver, 9, 3), 32, 2);
    EXPECT_NEAR(Alpha(sliver, 10, 3), 128, 2);
    EXPECT_NEAR(Alpha(sliver, 13, 3), 64, 2);

    ExpectMaxDifference(Shape("<rect x=\"3.3\" y=\"4.6\" width=\"0.7\" height=\"0.4\"/>"), 6);
    ExpectMaxDifference(Shape("<circle cx=\"10.3\" cy=\"10.6\" r=\"0.3\"/>"), 10);
    ExpectMaxDifference(Shape("<circle cx=\"10.3\" cy=\"10.6\" r=\"1\"/>"), 12);
}

TEST(VectorCoverage, ThinStrokesAndCrossings)
{
    ExpectMaxDifference(Shape("<path d=\"M5,5 L35,28\" fill=\"none\" stroke=\"#000\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M12,31.75 L29.5,5.5 L7.75,26 L20,13.5\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M12,31.75 L29.5,5.5 L7.75,26 L20,13.5\" fill=\"none\" stroke=\"#000\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M6.5,6.5 h20 v20 h-20 Z\" fill=\"none\" stroke=\"#000\" stroke-width=\"1.375\"/>"), 8);
    ExpectMaxDifference(Shape("<path d=\"M6,8 L18,17 L30,6\" fill=\"none\" stroke=\"#000\" stroke-width=\"2\""
                              " stroke-linejoin=\"round\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M8,8 L32,10 L28,32 L10,28 Z\" fill=\"none\" stroke=\"#000\" stroke-width=\"3\"/>"), 20);
}

TEST(VectorCoverage, WideStrokeOfCurves)
{
    ExpectMaxDifference(Shape("<ellipse cx=\"21\" cy=\"15.5\" rx=\"6.855\" ry=\"1.005\" fill=\"none\" stroke=\"#000\""
                              " stroke-width=\"12\"/>"), 20);

    // Inner side of the stroke is folded here: the curve is bent sharper than the stroke half width
    ExpectMaxDifference(Shape("<path d=\"M30,14.5 C40,6.5 18,36 21,36\" fill=\"none\" stroke=\"#000\" stroke-width=\"8\"/>"), 48);
    ExpectMaxDifference(Shape("<circle cx=\"20\" cy=\"20\" r=\"5\" fill=\"none\" stroke=\"#000\" stroke-width=\"14\"/>"), 20);
    ExpectMaxDifference(Shape("<path d=\"M5,30 C5,5 35,5 35,30\" fill=\"none\" stroke=\"#000\" stroke-width=\"6\"/>"), 20);
}

TEST(VectorCoverage, CurveStrokeEndsFollowTheTangent)
{
    Ref<Bitmap> bitmap = RenderSvg(Shape("<path d=\"M10,30 C10,10 30,10 30,30\" fill=\"none\" stroke=\"#000\""
                                         " stroke-width=\"8\"/>"));
    for (int x = 7; x < 13; x++)
    {
        EXPECT_EQ(Alpha(bitmap, x, 29), 255) << x;
        EXPECT_EQ(Alpha(bitmap, x, 30), 0) << x;
        EXPECT_EQ(Alpha(bitmap, x + 20, 29), 255) << x;
        EXPECT_EQ(Alpha(bitmap, x + 20, 30), 0) << x;
    }
}

TEST(VectorCoverage, ClosedLineHasJoinsAtBothEnds)
{
    const char* shapes[] = { "M10,20 L30,20 Z", "M10,20 L30,20 L10,20 Z" };
    for (const char* data : shapes)
    {
        String path = String("<path d=\"") + data + "\" fill=\"none\" stroke=\"#000\" stroke-width=\"8\" stroke-linecap=\"square\"";

        Ref<Bitmap> round = RenderSvg(Shape((path + " stroke-linejoin=\"round\"/>").Data()));
        EXPECT_EQ(Alpha(round, 32, 20), 255);
        EXPECT_EQ(Alpha(round, 7, 19), 255);
        EXPECT_EQ(Alpha(round, 34, 20), 0);
        EXPECT_LT(Alpha(round, 33, 16), 40);

        Ref<Bitmap> miter = RenderSvg(Shape((path + "/>").Data()));
        EXPECT_EQ(Alpha(miter, 29, 20), 255);
        EXPECT_EQ(Alpha(miter, 30, 20), 0);
        EXPECT_EQ(Alpha(miter, 9, 20), 0);
    }
}

TEST(VectorCoverage, ZeroLengthSubPathDrawsCap)
{
    Ref<Bitmap> closed = RenderSvg(Shape("<path d=\"M20,20 Z\" stroke=\"#000\" stroke-width=\"6\" stroke-linecap=\"round\"/>"));
    EXPECT_EQ(Alpha(closed, 19, 19), 255);
    EXPECT_EQ(Alpha(closed, 24, 20), 0);
    EXPECT_NEAR(AlphaSum(closed), 3.14159265*9.0, 0.6);

    Ref<Bitmap> butt = RenderSvg(Shape("<path d=\"M20,20 Z\" stroke=\"#000\" stroke-width=\"6\"/>"));
    EXPECT_EQ(AlphaSum(butt), 0.0);
}

TEST(VectorCoverage, NonUniformPixelScale)
{
    VectorTessellationParams params;
    params.pixelScale = Vec2F(2.0f, 1.0f);

    VectorMesh mesh;
    VectorTessellator::Tessellate(Parse(Svg(10, 10, "<rect x=\"2.25\" y=\"2.25\" width=\"5\" height=\"5\"/>")), mesh, params);
    EXPECT_EQ(mesh.pixelScale, Vec2F(2.0f, 1.0f));

    for (Vec2F& position : mesh.positions)
        position.x *= 2.0f;

    mesh.size = Vec2F(20, 10);
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);

    EXPECT_NEAR(Alpha(bitmap, 4, 5), 128, 2);
    EXPECT_EQ(Alpha(bitmap, 3, 5), 0);
    EXPECT_EQ(Alpha(bitmap, 5, 5), 255);
    EXPECT_NEAR(Alpha(bitmap, 14, 5), 128, 2);
    EXPECT_NEAR(Alpha(bitmap, 8, 2), 191, 2);
    EXPECT_NEAR(Alpha(bitmap, 8, 7), 64, 2);
}

TEST(VectorCoverage, PatchesCoverTheWholePixelSquares)
{
    ExpectMaxDifference(Shape("<rect x=\"10.67\" y=\"17.976\" width=\"10.669\" height=\"11.595\" rx=\"0.181\"/>"), 16);
    ExpectMaxDifference(Shape("<rect x=\"13.086\" y=\"12.32\" width=\"8.386\" height=\"1.477\" fill=\"none\" stroke=\"#000\""
                              " stroke-width=\"1.333\" stroke-linejoin=\"bevel\"/>"), 16);
}

TEST(VectorCoverage, LoopsAreClosedOverSnappedCrossings)
{
    ExpectMaxDifference(Shape("<ellipse cx=\"26.599\" cy=\"27.271\" rx=\"1.67\" ry=\"4.872\" fill=\"none\" stroke=\"#000\""
                              " stroke-width=\"1.333\"/>"), 24);
}

TEST(VectorCoverage, AdjacentBandsShareSidePoints)
{
    ExpectMaxDifference(Shape("<path d=\"M13.955,20.638 C34.565,22.945 29.2,13.041 8.947,4.206 C35.402,7.81 16.16,24.951"
                              " 27.507,23.78 C18.066,30.078 18.156,30.73 5.729,27.104 Z\" fill=\"none\" stroke=\"#000\""
                              " stroke-linejoin=\"round\"/>"), 40);
}

TEST(VectorCoverage, MiterJoinOfSnappedLines)
{
    ExpectMaxDifference(Shape("<path d=\"M13.932,34.057 L27.803,17.318 L12.075,4.271\" fill=\"none\" stroke=\"#000\""
                              " stroke-width=\"6.609\" stroke-linecap=\"square\"/>"), 20);
}

TEST(VectorCoverage, NeedleBetweenBandsOfDifferentWidth)
{
    ExpectMaxDifference(Shape("<path d=\"M6.312,6.411 L19.796,18.523 L13.84,12.604\"/>"), 40);
}

TEST(VectorCoverage, FillUnderStrokeOfTheSameColorIsOpaque)
{
    String arrow = Svg(20, 20, "<path d=\"M9.223 7.668 L10.777 7.668 L10.777 5.332 L12.332 5.332 L10 3 L7.668 5.332"
                               " L9.223 5.332 Z\" stroke=\"#000\" stroke-linejoin=\"round\"/>");
    Ref<Bitmap> bitmap = RenderSvg(arrow);

    for (int y = 5; y <= 7; y++)
    {
        EXPECT_EQ(Alpha(bitmap, 9, y), 255) << y;
        EXPECT_EQ(Alpha(bitmap, 10, y), 255) << y;
    }

    EXPECT_NEAR(Alpha(bitmap, 9, 2), 57, 3);
    for (int y = 0; y < 20; y++)
    {
        for (int x = 0; x < 10; x++)
            EXPECT_NEAR(Alpha(bitmap, x, y), Alpha(bitmap, 19 - x, y), 1) << x << "," << y;
    }

    ExpectMaxDifference(arrow, 6, 64);
}

TEST(VectorCoverage, FillUnderStrokeOfOtherColorIsOpaque)
{
    for (const char* width : { "1", "0.78", "0.4", "2.5" })
    {
        String svg = Svg(20, 20, String("<path d=\"M4.3 4.6 L15.2 6.1 L12.7 15.4 L5.1 12.2 Z\" fill=\"#f00\" stroke=\"#00f\""
                                        " stroke-width=\"") + width + "\"/>");
        Ref<Bitmap> bitmap = RenderSvg(svg);
        for (int y = 7; y <= 11; y++)
        {
            for (int x = 6; x <= 12; x++)
                EXPECT_EQ(Alpha(bitmap, x, y), 255) << width << ": " << x << "," << y;
        }

        ExpectMaxDifference(svg, 10, 64);
    }

    // The closing edge of an open path has no stroke over it
    ExpectMaxDifference(Shape("<path d=\"M24 8 L24 12 L28 12\" fill=\"#f00\" stroke=\"#00f\" stroke-width=\"0.75\"/>"), 10, 64);
    ExpectMaxDifference(Shape("<path d=\"M10.4 8.2 L10.4 20.6 L28.3 20.6\" fill=\"#f00\" stroke=\"#00f\" stroke-width=\"3\"/>"),
                        10, 64);
    ExpectMaxDifference(Shape("<path d=\"M12 18 C13.582 3.031 22.84 20.065 29 24\" fill=\"#f00\" stroke=\"#00f\"/>"), 10, 64);

    // The stroke goes over the fill of the self crossing path, far from the edges of the fill
    ExpectMaxDifference(Shape("<path d=\"M17.953 28.11 C20.858 15.13 34.442 15.216 28.41 8.405 C33.921 4.698 4.617 12.666"
                              " 18.335 18.727 C34.206 27.857 3.147 31.428 32.232 9.951\" fill=\"#f00\" stroke=\"#00f\"/>"),
                        16, 64);
}

TEST(VectorCoverage, SlantedStrokeEdgesAreAsExactAsFillEdges)
{
    const char* path = "<path d=\"M5 13.15 L7.25 15.21 L13.73 8.68 L11.63 6.57 Z\" stroke-width=\"0.78\" stroke-linejoin=\"round\"";

    ExpectMaxDifference(Svg(20, 20, String(path) + " stroke=\"#000\"/>"), 6, 64);
    ExpectMaxDifference(Svg(20, 20, String(path) + " fill=\"none\" stroke=\"#000\"/>"), 6, 64);
    ExpectMaxDifference(Svg(20, 20, "<path d=\"M5 13.15 L7.25 15.21 L13.73 8.68 L11.63 6.57 Z\"/>"), 6, 64);
}

TEST(VectorCoverage, RadialGradientFromTheRectCorner)
{
    Vector<AlphaStop> stops = { { 0, 0.147 }, { 0.146, 0.134 }, { 0.257, 0.117 }, { 0.493, 0.06 }, { 0.701, 0.02 },
                                { 0.771, 0.011 }, { 1, 0 } };
    Ref<Bitmap> bitmap = RenderSvg(Svg(40, 40, RadialGradient(20, 17, 18, stops) +
                                       "<rect x=\"2\" y=\"0\" width=\"18\" height=\"17\" fill=\"url(#g)\"/>"));

    EXPECT_LE(RampDifference(bitmap, Vec2I(2, 0), Vec2I(20, 17), 20, 17, 18, stops), 2.5);
}

TEST(VectorCoverage, RadialGradientWithCloseStopsIsSymmetric)
{
    Vector<AlphaStop> stops = { { 0.56, 0.85 }, { 0.64, 0.59 }, { 0.72, 0.31 }, { 0.8, 0.16 }, { 0.88, 0.04 }, { 0.96, 0 } };
    Ref<Bitmap> bitmap = RenderSvg(Svg(25, 25, RadialGradient(12.5, 12.5, 12.5, stops) +
                                       "<rect width=\"25\" height=\"25\" fill=\"url(#g)\"/>"));

    EXPECT_LE(RampDifference(bitmap, Vec2I(0, 0), Vec2I(25, 25), 12.5, 12.5, 12.5, stops), 2.5);
    EXPECT_LE(MirrorDifference(bitmap, 12.5f, 12.5f), 1);

    for (int i = 0; i < 25; i++)
        EXPECT_NEAR(Alpha(bitmap, i, 12), Alpha(bitmap, 12, i), 1) << i;
}

TEST(VectorCoverage, RadialGradientOfLowAlphaIsSmooth)
{
    Vector<AlphaStop> stops = { { 0, 0.102 }, { 0.1, 0.1 }, { 0.55, 0.131 }, { 1, 0.2 } };
    Ref<Bitmap> bitmap = RenderSvg(Svg(100, 100, RadialGradient(50.5, 50.5, 57.46, stops) +
                                       "<rect x=\"1\" y=\"1\" width=\"98\" height=\"98\" fill=\"url(#g)\"/>"));

    EXPECT_LE(RampDifference(bitmap, Vec2I(1, 1), Vec2I(99, 99), 50.5, 50.5, 57.46, stops), 2.5);
}

TEST(VectorCoverage, RadialGradientInCircleKeepsTheMeshSmall)
{
    Vector<AlphaStop> stops = { { 0.56, 0.086 }, { 0.72, 0.045 }, { 0.84, 0.009 }, { 0.98, 0 } };
    String svg = Svg(25, 25, RadialGradient(12.5, 12.5, 12.5, stops) + "<circle cx=\"12.5\" cy=\"12.5\" r=\"12.5\" fill=\"url(#g)\"/>");

    VectorMesh mesh;
    VectorTessellator::Tessellate(Parse(svg), mesh);
    EXPECT_LT(mesh.GetTrianglesCount(), 2400u);

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    EXPECT_LE(RampDifference(bitmap, Vec2I(4, 4), Vec2I(21, 21), 12.5, 12.5, 12.5, stops), 2.5);
    EXPECT_LE(MirrorDifference(bitmap, 12.5f, 12.5f), 1);
}

TEST(VectorCoverage, EvenOddStarHoleKeepsSymmetry)
{
    String svg = Svg(12, 12, "<path fill-rule=\"evenodd\" d=\"M2.01 6 C4.46 6 5 5.46 5 3.01 L6 3.01 C6 5.46 6.54 6 8.99 6 L8.99 7"
                             " C6.54 7 6 7.54 6 9.99 L5 9.99 C5 7.54 4.46 7 2.01 7 Z M4.28 6.5 C4.992 6.5 5.5 5.992 5.5 5.28"
                             " C5.5 5.992 6.008 6.5 6.72 6.5 C6.008 6.5 5.5 7.008 5.5 7.72 C5.5 7.008 4.992 6.5 4.28 6.5 Z\"/>");
    Ref<Bitmap> bitmap = RenderSvg(svg);

    EXPECT_LE(MirrorDifference(bitmap, 5.5f, 6.5f), 1);
    EXPECT_NEAR(Alpha(bitmap, 4, 5), 174, 4);
    EXPECT_NEAR(Alpha(bitmap, 6, 7), 174, 4);
    EXPECT_NEAR(Alpha(bitmap, 5, 6), 47, 4);
    EXPECT_NEAR(Alpha(bitmap, 5, 5), 230, 4);

    ExpectMaxDifference(svg, 6, 64);
}

TEST(VectorCoverage, EvenOddHoleTouchingTheCornerPixels)
{
    String capsule = Svg(10, 25, "<path fill-rule=\"evenodd\" d=\"M5 2 A2 2 0 0 1 7 4 V20 A2 2 0 0 1 3 20 V4 A2 2 0 0 1 5 2 Z"
                                 " M4 3 V21 H6 V3 Z\"/>");
    Ref<Bitmap> bitmap = RenderSvg(capsule);

    EXPECT_LE(MirrorDifference(bitmap, 5.0f, 12.0f), 1);
    EXPECT_NEAR(Alpha(bitmap, 3, 2), 81, 3);
    EXPECT_NEAR(Alpha(bitmap, 6, 21), 81, 3);
    ExpectMaxDifference(capsule, 6, 64);
}

TEST(VectorCoverage, EvenOddFrameWithRoundedHole)
{
    String frame = Svg(30, 8, "<path fill-rule=\"evenodd\" d=\"M2 2 H28 V6 H2 Z M3.85 3 A0.85 0.85 0 0 0 3 3.85 V4.15"
                              " A0.85 0.85 0 0 0 3.85 5 H26.15 A0.85 0.85 0 0 0 27 4.15 V3.85 A0.85 0.85 0 0 0 26.15 3 Z\"/>");
    Ref<Bitmap> bitmap = RenderSvg(frame);

    EXPECT_LE(MirrorDifference(bitmap, 15.0f, 4.0f), 1);
    for (int x = 4; x < 26; x++)
    {
        EXPECT_EQ(Alpha(bitmap, x, 3), 0) << x;
        EXPECT_EQ(Alpha(bitmap, x, 4), 0) << x;
        EXPECT_EQ(Alpha(bitmap, x, 2), 255) << x;
        EXPECT_EQ(Alpha(bitmap, x, 5), 255) << x;
    }

    EXPECT_NEAR(Alpha(bitmap, 3, 3), 40, 3);
}

TEST(VectorCoverage, OverlappingStrokedSubPathsAreUnited)
{
    String svg = Svg(10, 10, "<path d=\"M2.5 5.5 C4.5 5.5 5.5 4.5 5.5 2.5 M5.5 2.5 C5.5 4.5 6.5 5.5 8.5 5.5"
                             " M8.5 5.5 C6.5 5.5 5.5 6.5 5.5 8.5 M5.5 8.5 C5.5 6.5 4.5 5.5 2.5 5.5\" fill=\"none\""
                             " stroke=\"#fff\" stroke-linecap=\"round\"/>");
    Ref<Bitmap> bitmap = RenderSvg(svg);

    EXPECT_LE(MirrorDifference(bitmap, 5.5f, 5.5f), 1);
    EXPECT_NEAR(Alpha(bitmap, 5, 5), 20, 5);
    EXPECT_NEAR(Alpha(bitmap, 5, 2), 227, 5);
    EXPECT_NEAR(Alpha(bitmap, 2, 5), 227, 5);
    EXPECT_NEAR(Alpha(bitmap, 5, 3), 255, 1);
    EXPECT_LE(Alpha(bitmap, 5, 1), 2);

    ExpectMaxDifference(svg, 6, 64);
}

TEST(VectorCoverage, StrokeOfTinyCircleIsSymmetric)
{
    String svg = Svg(6, 6, "<circle cx=\"2.5\" cy=\"2.5\" r=\"0.8\" fill=\"none\" stroke=\"#fff\"/>");
    Ref<Bitmap> bitmap = RenderSvg(svg);

    EXPECT_LE(MirrorDifference(bitmap, 2.5f, 2.5f), 1);
    EXPECT_NEAR(Alpha(bitmap, 1, 2), 195, 4);
    EXPECT_NEAR(Alpha(bitmap, 2, 1), 195, 4);
    EXPECT_NEAR(Alpha(bitmap, 1, 1), 79, 4);
    EXPECT_NEAR(Alpha(bitmap, 2, 2), 183, 4);
}

TEST(VectorCoverage, SymmetricIconsRenderSymmetric)
{
    for (const char* body : { "<circle cx=\"10\" cy=\"10\" r=\"6.3\"/>",
                              "<circle cx=\"10\" cy=\"10\" r=\"6.4\" fill=\"none\" stroke=\"#000\" stroke-width=\"0.48\"/>",
                              "<rect x=\"3.5\" y=\"4.5\" width=\"13\" height=\"11\" rx=\"3\" fill=\"none\" stroke=\"#000\"/>",
                              "<path d=\"M10 3 L16 10 L10 17 L4 10 Z\" stroke=\"#000\" stroke-linejoin=\"round\"/>",
                              "<path d=\"M10 2.4 L17.6 10 L10 17.6 L2.4 10 Z M10 7 L7 10 L10 13 L13 10 Z\" fill-rule=\"evenodd\"/>" })
    {
        EXPECT_LE(MirrorDifference(RenderSvg(Svg(20, 20, body)), 10.0f, 10.0f), 1) << body;
        ExpectMaxDifference(Svg(20, 20, body), 6, 64);
    }
}

TEST(VectorCoverage, CurvesOfUiScaleAreExactAtPixelCenters)
{
    ExpectMaxDifference(Svg(20, 20, "<path d=\"M9.103 4 C6.538 4.511 4.511 6.536 4 9.103 L4 11.897 C4.511 14.464 6.538 16.489"
                                    " 9.103 17 L11.897 17 C14.464 16.489 16.489 14.464 17 11.897 L17 9.103 C16.489 6.536"
                                    " 14.464 4.511 11.897 4 Z\"/>"), 6, 64);
    ExpectMaxDifference(Svg(40, 40, "<rect x=\"6.5\" y=\"6.5\" width=\"26\" height=\"24\" rx=\"10\" fill=\"none\""
                                    " stroke=\"#000\"/>"), 6, 64);
    ExpectMaxDifference(Svg(40, 40, "<circle cx=\"20.3\" cy=\"19.6\" r=\"14.2\"/>"), 6, 64);
}
