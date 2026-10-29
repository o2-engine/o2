#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    VectorMesh ScalableMesh(const VectorImage& image)
    {
        VectorTessellationParams params;
        params.scalable = true;
        params.pixelScale = Vec2F(4, 4);

        VectorMesh mesh;
        VectorTessellator::Tessellate(image, mesh, params);
        return mesh;
    }

    // Compares the mesh built once and put to the scale with the mesh tessellated for this scale
    BitmapCompareResult CompareAtScale(const String& svg, float scale)
    {
        VectorImage image = Parse(svg);

        VectorTessellationParams params;
        params.pixelScale = Vec2F(scale, scale);
        params.pixelSnapped = false;

        VectorMesh exact;
        VectorTessellator::Tessellate(image, exact, params);

        VectorMesh scalable = ScalableMesh(image);
        scalable.Rescale(Vec2F(scale, scale));

        Ref<Bitmap> expected = VectorRasterizer::Rasterize(exact, scale);
        Ref<Bitmap> actual = VectorRasterizer::Rasterize(scalable, scale);
        return BitmapCompare::Compare(*expected, *actual, 24);
    }

    void ExpectSameAtScales(const String& svg, float minSimilarity = 0.99f)
    {
        for (float scale : { 0.5f, 1.0f, 1.5f, 2.0f, 3.3f, 8.0f })
        {
            BitmapCompareResult result = CompareAtScale(svg, scale);
            EXPECT_TRUE(result.comparable) << scale;
            EXPECT_GE(result.similarity, minSimilarity) << "scale " << scale << " max " << result.maxDifference;
            EXPECT_LE(result.meanDifference, 1.5f) << "scale " << scale;
        }
    }
}

TEST(VectorScalableMesh, RectangleIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40, "<rect x=\"8\" y=\"6\" width=\"20\" height=\"25\" fill=\"#204080\"/>"));
}

TEST(VectorScalableMesh, RectangleOffThePixelsIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40, "<rect x=\"8.3\" y=\"6.7\" width=\"20.4\" height=\"25.1\" fill=\"#204080\"/>"));
}

TEST(VectorScalableMesh, CircleWithHoleIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40,
        "<path fill-rule=\"evenodd\" fill=\"#c03020\" d=\"M20 4a16 16 0 1 0 0 32a16 16 0 1 0 0-32z"
        "M20 12a8 8 0 1 0 0 16a8 8 0 1 0 0-16z\"/>"));
}

TEST(VectorScalableMesh, StrokeIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40,
        "<path fill=\"none\" stroke=\"#108040\" stroke-width=\"3\" stroke-linejoin=\"round\" stroke-linecap=\"round\" "
        "d=\"M6 30L14 8L24 28L34 10\"/>"));
}

TEST(VectorScalableMesh, FillUnderStrokeIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40,
        "<rect x=\"6.5\" y=\"6.5\" width=\"27\" height=\"27\" rx=\"5\" fill=\"#e0e0f0\" stroke=\"#303060\" "
        "stroke-width=\"2\"/>"));
}

TEST(VectorScalableMesh, GradientIsTheSameAtAnyScale)
{
    ExpectSameAtScales(Svg(40, 40,
        "<defs><linearGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"5\" x2=\"0\" y2=\"35\">"
        "<stop offset=\"0\" stop-color=\"#fff\"/><stop offset=\"1\" stop-color=\"#246\"/></linearGradient></defs>"
        "<circle cx=\"20\" cy=\"20\" r=\"15\" fill=\"url(#g)\"/>"));
}

TEST(VectorScalableMesh, ThinLineKeepsItsCoverage)
{
    ExpectSameAtScales(Svg(40, 40, "<rect x=\"5\" y=\"10.2\" width=\"30\" height=\"0.4\" fill=\"#000\"/>"
                                   "<rect x=\"12.3\" y=\"15\" width=\"0.5\" height=\"20\" fill=\"#000\"/>"), 0.97f);
}

// No pixel is off the mesh of the scale more than an antialiasing difference
void ExpectNoArtifactsAtScales(const String& svg)
{
    for (float scale : { 2.0f, 4.0f, 10.0f })
    {
        BitmapCompareResult result = CompareAtScale(svg, scale);
        ASSERT_TRUE(result.comparable) << scale;
        EXPECT_LE(result.maxDifference, 48) << "scale " << scale;
    }
}

// A band between two rounded rectangles ends in a spike of almost zero angle, as the inset bands of the WordFall tiles
TEST(VectorScalableMesh, SpikeOfABandStaysInside)
{
    ExpectNoArtifactsAtScales(Svg(40, 40,
        "<path fill=\"#000\" d=\"M8 1A7 7 0 0 0 1 8L1 24A7 7 0 0 0 8 31L9 31A7 7 0 0 1 2 24L2 8A7 7 0 0 1 9 1Z\"/>"));
}

// Skins of a corner rounder than they are deep would overlap, a translucent fill shows it
TEST(VectorScalableMesh, TightCornerOfTranslucentFillHasNoOverlaps)
{
    ExpectNoArtifactsAtScales(Svg(40, 40,
        "<rect x=\"4\" y=\"4\" width=\"30\" height=\"30\" rx=\"0.4\" fill=\"#000\" fill-opacity=\"0.4\"/>"
        "<path fill=\"#000\" fill-opacity=\"0.4\" d=\"M20 36L10 18Q9.6 17.2 10.4 17L29.6 17Q30.4 17.2 30 18Z\"/>"));
}

TEST(VectorScalableMesh, RescaleChangesNoTriangles)
{
    VectorMesh mesh = ScalableMesh(Parse(Svg(40, 40, "<circle cx=\"20\" cy=\"20\" r=\"15\" fill=\"#135\"/>")));
    ASSERT_TRUE(mesh.IsScalable());

    Vector<VertexIndex> indexes = mesh.indexes;
    int vertices = mesh.positions.Count();

    mesh.Rescale(Vec2F(7, 7));

    EXPECT_EQ(indexes, mesh.indexes);
    EXPECT_EQ(vertices, mesh.positions.Count());
    EXPECT_EQ(Vec2F(7, 7), mesh.pixelScale);
}

TEST(VectorScalableMesh, SlicedMeshIsDrawnTheSame)
{
    VectorImage image = Parse(Svg(40, 40,
        "<rect x=\"2.5\" y=\"2.5\" width=\"35\" height=\"35\" rx=\"8\" fill=\"#fc4\" stroke=\"#420\"/>"));

    VectorMesh whole = ScalableMesh(image);
    VectorMesh sliced = whole;
    sliced.SplitBySlices(BorderF(10, 10, 10, 10));
    EXPECT_GT(sliced.GetTrianglesCount(), whole.GetTrianglesCount());

    for (float scale : { 1.0f, 2.5f })
    {
        whole.Rescale(Vec2F(scale, scale));
        sliced.Rescale(Vec2F(scale, scale));

        Ref<Bitmap> expected = VectorRasterizer::Rasterize(whole, scale);
        Ref<Bitmap> actual = VectorRasterizer::Rasterize(sliced, scale);
        BitmapCompareResult result = BitmapCompare::Compare(*expected, *actual, 8);
        EXPECT_GE(result.similarity, 0.995f) << scale;
    }
}
