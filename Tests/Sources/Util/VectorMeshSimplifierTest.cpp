#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/VectorGraphics/VectorMeshSimplifier.h"
#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

// Removing the mesh vertices that the triangles around them draw the same without

namespace
{
    const float tolerance = VectorTessellationParams().simplification;

    Color32Bit Gray(int level, int alpha = 255)
    {
        return Color4(level, level, level, alpha).ABGR();
    }

    // Grid of cells x cells squares at the origin, two triangles each; vertices are not shared with other grids
    void AddGrid(VectorMesh& mesh, const Vec2F& origin, int cells, const Function<Color32Bit(int, int)>& colorAt)
    {
        VertexIndex first = (VertexIndex)mesh.positions.Count();
        for (int y = 0; y <= cells; y++)
        {
            for (int x = 0; x <= cells; x++)
            {
                mesh.positions.Add(origin + Vec2F((float)x, (float)y));
                mesh.colors.Add(colorAt(x, y));
            }
        }

        for (int y = 0; y < cells; y++)
        {
            for (int x = 0; x < cells; x++)
            {
                VertexIndex corner = first + (VertexIndex)(y*(cells + 1) + x);
                for (VertexIndex index : { corner, corner + 1, corner + (VertexIndex)cells + 2,
                                           corner, corner + (VertexIndex)cells + 2, corner + (VertexIndex)cells + 1 })
                {
                    mesh.indexes.Add(index);
                }
            }
        }
    }

    VectorMesh MakeMesh(float size)
    {
        VectorMesh mesh;
        mesh.size = Vec2F(size, size);
        return mesh;
    }

    double Area(const VectorMesh& mesh)
    {
        double area = 0;
        for (int i = 0; i + 2 < mesh.indexes.Count(); i += 3)
        {
            Vec2F a = mesh.positions[mesh.indexes[i]], b = mesh.positions[mesh.indexes[i + 1]],
                c = mesh.positions[mesh.indexes[i + 2]];
            area += Math::Abs((double)(b.x - a.x)*(c.y - a.y) - (double)(b.y - a.y)*(c.x - a.x))*0.5;
        }

        return area;
    }

    int MaxDifference(const VectorMesh& a, const VectorMesh& b)
    {
        Color4 background(96, 96, 96, 255);
        auto bitmapA = VectorRasterizer::Rasterize(a, 1.0f, background);
        auto bitmapB = VectorRasterizer::Rasterize(b, 1.0f, background);

        BitmapCompareResult result = BitmapCompare::Compare(*bitmapA, *bitmapB, 0, background);
        EXPECT_TRUE(result.comparable);
        return result.maxDifference;
    }

    VectorMesh Tessellate(const String& svg, float simplification, float scale = 1.0f)
    {
        VectorTessellationParams params;
        params.pixelScale = Vec2F(scale, scale);
        params.simplification = simplification;

        VectorMesh mesh;
        VectorTessellator::Tessellate(Parse(svg), mesh, params);
        return mesh;
    }

    const char* panelBody =
        "<defs><radialGradient id=\"g\" gradientUnits=\"userSpaceOnUse\" cx=\"15\" cy=\"15\" r=\"15\">"
        "<stop offset=\"0\" stop-color=\"#000000\" stop-opacity=\"0.15\"/>"
        "<stop offset=\"0.5\" stop-color=\"#000000\" stop-opacity=\"0.13\"/>"
        "<stop offset=\"1\" stop-color=\"#000000\" stop-opacity=\"0\"/></radialGradient></defs>"
        "<rect x=\"0\" y=\"0\" width=\"15\" height=\"15\" fill=\"url(#g)\"/>"
        "<path d=\"M13.8 24.5 C9.5 23.8 6.2 20.5 5.5 16.2 L5.5 12.8 C6.2 8.5 9.5 5.2 13.8 4.5 L26.2 4.5 "
        "C30.5 5.2 33.8 8.5 34.5 12.8 L34.5 16.2 C33.8 20.5 30.5 23.8 26.2 24.5 Z\" fill=\"#009688\"/>";
}

TEST(VectorMeshSimplifier, GridOfOneColorBecomesTwoTriangles)
{
    VectorMesh mesh = MakeMesh(8);
    AddGrid(mesh, Vec2F(), 8, [](int, int) { return Gray(200); });

    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_EQ(mesh.GetTrianglesCount(), 2u);
    EXPECT_EQ(mesh.positions.Count(), 4);
    EXPECT_EQ(mesh.colors.Count(), 4);
    EXPECT_NEAR(Area(mesh), 64.0, 1e-6);
}

TEST(VectorMeshSimplifier, GridOfLinearGradientBecomesTwoTriangles)
{
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(), 8, [](int x, int y) { return Gray(40 + x*20 + y*5, 255 - x*10); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_EQ(mesh.GetTrianglesCount(), 2u);
    EXPECT_LE(MaxDifference(source, mesh), 1);
}

TEST(VectorMeshSimplifier, GradientRoundedToLevelsIsWithinTolerance)
{
    VectorMesh source = MakeMesh(12);
    AddGrid(source, Vec2F(), 12, [](int x, int y) { return Gray(Math::RoundToInt(30.0f + (float)x*3.3f + (float)y*0.7f)); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_LE(mesh.GetTrianglesCount(), 8u);
    EXPECT_LE(MaxDifference(source, mesh), 1);
}

TEST(VectorMeshSimplifier, VertexOffTheColorPlaneStays)
{
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(), 8, [](int x, int y) { return Gray(x == 4 && y == 4 ? 100 : 200); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_LT(mesh.GetTrianglesCount(), source.GetTrianglesCount());
    EXPECT_GT(mesh.GetTrianglesCount(), 2u);
    EXPECT_EQ(MaxDifference(source, mesh), 0);

    bool found = false;
    for (int i = 0; i < mesh.positions.Count(); i++)
        found = found || (mesh.positions[i] == Vec2F(4, 4) && mesh.colors[i] == Gray(100));

    EXPECT_TRUE(found);
}

TEST(VectorMeshSimplifier, ZeroToleranceKeepsColorsExactly)
{
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(), 8, [](int x, int y) { return Gray(100 + ((x*7 + y*13)%3)); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, 0.0f);

    EXPECT_EQ(MaxDifference(source, mesh), 0);
}

TEST(VectorMeshSimplifier, OutlineKeepsItsCorners)
{
    // L shape: three grids of one color welded by position
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(0, 0), 4, [](int, int) { return Gray(255); });
    AddGrid(source, Vec2F(4, 0), 4, [](int, int) { return Gray(255); });
    AddGrid(source, Vec2F(0, 4), 4, [](int, int) { return Gray(255); });

    // Grids share no vertices: weld them
    VectorMesh welded = MakeMesh(8);
    for (VertexIndex index : source.indexes)
    {
        int found = welded.positions.IndexOf(source.positions[index]);
        if (found < 0)
        {
            found = welded.positions.Count();
            welded.positions.Add(source.positions[index]);
            welded.colors.Add(source.colors[index]);
        }

        welded.indexes.Add((VertexIndex)found);
    }

    VectorMesh mesh = welded;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_EQ(mesh.positions.Count(), 6);
    EXPECT_EQ(mesh.GetTrianglesCount(), 4u);
    EXPECT_NEAR(Area(mesh), 48.0, 1e-6);
    EXPECT_EQ(MaxDifference(welded, mesh), 0);
}

TEST(VectorMeshSimplifier, SeamOfTwoColorsStaysClosed)
{
    // Two grids touch along x = 4 with vertices of different colors at the same points
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(0, 0), 4, [](int, int) { return Gray(255); });
    AddGrid(source, Vec2F(4, 0), 4, [](int, int) { return Gray(60); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0 }, tolerance);

    EXPECT_LT(mesh.GetTrianglesCount(), source.GetTrianglesCount());

    int onSeam = 0;
    for (const Vec2F& position : mesh.positions)
        onSeam += position.x == 4.0f ? 1 : 0;

    EXPECT_EQ(onSeam, 10);
    EXPECT_EQ(MaxDifference(source, mesh), 0);
}

TEST(VectorMeshSimplifier, LayersKeepTheirOrder)
{
    VectorMesh source = MakeMesh(8);
    AddGrid(source, Vec2F(0, 0), 8, [](int, int) { return Gray(255); });

    int secondLayer = source.indexes.Count();
    AddGrid(source, Vec2F(2, 2), 4, [](int, int) { return Gray(0, 128); });

    VectorMesh mesh = source;
    VectorMeshSimplifier::Simplify(mesh, { 0, secondLayer }, tolerance);

    ASSERT_EQ(mesh.GetTrianglesCount(), 4u);
    for (int i = 0; i < 6; i++)
        EXPECT_EQ(mesh.colors[mesh.indexes[i]], Gray(255));

    for (int i = 6; i < 12; i++)
        EXPECT_EQ(mesh.colors[mesh.indexes[i]], Gray(0, 128));

    EXPECT_EQ(MaxDifference(source, mesh), 0);
}

TEST(VectorMeshSimplifier, TrianglesBeforeTheFirstLayerStay)
{
    VectorMesh mesh = MakeMesh(8);
    AddGrid(mesh, Vec2F(0, 0), 2, [](int, int) { return Gray(255); });

    int layer = mesh.indexes.Count();
    AddGrid(mesh, Vec2F(4, 4), 2, [](int, int) { return Gray(255); });

    VectorMeshSimplifier::Simplify(mesh, { layer }, tolerance);

    EXPECT_EQ(mesh.GetTrianglesCount(), 8u + 2u);
}

TEST(VectorMeshSimplifier, UnusedVerticesAreRemoved)
{
    VectorMesh mesh = MakeMesh(4);
    mesh.positions = { Vec2F(9, 9), Vec2F(0, 0), Vec2F(4, 0), Vec2F(8, 8), Vec2F(0, 4) };
    mesh.colors = { Gray(1), Gray(2), Gray(3), Gray(4), Gray(5) };
    mesh.indexes = { 1, 2, 4 };

    VectorMeshSimplifier::RemoveUnusedVertices(mesh);

    ASSERT_EQ(mesh.positions.Count(), 3);
    EXPECT_EQ(mesh.positions[mesh.indexes[0]], Vec2F(0, 0));
    EXPECT_EQ(mesh.positions[mesh.indexes[1]], Vec2F(4, 0));
    EXPECT_EQ(mesh.positions[mesh.indexes[2]], Vec2F(0, 4));
    EXPECT_EQ(mesh.colors[mesh.indexes[2]], Gray(5));
}

TEST(VectorMeshSimplifier, TessellatedImageDrawsTheSame)
{
    for (float scale : { 1.0f, 2.0f })
    {
        VectorMesh full = Tessellate(Svg(40, 30, panelBody), 0.0f, scale);
        VectorMesh simplified = Tessellate(Svg(40, 30, panelBody), tolerance, scale);

        EXPECT_LT((float)simplified.GetTrianglesCount(), (float)full.GetTrianglesCount()*0.9f) << scale;
        EXPECT_LT(simplified.positions.Count(), full.positions.Count()) << scale;

        Color4 background(96, 96, 96, 255);
        auto fullBitmap = VectorRasterizer::Rasterize(full, scale, background);
        auto simplifiedBitmap = VectorRasterizer::Rasterize(simplified, scale, background);

        BitmapCompareResult result = BitmapCompare::Compare(*fullBitmap, *simplifiedBitmap, 0, background);
        ASSERT_TRUE(result.comparable);
        EXPECT_LE(result.maxDifference, 2) << scale;
    }
}

TEST(VectorMeshSimplifier, MeshesOfUiShapesStayWithinTrianglesCount)
{
    struct Shape
    {
        const char* name;
        String      svg;
        UInt        maxTriangles;
    };

    Vector<Shape> shapes =
    {
        { "pixel aligned rect", Svg(16, 16, "<rect x=\"2\" y=\"3\" width=\"10\" height=\"9\" fill=\"#3366ff\"/>"), 2 },
        { "linear gradient rect", Svg(16, 16,
            "<defs><linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"16\" gradientUnits=\"userSpaceOnUse\">"
            "<stop offset=\"0\" stop-color=\"#ffffff\"/><stop offset=\"1\" stop-color=\"#404040\"/></linearGradient></defs>"
            "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"url(#g)\"/>"), 2 },
        { "circle", Svg(20, 20, "<circle cx=\"10\" cy=\"10\" r=\"7.3\" fill=\"#ffffff\"/>"), 240 },
        { "panel with radial gradient", Svg(40, 30, panelBody), 340 },
    };

    for (const Shape& shape : shapes)
    {
        VectorMesh mesh = Tessellate(shape.svg, tolerance);
        EXPECT_LE(mesh.GetTrianglesCount(), shape.maxTriangles) << shape.name;
        EXPECT_GT(mesh.GetTrianglesCount(), 0u) << shape.name;
    }
}
