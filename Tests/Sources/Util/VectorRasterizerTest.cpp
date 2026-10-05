#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "VectorGraphicsTestHelpers.h"

using namespace o2;
using namespace VectorGraphicsTest;

namespace
{
    VectorMesh MeshOf(const Vector<Vec2F>& positions, const Vector<Color4>& colors, const Vec2F& size = Vec2F(8, 8))
    {
        VectorMesh mesh;
        mesh.size = size;
        mesh.positions = positions;
        for (size_t i = 0; i < positions.size(); i++)
        {
            mesh.colors.Add(colors[i%colors.size()].ABGR());
            mesh.indexes.Add((VertexIndex)i);
        }

        return mesh;
    }

    VectorMesh Quad(float left, float top, float right, float bottom, const Color4& color)
    {
        return MeshOf({ Vec2F(left, top), Vec2F(right, top), Vec2F(right, bottom),
                        Vec2F(left, top), Vec2F(right, bottom), Vec2F(left, bottom) }, { color });
    }
}

TEST(VectorRasterizer, SamplesAtPixelCenters)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(1.4f, 1.4f, 3.6f, 3.6f, Color4(255, 0, 0, 255)));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 9);
    EXPECT_EQ(Alpha(bitmap, 1, 1), 255);
    EXPECT_EQ(Alpha(bitmap, 3, 3), 255);

    bitmap = VectorRasterizer::Rasterize(Quad(1.6f, 1.6f, 3.4f, 3.4f, Color4(255, 0, 0, 255)));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 1);
    EXPECT_EQ(Alpha(bitmap, 2, 2), 255);
}

TEST(VectorRasterizer, TopLeftRule)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(1.5f, 1.5f, 3.5f, 3.5f, Color4(255, 0, 0, 255)));
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 4);
    EXPECT_EQ(Alpha(bitmap, 1, 1), 255);
    EXPECT_EQ(Alpha(bitmap, 2, 2), 255);
    EXPECT_EQ(Alpha(bitmap, 3, 1), 0);
    EXPECT_EQ(Alpha(bitmap, 1, 3), 0);
}

TEST(VectorRasterizer, SharedEdgeIsCoveredOnce)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(0.5f, 0.5f, 6.5f, 6.5f, Color4(0, 0, 0, 128)));
    EXPECT_EQ(CountAlpha(bitmap, 128, 128), 36);
    EXPECT_EQ(CountAlpha(bitmap, 129, 255), 0);

    VectorMesh fan = MeshOf({ Vec2F(4, 4), Vec2F(0, 0), Vec2F(8, 0), Vec2F(4, 4), Vec2F(8, 0), Vec2F(8, 8),
                              Vec2F(4, 4), Vec2F(8, 8), Vec2F(0, 8), Vec2F(4, 4), Vec2F(0, 8), Vec2F(0, 0) },
                            { Color4(0, 0, 0, 128) });
    bitmap = VectorRasterizer::Rasterize(fan);
    EXPECT_EQ(CountAlpha(bitmap, 128, 128), 64);

    VectorMesh reversed = MeshOf({ Vec2F(0, 0), Vec2F(8, 8), Vec2F(8, 0), Vec2F(0, 0), Vec2F(0, 8), Vec2F(8, 8) },
                                 { Color4(0, 0, 0, 128) });
    bitmap = VectorRasterizer::Rasterize(reversed);
    EXPECT_EQ(CountAlpha(bitmap, 128, 128), 64);
}

TEST(VectorRasterizer, InterpolatesVertexColors)
{
    VectorMesh mesh = MeshOf({ Vec2F(0, 0), Vec2F(8, 0), Vec2F(0, 8) },
                             { Color4(255, 0, 0, 255), Color4(0, 255, 0, 255), Color4(0, 0, 255, 255) });
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh, 1.0f, Color4(0, 0, 0, 255));

    Color4 pixel = VectorRasterizer::GetPixel(*bitmap, 2, 2);
    EXPECT_NEAR(pixel.r, 255*3/8, 1);
    EXPECT_NEAR(pixel.g, 255*2.5f/8, 1);
    EXPECT_NEAR(pixel.b, 255*2.5f/8, 1);

    pixel = VectorRasterizer::GetPixel(*bitmap, 0, 0);
    EXPECT_NEAR(pixel.r, 255*7/8, 1);
}

TEST(VectorRasterizer, BlendsOverOpaqueBackground)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(0, 0, 8, 8, Color4(255, 0, 0, 128)), 1.0f,
                                                     Color4(255, 255, 255, 255));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 3, 3), Color4(255, 127, 127, 255));
}

TEST(VectorRasterizer, TransparentTargetGetsPremultipliedColor)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(0, 0, 8, 8, Color4(200, 100, 50, 128)));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 3, 3), Color4(100, 50, 25, 128));

    VectorRasterizer::Unpremultiply(*bitmap);
    Color4 pixel = VectorRasterizer::GetPixel(*bitmap, 3, 3);
    EXPECT_NEAR(pixel.r, 200, 1);
    EXPECT_NEAR(pixel.g, 100, 1);
    EXPECT_NEAR(pixel.b, 50, 1);
    EXPECT_EQ(pixel.a, 128);
}

TEST(VectorRasterizer, BlendsTrianglesInDrawingOrder)
{
    VectorMesh mesh = Quad(0, 0, 8, 8, Color4(255, 0, 0, 255));
    VectorMesh over = Quad(2, 2, 6, 6, Color4(0, 0, 255, 128));
    for (size_t i = 0; i < over.positions.size(); i++)
    {
        mesh.positions.Add(over.positions[i]);
        mesh.colors.Add(over.colors[i]);
        mesh.indexes.Add((VertexIndex)(6 + i));
    }

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 0, 0), Color4(255, 0, 0, 255));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 4, 4), Color4(127, 0, 128, 255));

    VectorMesh twice = Quad(0, 0, 8, 8, Color4(0, 0, 0, 128));
    for (int i = 0; i < 6; i++)
        twice.indexes.Add((VertexIndex)i);

    EXPECT_EQ(Alpha(VectorRasterizer::Rasterize(twice), 4, 4), 192);
}

TEST(VectorRasterizer, ScaleAndOffset)
{
    VectorMesh mesh = Quad(1, 1, 3, 3, Color4(0, 0, 0, 255));

    Ref<Bitmap> scaled = VectorRasterizer::Rasterize(mesh, 3.0f);
    EXPECT_EQ(scaled->GetSize(), Vec2I(24, 24));
    EXPECT_EQ(CountAlpha(scaled, 255, 255), 36);
    EXPECT_EQ(Alpha(scaled, 3, 3), 255);
    EXPECT_EQ(Alpha(scaled, 2, 3), 0);

    Bitmap target(PixelFormat::R8G8B8A8, Vec2I(8, 8));
    memset(target.GetData(), 0, 8*8*4);
    VectorRasterizer::Rasterize(mesh, target, 1.0f, Vec2F(4, 2));
    EXPECT_EQ(VectorRasterizer::GetPixel(target, 5, 3).a, 255);
    EXPECT_EQ(VectorRasterizer::GetPixel(target, 6, 4).a, 255);
    EXPECT_EQ(VectorRasterizer::GetPixel(target, 1, 1).a, 0);
}

TEST(VectorRasterizer, ImageTopIsTheLastBitmapRow)
{
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(Quad(0, 0, 8, 1, Color4(10, 20, 30, 255)));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 0, 0), Color4(10, 20, 30, 255));
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 0, 1).a, 0);

    const UInt8* lastRow = bitmap->GetData() + 7*8*4;
    EXPECT_EQ(lastRow[0], 10);
    EXPECT_EQ(lastRow[3], 255);
    EXPECT_EQ(bitmap->GetData()[3], 0);

    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, -1, 0).a, 0);
    EXPECT_EQ(VectorRasterizer::GetPixel(*bitmap, 0, 8).a, 0);
}

TEST(VectorRasterizer, ClipsToBitmapAndSkipsDegenerateTriangles)
{
    VectorMesh mesh = MeshOf({ Vec2F(-50, -50), Vec2F(100, -50), Vec2F(-50, 100), Vec2F(1, 1), Vec2F(5, 5), Vec2F(3, 3) },
                             { Color4(0, 0, 0, 255) });
    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh);
    EXPECT_EQ(CountAlpha(bitmap, 255, 255), 64);

    EXPECT_EQ(VectorRasterizer::Rasterize(VectorMesh())->GetSize(), Vec2I(1, 1));
}
