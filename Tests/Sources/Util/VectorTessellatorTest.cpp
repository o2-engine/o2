#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <limits>

#include <cmath>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    VectorMesh Tessellate(const String& svg, float pixelScale = 1.0f, bool antialiasing = true, bool pixelSnapped = true)
    {
        VectorTessellationParams params;
        params.pixelScale = Vec2F(pixelScale, pixelScale);
        params.antialiasing = antialiasing;
        params.pixelSnapped = pixelSnapped;

        VectorMesh mesh;
        VectorTessellator::Tessellate(Parse(svg), mesh, params);
        return mesh;
    }

    void ExpectPixel(const Ref<Bitmap>& bitmap, int x, int y, const Color4& expected, int tolerance)
    {
        Color4 pixel = VectorRasterizer::GetPixel(*bitmap, x, y);
        EXPECT_NEAR(pixel.r, expected.r, tolerance) << "r at " << x << "," << y;
        EXPECT_NEAR(pixel.g, expected.g, tolerance) << "g at " << x << "," << y;
        EXPECT_NEAR(pixel.b, expected.b, tolerance) << "b at " << x << "," << y;
        EXPECT_NEAR(pixel.a, expected.a, tolerance) << "a at " << x << "," << y;
    }

    Ref<Bitmap> RenderOverWhite(const String& svg)
    {
        return VectorRasterizer::Rasterize(Parse(svg), 1.0f, Color4(255, 255, 255, 255));
    }

    const char* kCornerPath = "<path d=\"M4 4 H14 V14\" fill=\"none\" stroke=\"#000\" stroke-width=\"4\"";
}

TEST(VectorTessellator, IntegerRectIsExactlyOpaque)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(16, 16, "<rect x=\"3\" y=\"3\" width=\"10\" height=\"10\"/>"));

    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 100);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_EQ(Alpha(bitmap, 3, 3), 255);
    EXPECT_EQ(Alpha(bitmap, 12, 12), 255);
    EXPECT_EQ(Alpha(bitmap, 2, 3), 0);
    EXPECT_EQ(Alpha(bitmap, 13, 12), 0);
}

TEST(VectorTessellator, HalfPixelRectHasHalfAlphaEdges)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(16, 16, "<rect x=\"3.5\" y=\"3.5\" width=\"9\" height=\"9\"/>"));

    for (int i = 4; i < 12; i++)
    {
        EXPECT_NEAR(Alpha(bitmap, 3, i), 128, 1);
        EXPECT_NEAR(Alpha(bitmap, 12, i), 128, 1);
        EXPECT_NEAR(Alpha(bitmap, i, 3), 128, 1);
        EXPECT_NEAR(Alpha(bitmap, i, 12), 128, 1);
        EXPECT_EQ(Alpha(bitmap, i, 7), 255);
    }

    EXPECT_NEAR(Alpha(bitmap, 3, 3), 64, 1);
    EXPECT_EQ(Alpha(bitmap, 2, 7), 0);
    EXPECT_NEAR(AlphaSum(bitmap), 81.0, 1.0);
}

TEST(VectorTessellator, QuarterPixelRectCoverage)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(16, 16, "<rect x=\"3.25\" y=\"3\" width=\"9\" height=\"10\"/>"));
    EXPECT_NEAR(Alpha(bitmap, 3, 7), 191, 1);
    EXPECT_NEAR(Alpha(bitmap, 12, 7), 64, 1);
}

TEST(VectorTessellator, CircleAreaWithinOnePercent)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(50, 50, "<circle cx=\"25\" cy=\"25\" r=\"20\"/>"));
    double expected = 3.14159265*400.0;
    EXPECT_NEAR(AlphaSum(bitmap), expected, expected*0.01);
    EXPECT_EQ(Alpha(bitmap, 25, 25), 255);
    EXPECT_EQ(Alpha(bitmap, 2, 2), 0);

    Ref<Bitmap> small = RenderSvg(Svg(12, 12, "<circle cx=\"6.3\" cy=\"5.6\" r=\"4\"/>"));
    EXPECT_NEAR(AlphaSum(small), 3.14159265*16.0, 3.14159265*16.0*0.02);
}

TEST(VectorTessellator, RingUnderNonZeroAndEvenOdd)
{
    const char* sameDirection = "d=\"M2 2 H18 V18 H2 Z M6 6 H14 V14 H6 Z\"";
    const char* oppositeDirection = "d=\"M2 2 H18 V18 H2 Z M6 6 V14 H14 V6 Z\"";

    Ref<Bitmap> nonZero = RenderSvg(Svg(20, 20, String("<path ") + sameDirection + "/>"));
    EXPECT_EQ(Alpha(nonZero, 10, 10), 255);
    EXPECT_EQ(CountAlpha(nonZero, 255, 255), 256);

    Ref<Bitmap> evenOdd = RenderSvg(Svg(20, 20, String("<path fill-rule=\"evenodd\" ") + sameDirection + "/>"));
    EXPECT_EQ(Alpha(evenOdd, 10, 10), 0);
    EXPECT_EQ(Alpha(evenOdd, 4, 10), 255);
    EXPECT_EQ(CountAlpha(evenOdd, 255, 255), 256 - 64);
    EXPECT_EQ(CountAlpha(evenOdd, 1, 254), 0);

    Ref<Bitmap> hole = RenderSvg(Svg(20, 20, String("<path ") + oppositeDirection + "/>"));
    EXPECT_EQ(Alpha(hole, 10, 10), 0);
    EXPECT_EQ(CountAlpha(hole, 255, 255), 256 - 64);
}

TEST(VectorTessellator, CircleWithHoleArea)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(40, 40, "<path fill-rule=\"evenodd\" d=\"M36 20 A16 16 0 1 1 4 20 A16 16 0 1 1 36 20 Z "
                                            "M28 20 A8 8 0 1 1 12 20 A8 8 0 1 1 28 20 Z\"/>"));
    double expected = 3.14159265*(256.0 - 64.0);
    EXPECT_NEAR(AlphaSum(bitmap), expected, expected*0.01);
    EXPECT_EQ(Alpha(bitmap, 20, 20), 0);
    EXPECT_EQ(Alpha(bitmap, 8, 20), 255);
}

TEST(VectorTessellator, ConcavePolygon)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 20, "<polygon points=\"2,2 18,2 18,18 12,18 12,8 8,8 8,18 2,18\"/>"));

    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 256 - 40);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_EQ(Alpha(bitmap, 10, 12), 0);
    EXPECT_EQ(Alpha(bitmap, 10, 5), 255);
    EXPECT_EQ(Alpha(bitmap, 4, 16), 255);
}

TEST(VectorTessellator, ThinRectScalesAlphaByWidth)
{
    Ref<Bitmap> centered = RenderSvg(Svg(20, 10, "<rect x=\"2\" y=\"5.25\" width=\"16\" height=\"0.5\"/>"));
    for (int x = 4; x < 16; x++)
    {
        EXPECT_NEAR(Alpha(centered, x, 5), 128, 3);
        EXPECT_EQ(Alpha(centered, x, 4), 0);
        EXPECT_EQ(Alpha(centered, x, 6), 0);
    }

    Ref<Bitmap> between = RenderSvg(Svg(20, 10, "<rect x=\"2\" y=\"4.75\" width=\"16\" height=\"0.5\"/>"));
    for (int x = 4; x < 16; x++)
    {
        EXPECT_NEAR(Alpha(between, x, 4), 64, 3);
        EXPECT_NEAR(Alpha(between, x, 5), 64, 3);
    }

    Ref<Bitmap> quarter = RenderSvg(Svg(20, 10, "<rect x=\"2\" y=\"5.375\" width=\"16\" height=\"0.25\"/>"));
    EXPECT_NEAR(Alpha(quarter, 10, 5), 64, 3);
}

TEST(VectorTessellator, OnePixelRectOnPixelRow)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 10, "<rect x=\"2\" y=\"5\" width=\"16\" height=\"1\"/>"));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 16);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
}

TEST(VectorTessellator, OnePixelStroke)
{
    Ref<Bitmap> centered = RenderSvg(Svg(12, 12, "<line x1=\"5.5\" y1=\"1\" x2=\"5.5\" y2=\"11\" stroke=\"#000\"/>"));
    EXPECT_EQ(CountAlpha(centered, 255, 255), 10);
    EXPECT_EQ(CountAlpha(centered, 1, 254), 0);
    EXPECT_EQ(Alpha(centered, 5, 6), 255);

    Ref<Bitmap> between = RenderSvg(Svg(12, 12, "<line x1=\"5\" y1=\"1\" x2=\"5\" y2=\"11\" stroke=\"#000\"/>"));
    for (int y = 2; y < 10; y++)
    {
        EXPECT_NEAR(Alpha(between, 4, y), 128, 1);
        EXPECT_NEAR(Alpha(between, 5, y), 128, 1);
        EXPECT_EQ(Alpha(between, 3, y), 0);
        EXPECT_EQ(Alpha(between, 6, y), 0);
    }
}

TEST(VectorTessellator, ThinStrokeScalesAlphaByWidth)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(12, 12, "<line x1=\"1\" y1=\"5.5\" x2=\"11\" y2=\"5.5\" stroke=\"#000\" stroke-width=\"0.5\"/>"));
    for (int x = 2; x < 10; x++)
    {
        EXPECT_NEAR(Alpha(bitmap, x, 5), 128, 2);
        EXPECT_EQ(Alpha(bitmap, x, 4), 0);
        EXPECT_EQ(Alpha(bitmap, x, 6), 0);
    }
}

TEST(VectorTessellator, StrokeWidthExtents)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 20, "<line x1=\"4\" y1=\"10\" x2=\"16\" y2=\"10\" stroke=\"#000\" stroke-width=\"4\"/>"));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 12*4);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_EQ(Alpha(bitmap, 4, 8), 255);
    EXPECT_EQ(Alpha(bitmap, 15, 11), 255);
    EXPECT_EQ(Alpha(bitmap, 3, 10), 0);
    EXPECT_EQ(Alpha(bitmap, 10, 7), 0);

    Ref<Bitmap> wide = RenderSvg(Svg(20, 20, "<line x1=\"4\" y1=\"10\" x2=\"16\" y2=\"10\" stroke=\"#000\" stroke-width=\"3\"/>"));
    EXPECT_NEAR(AlphaSum(wide), 36.0, 0.5);
    EXPECT_NEAR(Alpha(wide, 10, 8), 128, 1);
    EXPECT_EQ(Alpha(wide, 10, 9), 255);
}

TEST(VectorTessellator, StrokeCapsExtents)
{
    const char* line = "<line x1=\"6\" y1=\"10\" x2=\"14\" y2=\"10\" stroke=\"#000\" stroke-width=\"4\" stroke-linecap=\"";

    Ref<Bitmap> butt = RenderSvg(Svg(20, 20, String(line) + "butt\"/>"));
    EXPECT_EQ(Alpha(butt, 5, 10), 0);
    EXPECT_EQ(Alpha(butt, 14, 10), 0);
    EXPECT_NEAR(AlphaSum(butt), 32.0, 0.01);

    Ref<Bitmap> square = RenderSvg(Svg(20, 20, String(line) + "square\"/>"));
    EXPECT_EQ(Alpha(square, 4, 8), 255);
    EXPECT_EQ(Alpha(square, 15, 11), 255);
    EXPECT_EQ(Alpha(square, 3, 10), 0);
    EXPECT_NEAR(AlphaSum(square), 48.0, 0.01);

    Ref<Bitmap> round = RenderSvg(Svg(20, 20, String(line) + "round\"/>"));
    EXPECT_EQ(Alpha(round, 5, 10), 255);
    EXPECT_EQ(Alpha(round, 14, 9), 255);
    EXPECT_GT(Alpha(round, 4, 10), 200);
    EXPECT_LT(Alpha(round, 4, 8), 128);
    EXPECT_EQ(Alpha(round, 3, 10), 0);
    EXPECT_NEAR(AlphaSum(round), 32.0 + 3.14159265*4.0, 0.4);
}

TEST(VectorTessellator, StrokeJoinsExtents)
{
    Ref<Bitmap> miter = RenderSvg(Svg(20, 20, String(kCornerPath) + " stroke-linejoin=\"miter\"/>"));
    EXPECT_EQ(Alpha(miter, 15, 2), 255);
    EXPECT_EQ(Alpha(miter, 16, 2), 0);
    EXPECT_NEAR(AlphaSum(miter), 76.0 + 4.0, 0.01);

    Ref<Bitmap> bevel = RenderSvg(Svg(20, 20, String(kCornerPath) + " stroke-linejoin=\"bevel\"/>"));
    EXPECT_EQ(Alpha(bevel, 15, 2), 0);
    EXPECT_EQ(Alpha(bevel, 14, 3), 255);
    EXPECT_NEAR(AlphaSum(bevel), 76.0 + 2.0, 0.1);

    Ref<Bitmap> round = RenderSvg(Svg(20, 20, String(kCornerPath) + " stroke-linejoin=\"round\"/>"));
    EXPECT_GT(Alpha(round, 15, 2), 40);
    EXPECT_LT(Alpha(round, 15, 2), 200);
    EXPECT_NEAR(AlphaSum(round), 76.0 + 3.14159265, 0.1);

    Ref<Bitmap> limited = RenderSvg(Svg(20, 20, String(kCornerPath) + " stroke-linejoin=\"miter\" stroke-miterlimit=\"1.2\"/>"));
    EXPECT_EQ(Alpha(limited, 15, 2), 0);
    EXPECT_NEAR(AlphaSum(limited), AlphaSum(bevel), 0.01);
}

TEST(VectorTessellator, InnerSideOfStrokeJoinIsSolid)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 20, String(kCornerPath) + "/>"));
    EXPECT_EQ(Alpha(bitmap, 12, 5), 255);
    EXPECT_EQ(Alpha(bitmap, 13, 4), 255);
    EXPECT_EQ(Alpha(bitmap, 11, 6), 0);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
}

TEST(VectorTessellator, ClosedStrokeIsRing)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 20, "<rect x=\"5\" y=\"5\" width=\"10\" height=\"10\" fill=\"none\" stroke=\"#000\" stroke-width=\"2\"/>"));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 144 - 64);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_EQ(Alpha(bitmap, 10, 10), 0);
    EXPECT_EQ(Alpha(bitmap, 4, 4), 255);
    EXPECT_EQ(Alpha(bitmap, 15, 15), 255);
}

TEST(VectorTessellator, DotWithRoundAndSquareCaps)
{
    Ref<Bitmap> round = RenderSvg(Svg(20, 20, "<path d=\"M10 10 L10 10\" stroke=\"#000\" stroke-width=\"8\" stroke-linecap=\"round\"/>"));
    EXPECT_NEAR(AlphaSum(round), 3.14159265*16.0, 1.0);

    Ref<Bitmap> square = RenderSvg(Svg(20, 20, "<path d=\"M10 10 L10 10\" stroke=\"#000\" stroke-width=\"8\" stroke-linecap=\"square\"/>"));
    EXPECT_EQ(CountAlpha(square, 255, 255), 64);

    Ref<Bitmap> butt = RenderSvg(Svg(20, 20, "<path d=\"M10 10 L10 10\" stroke=\"#000\" stroke-width=\"8\"/>"));
    EXPECT_EQ(CountAlpha(butt, 1, 255), 0);
}

TEST(VectorTessellator, LinearGradientIsExactAtStops)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(101, 4, "<linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"0.5\" x2=\"100.5\">"
                                            "<stop offset=\"0\" stop-color=\"#ff0000\"/><stop offset=\"0.5\" stop-color=\"#00ff00\"/>"
                                            "<stop offset=\"1\" stop-color=\"#0000ff\"/></linearGradient>"
                                            "<rect width=\"101\" height=\"4\" fill=\"url(#g)\"/>"));

    ExpectPixel(bitmap, 0, 1, Color4(255, 0, 0, 255), 0);
    ExpectPixel(bitmap, 50, 2, Color4(0, 255, 0, 255), 0);
    ExpectPixel(bitmap, 100, 1, Color4(0, 0, 255, 255), 0);
    ExpectPixel(bitmap, 25, 1, Color4(128, 128, 0, 255), 1);
    ExpectPixel(bitmap, 75, 2, Color4(0, 128, 128, 255), 1);
    ExpectPixel(bitmap, 10, 1, Color4(204, 51, 0, 255), 1);
}

TEST(VectorTessellator, LinearGradientPadsOutsideOfRamp)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(100, 4, "<linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"40\" x2=\"60\">"
                                            "<stop offset=\"0\" stop-color=\"#ff0000\"/><stop offset=\"1\" stop-color=\"#0000ff\"/></linearGradient>"
                                            "<rect width=\"100\" height=\"4\" fill=\"url(#g)\"/>"));

    for (int x = 0; x < 40; x += 3)
        ExpectPixel(bitmap, x, 1, Color4(255, 0, 0, 255), 0);

    for (int x = 60; x < 100; x += 3)
        ExpectPixel(bitmap, x, 2, Color4(0, 0, 255, 255), 0);

    ExpectPixel(bitmap, 49, 1, Color4(134, 0, 121, 255), 1);
}

TEST(VectorTessellator, DiagonalLinearGradientInBoundingBox)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(40, 40, "<linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\">"
                                            "<stop offset=\"0\" stop-color=\"#000000\"/><stop offset=\"1\" stop-color=\"#ffffff\"/></linearGradient>"
                                            "<rect width=\"40\" height=\"40\" fill=\"url(#g)\"/>"));

    for (int i = 0; i < 40; i += 5)
    {
        int expected = Math::RoundToInt((i + 0.5f)/40.0f*255.0f);
        ExpectPixel(bitmap, i, i, Color4(expected, expected, expected, 255), 1);
        ExpectPixel(bitmap, i, 39 - i, Color4(128, 128, 128, 255), 1);
    }
}

TEST(VectorTessellator, GradientWithEqualOffsetsMakesHardStep)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(20, 4, "<linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" x2=\"20\">"
                                           "<stop offset=\"0\" stop-color=\"#ff0000\"/><stop offset=\"0.5\" stop-color=\"#ff0000\"/>"
                                           "<stop offset=\"0.5\" stop-color=\"#0000ff\"/><stop offset=\"1\" stop-color=\"#0000ff\"/></linearGradient>"
                                           "<rect width=\"20\" height=\"4\" fill=\"url(#g)\"/>"));

    ExpectPixel(bitmap, 9, 1, Color4(255, 0, 0, 255), 0);
    ExpectPixel(bitmap, 10, 1, Color4(0, 0, 255, 255), 0);
}

TEST(VectorTessellator, GradientStopsAreSimplified)
{
    Vector<VectorGradientStop> stops;
    for (int i = 0; i <= 256; i++)
    {
        int value = Math::Min(i, 255);
        stops.Add(VectorGradientStop((float)i/256.0f, Color4(value, value, value, 255)));
    }

    EXPECT_EQ(VectorTessellator::SimplifyStops(stops).size(), 2u);

    stops[128].color = Color4(255, 0, 0, 255);
    Vector<VectorGradientStop> simplified = VectorTessellator::SimplifyStops(stops);
    EXPECT_EQ(simplified.size(), 5u);
    EXPECT_EQ(simplified[2].color, Color4(255, 0, 0, 255));

    VectorImage image;
    image.size = Vec2F(64, 8);
    image.shapes.Add(VectorShape());
    image.shapes[0].AddRect(Vec2F(), Vec2F(64, 8));
    stops[128].color = Color4(128, 128, 128, 255);
    image.shapes[0].fill = VectorPaint::Linear(Vec2F(0, 0), Vec2F(64, 0), stops);

    VectorMesh mesh;
    VectorTessellator::Tessellate(image, mesh);
    EXPECT_LT(mesh.GetTrianglesCount(), 120u);

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    for (int x = 0; x < 64; x += 7)
        EXPECT_NEAR(VectorRasterizer::GetPixel(*bitmap, x, 4).r, (x + 0.5f)/64.0f*256.0f, 2.0f);
}

TEST(VectorTessellator, RadialGradientCenterAndEdge)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(41, 41, "<radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"20.5\" cy=\"20.5\" r=\"20\">"
                                            "<stop offset=\"0\" stop-color=\"#ffffff\"/><stop offset=\"1\" stop-color=\"#000000\"/></radialGradient>"
                                            "<rect width=\"41\" height=\"41\" fill=\"url(#g)\"/>"));

    ExpectPixel(bitmap, 20, 20, Color4(255, 255, 255, 255), 6);
    ExpectPixel(bitmap, 30, 20, Color4(128, 128, 128, 255), 6);
    ExpectPixel(bitmap, 20, 10, Color4(128, 128, 128, 255), 6);
    ExpectPixel(bitmap, 27, 27, Color4(129, 129, 129, 255), 8);
    ExpectPixel(bitmap, 40, 20, Color4(0, 0, 0, 255), 6);
    ExpectPixel(bitmap, 0, 0, Color4(0, 0, 0, 255), 0);
}

TEST(VectorTessellator, GradientStopOpacityAndStrokeGradient)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(21, 6, "<linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"0.5\" x2=\"20.5\">"
                                           "<stop offset=\"0\" stop-color=\"#ff0000\" stop-opacity=\"0\"/>"
                                           "<stop offset=\"1\" stop-color=\"#ff0000\"/></linearGradient>"
                                           "<line x1=\"0\" y1=\"3\" x2=\"21\" y2=\"3\" stroke=\"url(#g)\" stroke-width=\"2\"/>"));

    EXPECT_EQ(Alpha(bitmap, 0, 2), 0);
    EXPECT_NEAR(Alpha(bitmap, 10, 3), 128, 1);
    EXPECT_EQ(Alpha(bitmap, 20, 2), 255);
    EXPECT_EQ(Alpha(bitmap, 10, 1), 0);
}

TEST(VectorTessellator, OpacityMultipliesAlpha)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(10, 10, "<g opacity=\"0.5\"><rect x=\"2\" y=\"2\" width=\"6\" height=\"6\" fill=\"#ff0000\" fill-opacity=\"0.5\"/></g>"));
    EXPECT_EQ(Alpha(bitmap, 5, 5), 64);
    EXPECT_EQ(CountAlpha(bitmap, 64, 64), 36);

    Ref<Bitmap> stroke = RenderSvg(Svg(10, 10, "<line x1=\"0\" y1=\"5\" x2=\"10\" y2=\"5\" stroke=\"rgba(0,0,0,0.5)\" stroke-width=\"2\" stroke-opacity=\"0.5\"/>"));
    EXPECT_EQ(Alpha(stroke, 5, 5), 64);
}

TEST(VectorTessellator, TranslucentFillHasUniformInterior)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(40, 40, "<path fill=\"#000\" fill-opacity=\"0.5\" d=\"M4 4 L36 6 L30 20 L36 36 L20 28 L4 36 L10 20 Z "
                                            "M20 10 A6 6 0 1 0 20 22 A6 6 0 1 0 20 10 Z\"/>"));

    EXPECT_EQ(CountAlpha(bitmap, 129, 255), 0);
    EXPECT_GT(CountAlpha(bitmap, 128, 128), 400);
    EXPECT_EQ(Alpha(bitmap, 20, 16), 0);
}

TEST(VectorTessellator, TranslucentStrokeWithRoundJoinsHasUniformInterior)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(60, 40, "<path fill=\"none\" stroke=\"#000\" stroke-opacity=\"0.5\" stroke-width=\"7\" stroke-linejoin=\"round\""
                                            " stroke-linecap=\"round\" d=\"M8 30 L20 8 L30 30 L42 10 L52 30 L30 34 C20 20 14 30 10 12\"/>"));

    EXPECT_EQ(CountAlpha(bitmap, 129, 255), 0);
    EXPECT_GT(CountAlpha(bitmap, 128, 128), 500);
}

TEST(VectorTessellator, TranslucentOverlappingSubPathsAreUnited)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(30, 30, "<path fill=\"#000\" fill-opacity=\"0.5\" d=\"M2 2 H18 V18 H2 Z M10 10 H28 V28 H10 Z\"/>"));
    EXPECT_EQ(CountAlpha(bitmap, 129, 255), 0);
    EXPECT_EQ(CountAlpha(bitmap, 128, 128), 256 + 324 - 64);
    EXPECT_EQ(CountAlpha(bitmap, 1, 127), 0);
}

TEST(VectorTessellator, SelfIntersectingStar)
{
    const char* star = "d=\"M20 2 L31 36 L2 15 L38 15 L9 36 Z\"";

    Ref<Bitmap> nonZero = RenderSvg(Svg(40, 40, String("<path ") + star + "/>"));
    EXPECT_EQ(Alpha(nonZero, 20, 20), 255);
    EXPECT_EQ(Alpha(nonZero, 20, 8), 255);

    Ref<Bitmap> evenOdd = RenderSvg(Svg(40, 40, String("<path fill-rule=\"evenodd\" ") + star + "/>"));
    EXPECT_EQ(Alpha(evenOdd, 20, 20), 0);
    EXPECT_EQ(Alpha(evenOdd, 20, 8), 255);
    EXPECT_GT(AlphaSum(nonZero), AlphaSum(evenOdd) + 50.0);

    Ref<Bitmap> translucent = RenderSvg(Svg(40, 40, String("<path fill-opacity=\"0.5\" ") + star + "/>"));
    EXPECT_EQ(CountAlpha(translucent, 129, 255), 0);
}

TEST(VectorTessellator, CoincidentAndDegenerateEdges)
{
    Ref<Bitmap> adjacent = RenderSvg(Svg(20, 20, "<path fill-opacity=\"0.5\" d=\"M2 2 H10 V18 H2 Z M10 2 H18 V18 H10 Z M4 4 H4 Z M5 5 L9 9 L5 5 Z\"/>"));
    EXPECT_EQ(CountAlpha(adjacent, 128, 128), 256);
    EXPECT_EQ(CountAlpha(adjacent, 1, 127), 0);
    EXPECT_EQ(CountAlpha(adjacent, 129, 255), 0);

    Ref<Bitmap> twice = RenderSvg(Svg(20, 20, "<path d=\"M2 2 H18 V18 H2 Z M2 2 H18 V18 H2 Z\"/>"));
    EXPECT_EQ(CountAlpha(twice, 255, 255), 256);

    Ref<Bitmap> twiceEvenOdd = RenderSvg(Svg(20, 20, "<path fill-rule=\"evenodd\" d=\"M2 2 H18 V18 H2 Z M2 2 H18 V18 H2 Z\"/>"));
    EXPECT_EQ(CountAlpha(twiceEvenOdd, 1, 255), 0);
}

TEST(VectorTessellator, NoAntialiasingGivesHardEdges)
{
    Ref<Bitmap> bitmap = RenderSvg(Svg(16, 16, "<rect x=\"3.3\" y=\"3.3\" width=\"9\" height=\"9\"/><circle cx=\"8\" cy=\"8\" r=\"7.2\" fill-rule=\"evenodd\"/>"),
                                1.0f, false);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_GT(CountAlpha(bitmap, 255, 255), 100);

    Ref<Bitmap> ring = RenderSvg(Svg(20, 20, "<path fill-rule=\"evenodd\" d=\"M2 2 H18 V18 H2 Z M6 6 H14 V14 H6 Z\"/>"), 1.0f, false);
    EXPECT_EQ(CountAlpha(ring, 255, 255), 256 - 64);
}

TEST(VectorTessellator, ScaleRendersMorePixels)
{
    String svg = Svg(16, 16, "<rect x=\"3\" y=\"3\" width=\"10\" height=\"10\"/>");

    Ref<Bitmap> doubled = RenderSvg(svg, 2.0f);
    EXPECT_EQ(doubled->GetSize(), Vec2I(32, 32));
    EXPECT_EQ(CountAlpha(doubled, 255, 255), 400);
    EXPECT_EQ(CountAlpha(doubled, 1, 254), 0);

    Ref<Bitmap> half = RenderSvg(svg, 0.5f);
    EXPECT_EQ(half->GetSize(), Vec2I(8, 8));
    EXPECT_NEAR(AlphaSum(half), 25.0, 0.1);
}

TEST(VectorTessellator, FringeIsOnePixelWideAroundTheEdge)
{
    String svg = Svg(16, 16, "<rect x=\"3\" y=\"3\" width=\"10\" height=\"10\"/>");

    for (float pixelScale : { 1.0f, 4.0f })
    {
        VectorMesh mesh = Tessellate(svg, pixelScale, true, false);
        ASSERT_GT(mesh.GetTrianglesCount(), 0u);
        ASSERT_EQ(mesh.positions.size(), mesh.colors.size());
        EXPECT_EQ(mesh.size, Vec2F(16, 16));

        float half = 0.5f/pixelScale;
        for (size_t i = 0; i < mesh.positions.size(); i++)
        {
            const Vec2F& position = mesh.positions[i];
            int alpha = (int)(mesh.colors[i] >> 24);
            float outside = Math::Max(Math::Max(3.0f - position.x, position.x - 13.0f),
                                      Math::Max(3.0f - position.y, position.y - 13.0f));

            EXPECT_LE(outside, half + 1e-4f);
            if (alpha == 0)
                EXPECT_NEAR(outside, half, 1e-4f);
            else if (alpha == 255)
                EXPECT_LE(outside, -half + 1e-4f);
        }
    }
}

TEST(VectorTessellator, EdgesOnPixelBoundsHaveNoFringe)
{
    String rect = Svg(16, 16, "<rect x=\"3\" y=\"3\" width=\"10\" height=\"10\"/>");

    VectorMesh mesh = Tessellate(rect);
    EXPECT_EQ(mesh.GetTrianglesCount(), 2u);
    for (Color32Bit color : mesh.colors)
        EXPECT_EQ(color >> 24, 255u);

    Vec2F min, max;
    ASSERT_TRUE(mesh.GetBounds(min, max));
    EXPECT_EQ(min, Vec2F(3, 3));
    EXPECT_EQ(max, Vec2F(13, 13));

    // The same edges are between the pixels of the doubled scale and inside of them at the scale 1.5
    EXPECT_EQ(Tessellate(rect, 2.0f).GetTrianglesCount(), 2u);
    EXPECT_GT(Tessellate(rect, 1.5f).GetTrianglesCount(), 2u);
    EXPECT_GT(Tessellate(rect, 1.0f, true, false).GetTrianglesCount(), 2u);

    String frame = Svg(16, 16, "<path fill-rule=\"evenodd\" d=\"M2 2 H14 V14 H2 Z M4 4 V12 H12 V4 Z\"/>");
    Ref<Bitmap> bitmap = RenderSvg(frame);
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 144 - 64);
    EXPECT_EQ(CountAlpha(bitmap, 1, 254), 0);
    EXPECT_LE(Tessellate(frame).GetTrianglesCount(), 12u);
}

TEST(VectorTessellator, HardEdgesMeetSlantedAndRoundOnes)
{
    // Pixels along the hard sides are covered exactly up to the corners with the other edges
    ExpectMaxDifference(Svg(20, 20, "<path d=\"M6 4 L15 10 L6 16 Z\"/>"), 6, 64);
    ExpectMaxDifference(Svg(20, 20, "<path d=\"M3 3 H17 V10 L10 17 H3 Z\"/>"), 6, 64);
    ExpectMaxDifference(Svg(20, 20, "<path d=\"M3 3 H17 V17 H3 Z M5.3 5.2 L9.7 6.1 L6.2 9.4 Z\" fill-rule=\"evenodd\"/>"), 6, 64);
    ExpectMaxDifference(Svg(24, 20, "<rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"4\"/>"), 6, 64);
    ExpectMaxDifference(Svg(24, 20, "<path d=\"M2 3 H22 V17 H2 Z M2.6 3.4 L21 10 L2.6 16.5 Z\" fill-rule=\"evenodd\"/>"), 6, 64);

    Ref<Bitmap> rounded = RenderSvg(Svg(24, 20, "<rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"4\"/>"));
    EXPECT_LE(MirrorDifference(rounded, 12.0f, 10.0f), 1);
    for (int x = 7; x < 17; x++)
    {
        EXPECT_EQ(Alpha(rounded, x, 3), 255) << x;
        EXPECT_EQ(Alpha(rounded, x, 2), 0) << x;
    }
}

TEST(VectorTessellator, GradientStopsOnTheShapeEdgesLeaveNoHoles)
{
    String svg = Svg(100, 40,
        "<defs><linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"12\" y1=\"19.5\" x2=\"87\" y2=\"19.5\">"
        "<stop offset=\"0\" stop-color=\"#009c8d\"/><stop offset=\"0.5\" stop-color=\"#00a696\"/>"
        "<stop offset=\"0.812\" stop-color=\"#00aa99\"/><stop offset=\"1\" stop-color=\"#00ab9a\"/></linearGradient></defs>"
        "<path d=\"M13.014 10 C12.552 10.19 12.183 10.581 12 11.072 L12 29 L87 29 L87 11.072 C86.817 10.581 86.448 10.19"
        " 85.986 10 Z\" fill=\"url(#g)\"/>");
    Ref<Bitmap> bitmap = RenderSvg(svg);

    for (int y = 12; y < 29; y++)
    {
        for (int x = 12; x < 87; x++)
            ASSERT_EQ(Alpha(bitmap, x, y), 255) << x << "," << y;
    }

    Ref<Bitmap> box = RenderSvg(Svg(20, 20,
        "<defs><radialGradient id=\"g\"><stop offset=\"0\" stop-color=\"#f00\"/><stop offset=\"1\" stop-color=\"#00f\"/>"
        "</radialGradient></defs><rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"url(#g)\"/>"));
    EXPECT_EQ(CountAlpha(box, 255, 255), 256);
}

TEST(VectorTessellator, MeshesOfUiShapesAreSmall)
{
    EXPECT_LE(Tessellate(Svg(20, 20, "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" rx=\"3\"/>")).GetTrianglesCount(), 140u);
    EXPECT_LE(Tessellate(Svg(20, 20, "<circle cx=\"10\" cy=\"10\" r=\"6\"/>")).GetTrianglesCount(), 260u);
    EXPECT_LE(Tessellate(Svg(20, 20, "<rect x=\"2.5\" y=\"2.5\" width=\"15\" height=\"15\" fill=\"none\" stroke=\"#000\"/>"))
              .GetTrianglesCount(), 12u);
    EXPECT_LE(Tessellate(Svg(20, 20, "<path d=\"M4 10 L8 14 L16 5\" fill=\"none\" stroke=\"#000\" stroke-width=\"2\"/>"))
              .GetTrianglesCount(), 170u);
}

TEST(VectorTessellator, TessellationIsDeterministic)
{
    String svg = Svg(40, 40, "<circle cx=\"20\" cy=\"20\" r=\"15\" fill=\"#f00\" stroke=\"#00f\" stroke-width=\"3\"/>"
                             "<path d=\"M20 2 L31 36 L2 15 L38 15 L9 36 Z\" fill-opacity=\"0.5\"/>");

    VectorMesh first = Tessellate(svg), second = Tessellate(svg);
    EXPECT_EQ(first.positions, second.positions);
    EXPECT_EQ(first.colors, second.colors);
    EXPECT_EQ(first.indexes, second.indexes);
}

TEST(VectorTessellator, SharpSpikeDoesNotExplode)
{
    VectorMesh mesh = Tessellate(Svg(40, 20, "<polygon points=\"2,9 38,10 2,11\"/>"));
    ASSERT_GT(mesh.GetTrianglesCount(), 0u);

    for (const Vec2F& position : mesh.positions)
    {
        EXPECT_GT(position.x, 0.9f);
        EXPECT_LT(position.x, 39.1f);
        EXPECT_GT(position.y, 7.9f);
        EXPECT_LT(position.y, 12.1f);
    }

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    EXPECT_NEAR(AlphaSum(bitmap), 36.0, 2.0);
    EXPECT_NEAR(Alpha(bitmap, 20, 9), 127, 8);
    EXPECT_NEAR(Alpha(bitmap, 20, 10), 127, 8);
    EXPECT_NEAR(Alpha(bitmap, 29, 9), 64, 8);
    EXPECT_NEAR(Alpha(bitmap, 29, 10), 64, 8);

    Ref<Bitmap> stroked = RenderSvg(Svg(40, 40, "<path d=\"M4 30 L36 20 L4 26\" fill=\"none\" stroke=\"#000\" stroke-width=\"3\" stroke-miterlimit=\"100\"/>"));
    EXPECT_GT(CountAlpha(stroked, 255, 255), 50);
}

TEST(VectorTessellator, SmallDotsKeepTheirArea)
{
    const float pi = 3.14159265f;

    for (const char* center : { "cx=\"6.5\" cy=\"6.5\"", "cx=\"6\" cy=\"6\"", "cx=\"6.19\" cy=\"5.7\"" })
    {
        Ref<Bitmap> one = RenderSvg(Svg(12, 12, String("<circle ") + center + " r=\"1\"/>"));
        EXPECT_NEAR(AlphaSum(one), pi, pi*0.12f) << center;

        Ref<Bitmap> half = RenderSvg(Svg(12, 12, String("<circle ") + center + " r=\"0.5\"/>"));
        EXPECT_NEAR(AlphaSum(half), pi*0.25f, pi*0.25f*0.4f) << center;

        Ref<Bitmap> tiny = RenderSvg(Svg(12, 12, String("<circle ") + center + " r=\"0.3\"/>"));
        EXPECT_GT(AlphaSum(tiny), pi*0.09f*0.25f) << center;
        EXPECT_LT(AlphaSum(tiny), pi*0.09f*2.5f) << center;
    }
}

TEST(VectorTessellator, ShortEdgesBetweenLongOnesDoNotDoubleCoverage)
{
    Ref<Bitmap> ring = RenderSvg(Svg(18, 18, "<circle cx=\"8.5\" cy=\"8.5\" r=\"5\" fill=\"none\" stroke=\"#000\" stroke-width=\"1.6\"/>"));
    EXPECT_NEAR(Alpha(ring, 8, 4), 80, 12);
    EXPECT_NEAR(Alpha(ring, 4, 8), 80, 12);
    EXPECT_NEAR(Alpha(ring, 8, 12), 80, 12);
    EXPECT_NEAR(Alpha(ring, 12, 8), 80, 12);

    Ref<Bitmap> chamfer = RenderSvg(Svg(12, 12, "<polygon fill-opacity=\"0.5\" points=\"2,2 9.95,2 10,2.05 10,10 2,10\"/>"));
    EXPECT_EQ(CountAlpha(chamfer, 129, 255), 0);
    EXPECT_EQ(CountAlpha(chamfer, 127, 128), 64);
}

TEST(VectorTessellator, TessellateShapeAppendsToMesh)
{
    VectorShape shape;
    shape.AddRect(Vec2F(1, 1), Vec2F(4, 4));

    VectorMesh mesh;
    mesh.size = Vec2F(8, 8);
    VectorTessellator::TessellateShape(shape, mesh);
    UInt triangles = mesh.GetTrianglesCount();
    ASSERT_GT(triangles, 0u);

    shape.fill = VectorPaint::Solid(Color4(255, 0, 0, 255));
    shape.Transform(Basis::Translated(Vec2F(2, 2)));
    VectorTessellator::TessellateShape(shape, mesh);
    EXPECT_EQ(mesh.GetTrianglesCount(), triangles*2);

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 1, 1), Color4(0, 0, 0, 255));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 4, 4), Color4(255, 0, 0, 255));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 7, 7).a, 0);
}

TEST(VectorTessellator, EmptyAndDegenerateShapesGiveNoTriangles)
{
    EXPECT_EQ(Tessellate(Svg(10, 10, "")).GetTrianglesCount(), 0u);
    EXPECT_EQ(Tessellate(Svg(10, 10, "<path d=\"M1 1 L5 5\"/>")).GetTrianglesCount(), 0u);
    EXPECT_EQ(Tessellate(Svg(10, 10, "<path d=\"M1 1 L5 5 L9 9 Z\"/>")).GetTrianglesCount(), 0u);
    EXPECT_EQ(Tessellate(Svg(10, 10, "<rect width=\"5\" height=\"5\" opacity=\"0\"/>")).GetTrianglesCount(), 0u);
    EXPECT_EQ(Tessellate(Svg(10, 10, "<rect width=\"5\" height=\"5\" fill=\"rgba(0,0,0,0)\"/>")).GetTrianglesCount(), 0u);

    VectorImage image;
    image.size = Vec2F(10, 10);
    image.shapes.Add(VectorShape());
    image.shapes[0].AddPolyline({ Vec2F(1, 1), Vec2F(NAN, 5), Vec2F(5, 9) }, true);
    image.shapes[0].stroke = VectorPaint::Solid(Color4::Black());

    VectorMesh mesh;
    VectorTessellator::Tessellate(image, mesh);
    EXPECT_EQ(mesh.GetTrianglesCount(), 0u);
}

TEST(VectorTessellator, ColorsAreStraightAlphaAbgr)
{
    VectorMesh mesh = Tessellate(Svg(10, 10, "<rect x=\"2\" y=\"2\" width=\"6\" height=\"6\" fill=\"#102030\"/>"));
    ASSERT_GT(mesh.colors.size(), 0u);
    for (Color32Bit color : mesh.colors)
        EXPECT_EQ(color & 0x00FFFFFF, Color4(0x10, 0x20, 0x30, 0).ABGR() & 0x00FFFFFF);

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));
    ExpectPixel(bitmap, 5, 5, Color4(0x10, 0x20, 0x30, 255), 0);
}

TEST(VectorTessellator, ShapesAreDrawnInOrder)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Svg(10, 10, "<rect width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
                                                     "<rect x=\"2\" y=\"2\" width=\"6\" height=\"6\" fill=\"#0000ff\" fill-opacity=\"0.5\"/>"
                                                     "<rect x=\"4\" y=\"4\" width=\"2\" height=\"2\" fill=\"#00ff00\"/>"));
    ExpectPixel(bitmap, 0, 0, Color4(255, 0, 0, 255), 0);
    ExpectPixel(bitmap, 3, 3, Color4(127, 0, 128, 255), 1);
    ExpectPixel(bitmap, 4, 4, Color4(0, 255, 0, 255), 0);
}

TEST(VectorTessellator, FillAndStrokeOfOneShape)
{
    Ref<Bitmap> bitmap = RenderOverWhite(Svg(20, 20, "<rect x=\"5\" y=\"5\" width=\"10\" height=\"10\" fill=\"#ff0000\" stroke=\"#0000ff\" stroke-width=\"2\"/>"));
    ExpectPixel(bitmap, 10, 10, Color4(255, 0, 0, 255), 0);
    ExpectPixel(bitmap, 4, 10, Color4(0, 0, 255, 255), 0);
    ExpectPixel(bitmap, 5, 10, Color4(0, 0, 255, 255), 0);
    ExpectPixel(bitmap, 6, 10, Color4(255, 0, 0, 255), 0);
    ExpectPixel(bitmap, 3, 10, Color4(255, 255, 255, 255), 0);
}

TEST(VectorTessellator, StronglyAnisotropicScaleIsBuiltAsModeratelyAnisotropic)
{
    VectorImage image = Parse(Svg(30, 30, "<defs><radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"15\" cy=\"15\" r=\"14\">"
                                          "<stop offset=\"0\" stop-color=\"#000000\" stop-opacity=\"0.3\"/>"
                                          "<stop offset=\"1\" stop-color=\"#000000\" stop-opacity=\"0\"/></radialGradient></defs>"
                                          "<rect x=\"1\" y=\"1\" width=\"28\" height=\"28\" rx=\"6\" fill=\"url(#g)\"/>"));

    VectorTessellationParams degenerate;
    degenerate.pixelScale = Vec2F(32.0f, 0.0001f);

    VectorTessellationParams limited;
    limited.pixelScale = Vec2F(32.0f, 2.0f);

    VectorMesh degenerateMesh, limitedMesh;
    VectorTessellator::Tessellate(image, degenerateMesh, degenerate);
    VectorTessellator::Tessellate(image, limitedMesh, limited);

    EXPECT_GT(limitedMesh.GetTrianglesCount(), 0u);
    EXPECT_EQ(degenerateMesh.GetTrianglesCount(), limitedMesh.GetTrianglesCount());
    EXPECT_EQ(degenerateMesh.pixelScale, limitedMesh.pixelScale);
}

TEST(VectorTessellator, HugeStrokeWidthIsNotDrawn)
{
    VectorShape shape;
    shape.AddPolyline({ Vec2F(2, 10), Vec2F(18, 10) }, false);
    shape.stroke = VectorPaint::Solid(Color4::Black());
    shape.strokeWidth = std::numeric_limits<float>::infinity();

    VectorMesh mesh;
    VectorTessellator::TessellateShape(shape, mesh);
    EXPECT_EQ(mesh.GetTrianglesCount(), 0u);

    shape.strokeWidth = 1e30f;
    VectorTessellator::TessellateShape(shape, mesh);
    EXPECT_EQ(mesh.GetTrianglesCount(), 0u);
}
