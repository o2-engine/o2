#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Bitmap/BitmapCompare.h"

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    const char* kPanel = "<linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\"><stop offset=\"0\" stop-color=\"#e0e8f0\"/>"
                         "<stop offset=\"1\" stop-color=\"#6078a0\"/></linearGradient>"
                         "<rect x=\"1.5\" y=\"1.5\" width=\"21\" height=\"21\" rx=\"5\" fill=\"url(#g)\" stroke=\"#203050\"/>";

    VectorMesh PanelMesh()
    {
        VectorMesh mesh;
        VectorTessellator::Tessellate(Parse(Svg(24, 24, kPanel)), mesh);
        return mesh;
    }

    VectorMesh SingleTriangle()
    {
        VectorMesh mesh;
        mesh.size = Vec2F(10, 10);
        mesh.positions = { Vec2F(0, 0), Vec2F(10, 0), Vec2F(0, 10) };
        mesh.colors = { Color4(0, 0, 0, 255).ABGR(), Color4(200, 100, 50, 255).ABGR(), Color4(0, 0, 0, 55).ABGR() };
        mesh.indexes = { 0, 1, 2 };
        return mesh;
    }

    bool HasStraddlingTriangle(const VectorMesh& mesh, bool vertical, float coordinate)
    {
        for (size_t i = 0; i + 2 < mesh.indexes.size(); i += 3)
        {
            bool less = false, greater = false;
            for (int k = 0; k < 3; k++)
            {
                const Vec2F& position = mesh.positions[mesh.indexes[i + k]];
                float value = vertical ? position.x : position.y;
                less = less || value < coordinate - 1e-4f;
                greater = greater || value > coordinate + 1e-4f;
            }

            if (less && greater)
                return true;
        }

        return false;
    }
}

TEST(VectorMeshSlices, SplitByLineInterpolatesPositionsAndColors)
{
    VectorMesh mesh = SingleTriangle();
    mesh.SplitByLine(true, 5.0f);

    EXPECT_EQ(mesh.GetTrianglesCount(), 3u);
    EXPECT_FALSE(HasStraddlingTriangle(mesh, true, 5.0f));

    bool foundTop = false, foundDiagonal = false;
    for (size_t i = 0; i < mesh.positions.size(); i++)
    {
        Color4 color;
        color.SetABGR(mesh.colors[i]);

        if (mesh.positions[i] == Vec2F(5, 0))
        {
            foundTop = true;
            EXPECT_EQ(color, Color4(100, 50, 25, 255));
        }

        if (mesh.positions[i] == Vec2F(5, 5))
        {
            foundDiagonal = true;
            EXPECT_EQ(color, Color4(100, 50, 25, 155));
        }
    }

    EXPECT_TRUE(foundTop);
    EXPECT_TRUE(foundDiagonal);
}

TEST(VectorMeshSlices, SplitByLineOutsideOfMeshChangesNothing)
{
    VectorMesh mesh = SingleTriangle();
    mesh.SplitByLine(true, 20.0f);
    mesh.SplitByLine(false, 0.0f);
    EXPECT_EQ(mesh.GetTrianglesCount(), 1u);
    EXPECT_EQ(mesh.positions.size(), 3u);
}

TEST(VectorMeshSlices, SplitBySlicesLeavesNoStraddlingTriangles)
{
    VectorMesh mesh = PanelMesh();
    UInt trianglesBefore = mesh.GetTrianglesCount();

    mesh.SplitBySlices(BorderF(7, 6, 8, 9));

    EXPECT_GT(mesh.GetTrianglesCount(), trianglesBefore);
    EXPECT_FALSE(HasStraddlingTriangle(mesh, true, 7.0f));
    EXPECT_FALSE(HasStraddlingTriangle(mesh, true, 24.0f - 8.0f));
    EXPECT_FALSE(HasStraddlingTriangle(mesh, false, 9.0f));
    EXPECT_FALSE(HasStraddlingTriangle(mesh, false, 24.0f - 6.0f));
}

TEST(VectorMeshSlices, SplitPreservesRendering)
{
    VectorMesh mesh = PanelMesh();
    Ref<Bitmap> before = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));

    mesh.SplitBySlices(BorderF(7, 6, 8, 9));
    Ref<Bitmap> after = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));

    BitmapCompareResult result = BitmapCompare::Compare(*before, *after, 1);
    EXPECT_FLOAT_EQ(result.similarity, 1.0f);
    EXPECT_LE(result.maxDifference, 1);
}

TEST(VectorMeshSlices, MapSlicedPoint)
{
    Vec2F imageSize(24, 24), targetSize(64, 40);
    BorderF borders(6, 7, 8, 5);

    auto map = [&](float x, float y) { return VectorMesh::MapSlicedPoint(Vec2F(x, y), imageSize, borders, targetSize); };

    EXPECT_EQ(map(0, 0), Vec2F(0, 0));
    EXPECT_EQ(map(3, 2), Vec2F(3, 2));
    EXPECT_EQ(map(6, 5), Vec2F(6, 5));
    EXPECT_EQ(map(24, 24), Vec2F(64, 40));
    EXPECT_EQ(map(20, 20), Vec2F(60, 36));
    EXPECT_EQ(map(16, 17), Vec2F(56, 33));
    EXPECT_EQ(map(11, 11), Vec2F(31, 19));
    EXPECT_EQ(map(-1, 25), Vec2F(-1, 41));
    EXPECT_EQ(VectorMesh::MapSlicedPoint(Vec2F(5, 9), imageSize, borders, imageSize), Vec2F(5, 9));
}

TEST(VectorMeshSlices, BordersShrinkWhenTargetIsSmaller)
{
    Vec2F imageSize(24, 24);
    BorderF borders(8, 8, 8, 8);

    auto map = [&](float x) { return VectorMesh::MapSlicedPoint(Vec2F(x, x), imageSize, borders, Vec2F(8, 8)).x; };

    EXPECT_FLOAT_EQ(map(0), 0.0f);
    EXPECT_FLOAT_EQ(map(8), 4.0f);
    EXPECT_FLOAT_EQ(map(12), 4.0f);
    EXPECT_FLOAT_EQ(map(16), 4.0f);
    EXPECT_FLOAT_EQ(map(24), 8.0f);
}

TEST(VectorMeshSlices, RemappedMeshKeepsCornersAndStretchesCenter)
{
    BorderF borders(7, 7, 7, 7);
    Vec2F targetSize(48, 36);

    VectorMesh mesh = PanelMesh();
    Ref<Bitmap> source = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));

    mesh.SplitBySlices(borders);
    for (Vec2F& position : mesh.positions)
        position = VectorMesh::MapSlicedPoint(position, mesh.size, borders, targetSize);

    mesh.size = targetSize;
    Ref<Bitmap> stretched = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(255, 255, 255, 255));
    ASSERT_EQ(stretched->GetSize(), Vec2I(48, 36));

    for (int y = 0; y < 7; y++)
    {
        for (int x = 0; x < 7; x++)
        {
            ExpectSamePixel(source, x, y, stretched, x, y);
            ExpectSamePixel(source, 23 - x, y, stretched, 47 - x, y);
            ExpectSamePixel(source, x, 23 - y, stretched, x, 35 - y);
            ExpectSamePixel(source, 23 - x, 23 - y, stretched, 47 - x, 35 - y);
        }
    }

    for (int y = 8; y < 28; y += 4)
        ExpectSamePixel(source, 1, 12, stretched, 1, y);

    Color4 top = VectorRasterizer::GetPixel(*stretched, 24, 3);
    Color4 center = VectorRasterizer::GetPixel(*stretched, 24, 18);
    Color4 bottom = VectorRasterizer::GetPixel(*stretched, 24, 32);
    EXPECT_GT(top.r, center.r);
    EXPECT_GT(center.r, bottom.r);
    ExpectSamePixel(stretched, 10, 18, stretched, 38, 18);
}

TEST(VectorMeshSlices, BordersAreCutAsSpriteDoesWhenTargetIsSmaller)
{
    Vec2F imageSize(30, 30);
    BorderF borders(10, 0, 4, 0);

    auto map = [&](float x) { return VectorMesh::MapSlicedPoint(Vec2F(x, 0), imageSize, borders, Vec2F(8, 30)).x; };

    EXPECT_FLOAT_EQ(map(0), 0.0f);
    EXPECT_FLOAT_EQ(map(7), 7.0f);
    EXPECT_FLOAT_EQ(map(10), 7.0f);
    EXPECT_FLOAT_EQ(map(20), 7.0f);
    EXPECT_FLOAT_EQ(map(29), 7.0f);
    EXPECT_FLOAT_EQ(map(29.5f), 7.5f);
    EXPECT_FLOAT_EQ(map(30), 8.0f);
}

TEST(VectorMeshSlices, BoundsIncludeTheFringe)
{
    VectorMesh mesh;
    Vec2F min, max;
    EXPECT_FALSE(mesh.GetBounds(min, max));

    VectorTessellationParams params;
    params.pixelScale = Vec2F(2, 2);
    params.pixelSnapped = false;
    VectorTessellator::Tessellate(Parse(Svg(10, 10, "<rect width=\"10\" height=\"10\"/>")), mesh, params);

    EXPECT_EQ(mesh.pixelScale, Vec2F(2, 2));
    ASSERT_TRUE(mesh.GetBounds(min, max));
    EXPECT_EQ(min, Vec2F(-0.25f, -0.25f));
    EXPECT_EQ(max, Vec2F(10.25f, 10.25f));
}

TEST(VectorMeshSlices, FillVertices)
{
    VectorMesh mesh;
    mesh.size = Vec2F(10, 20);
    mesh.positions = { Vec2F(0, 0), Vec2F(10, 20), Vec2F(5, 5), Vec2F(8, 18) };
    mesh.colors = { 0xFFFFFFFF, 0x80FF8040, 0xFF000000, 0xFFFFFFFF };
    mesh.indexes = { 0, 1, 2, 1, 2, 3 };

    Basis transform(Vec2F(100, 200), Vec2F(20, 0), Vec2F(0, 40));

    Vertex vertices[4];
    mesh.FillVertices(vertices, transform);
    EXPECT_EQ((Vec2F)vertices[0], Vec2F(100, 240));
    EXPECT_EQ((Vec2F)vertices[1], Vec2F(120, 200));
    EXPECT_EQ((Vec2F)vertices[2], Vec2F(110, 230));
    EXPECT_EQ(vertices[1].color, 0x80FF8040u);

    mesh.FillVertices(vertices, transform, Color4(255, 128, 0, 128));
    EXPECT_EQ(vertices[0].color, Color4(255, 128, 0, 128).ABGR());
    EXPECT_EQ(vertices[1].color, (Color4(0x40, 0x80, 0xFF, 0x80)*Color4(255, 128, 0, 128)).ABGR());

    mesh.FillSlicedVertices(vertices, Basis(Vec2F(), Vec2F(30, 0), Vec2F(0, 20)), BorderF(3, 0, 3, 0), Vec2F(30, 20));
    EXPECT_EQ((Vec2F)vertices[0], Vec2F(0, 20));
    EXPECT_EQ((Vec2F)vertices[1], Vec2F(30, 0));
    EXPECT_EQ((Vec2F)vertices[2], Vec2F(15, 15));
    EXPECT_EQ((Vec2F)vertices[3], Vec2F(28, 2));
}
