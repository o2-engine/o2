#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    const char* kCheckIcon =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 20 20\">"
        "<path d=\"M4.5 10.5 L8.5 14.5 L15.5 5.5\" fill=\"none\" stroke=\"#3c4654\" stroke-width=\"2\""
        " stroke-linecap=\"round\" stroke-linejoin=\"round\"/></svg>";

    const char* kCloseIcon =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 20 20\">"
        "<g stroke=\"#d04848\" stroke-width=\"1.5\" stroke-linecap=\"round\">"
        "<line x1=\"5.5\" y1=\"5.5\" x2=\"14.5\" y2=\"14.5\"/><line x1=\"14.5\" y1=\"5.5\" x2=\"5.5\" y2=\"14.5\"/></g></svg>";

    const char* kFolderIcon =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 20 20\">"
        "<path fill=\"#e8b548\" d=\"M2 5.5 C2 4.7 2.7 4 3.5 4 H7.6 L9.4 6 H16.5 C17.3 6 18 6.7 18 7.5 V14.5 "
        "C18 15.3 17.3 16 16.5 16 H3.5 C2.7 16 2 15.3 2 14.5 Z\"/>"
        "<rect x=\"2\" y=\"8\" width=\"16\" height=\"1\" fill=\"#b88a2c\"/>"
        "<circle cx=\"14\" cy=\"12.5\" r=\"1.5\" fill=\"#fff\" fill-opacity=\"0.8\"/></svg>";

    const char* kSearchIcon =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 20 20\">"
        "<circle cx=\"8.5\" cy=\"8.5\" r=\"5\" fill=\"none\" stroke=\"#56667c\" stroke-width=\"1.6\"/>"
        "<path d=\"M12.3 12.3 L16.5 16.5\" stroke=\"#56667c\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg>";

    const char* kArrowIcon =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"20\" viewBox=\"0 0 20 20\">"
        "<path d=\"M6 4 L15 10 L6 16 Z\" fill=\"#4a90d9\"/>"
        "<circle cx=\"3\" cy=\"6\" r=\"1\"/><circle cx=\"3\" cy=\"10\" r=\"1\"/><circle cx=\"3\" cy=\"14\" r=\"1\"/></svg>";

    const char* kButton =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"24\" viewBox=\"0 0 80 24\">"
        "<defs><linearGradient id=\"face\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\">"
        "<stop offset=\"0\" stop-color=\"#fbfcfd\"/><stop offset=\"0.5\" stop-color=\"#e4e9ef\"/>"
        "<stop offset=\"1\" stop-color=\"#c6cfdb\"/></linearGradient></defs>"
        "<rect x=\"0.5\" y=\"0.5\" width=\"79\" height=\"23\" rx=\"4\" fill=\"url(#face)\" stroke=\"#8996a8\"/>"
        "<rect x=\"1.5\" y=\"1.5\" width=\"77\" height=\"21\" rx=\"3\" fill=\"none\" stroke=\"#fff\" stroke-opacity=\"0.6\"/></svg>";

    const char* kPanel =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"48\" viewBox=\"0 0 64 48\">"
        "<defs><linearGradient id=\"head\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"1\" x2=\"0\" y2=\"13\">"
        "<stop offset=\"0\" stop-color=\"#6d7f97\"/><stop offset=\"1\" stop-color=\"#4d5d74\"/></linearGradient>"
        "<radialGradient id=\"glow\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\"><stop offset=\"0\" stop-color=\"#fff\" stop-opacity=\"0.5\"/>"
        "<stop offset=\"1\" stop-color=\"#fff\" stop-opacity=\"0\"/></radialGradient></defs>"
        "<rect x=\"1\" y=\"1\" width=\"62\" height=\"46\" rx=\"6\" fill=\"#eef1f5\"/>"
        "<path d=\"M1 13 V7 C1 3.7 3.7 1 7 1 H57 C60.3 1 63 3.7 63 7 V13 Z\" fill=\"url(#head)\"/>"
        "<rect x=\"1\" y=\"13\" width=\"62\" height=\"1\" fill=\"#2f3a4b\"/>"
        "<rect x=\"1\" y=\"14\" width=\"62\" height=\"0.5\" fill=\"#fff\"/>"
        "<circle cx=\"32\" cy=\"30\" r=\"12\" fill=\"url(#glow)\"/>"
        "<rect x=\"0.5\" y=\"0.5\" width=\"63\" height=\"47\" rx=\"6.5\" fill=\"none\" stroke=\"#1d2530\" stroke-opacity=\"0.7\"/></svg>";

    void ExpectFidelity(const char* svg, float minSimilarity, float maxMeanDifference, UInt maxTriangles)
    {
        Fidelity fidelity = MeasureFidelity(svg, 24);
        EXPECT_TRUE(fidelity.warnings.IsEmpty());
        EXPECT_TRUE(fidelity.result.comparable);
        EXPECT_GE(fidelity.result.similarity, minSimilarity);
        EXPECT_LE(fidelity.result.meanDifference, maxMeanDifference);
        EXPECT_LE(fidelity.result.maxDifference, 24);
        EXPECT_GT(fidelity.triangles, 0u);
        EXPECT_LE(fidelity.triangles, maxTriangles);

        printf("  similarity %.4f, mean difference %.3f, max difference %d, %u triangles\n", fidelity.result.similarity,
               fidelity.result.meanDifference, fidelity.result.maxDifference, fidelity.triangles);
    }
}

TEST(VectorFixtures, CheckIconWithRoundStroke)
{
    ExpectFidelity(kCheckIcon, 0.99f, 1.0f, 600);
}

TEST(VectorFixtures, CloseIconWithCrossedStrokes)
{
    ExpectFidelity(kCloseIcon, 0.99f, 1.0f, 600);
}

TEST(VectorFixtures, FolderIconWithCurvesAndThinRect)
{
    ExpectFidelity(kFolderIcon, 0.99f, 1.0f, 600);
}

TEST(VectorFixtures, SearchIconWithRingStroke)
{
    ExpectFidelity(kSearchIcon, 0.99f, 1.1f, 600);
}

TEST(VectorFixtures, ArrowIconWithSmallDots)
{
    ExpectFidelity(kArrowIcon, 0.99f, 1.0f, 600);
}

TEST(VectorFixtures, RoundedButtonWithGradientAndStrokes)
{
    ExpectFidelity(kButton, 0.995f, 0.5f, 800);
}

TEST(VectorFixtures, PanelWithGradientsAndSeparators)
{
    ExpectFidelity(kPanel, 0.995f, 0.5f, 3200);
}

TEST(VectorFixtures, ButtonKeepsCornersWhenSliced)
{
    VectorMesh mesh;
    VectorTessellator::Tessellate(Parse(kButton), mesh);
    Ref<Bitmap> source = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));

    BorderF borders(6, 6, 6, 6);
    Vec2F targetSize(120, 24);
    mesh.SplitBySlices(borders);
    for (Vec2F& position : mesh.positions)
        position = VectorMesh::MapSlicedPoint(position, mesh.size, borders, targetSize);

    mesh.size = targetSize;
    Ref<Bitmap> wide = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));

    for (int y = 0; y < 24; y++)
    {
        for (int x = 0; x < 6; x++)
        {
            ExpectSamePixel(source, x, y, wide, x, y);
            ExpectSamePixel(source, 79 - x, y, wide, 119 - x, y);
        }

        ExpectSamePixel(source, 40, y, wide, 60, y);
    }
}
