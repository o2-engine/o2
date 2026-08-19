#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Mesh.h"
#include "o2/Utils/Math/Vertex.h"

using namespace o2;

TEST(Mesh, DefaultConstructionAllocatesBuffersWithZeroCounts)
{
    Mesh m;
    EXPECT_EQ(m.GetMaxVertexCount(), 4u);
    EXPECT_EQ(m.GetMaxPolyCount(), 2u);
    EXPECT_EQ(m.vertexCount, 0u);
    EXPECT_EQ(m.polyCount, 0u);
    EXPECT_NE(m.GetVertexData(), nullptr);
    EXPECT_NE(m.GetIndexes(), nullptr);
}

TEST(Mesh, ResizeUpdatesMaxCountsAndZerosCurrentCounts)
{
    Mesh m;
    m.vertexCount = 3;
    m.polyCount = 1;

    m.Resize(10, 12);

    EXPECT_EQ(m.GetMaxVertexCount(), 10u);
    EXPECT_EQ(m.GetMaxPolyCount(), 12u);
    EXPECT_EQ(m.vertexCount, 0u);
    EXPECT_EQ(m.polyCount, 0u);
}

TEST(Mesh, ResizeAllocatesUsableBuffers)
{
    Mesh m;
    m.Resize(8, 4);

    Vertex* vertices = m.GetVertices<Vertex>();
    ASSERT_NE(vertices, nullptr);
    vertices[0].x = 1.0f;
    vertices[0].y = 2.0f;
    vertices[7].x = 9.0f;
    EXPECT_FLOAT_EQ(m.GetVertices<Vertex>()[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m.GetVertices<Vertex>()[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m.GetVertices<Vertex>()[7].x, 9.0f);

    VertexIndex* indexes = m.GetIndexes();
    ASSERT_NE(indexes, nullptr);
    indexes[0] = 5;
    indexes[11] = 7;
    EXPECT_EQ(m.GetIndexes()[0], 5u);
    EXPECT_EQ(m.GetIndexes()[11], 7u);
}

TEST(Mesh, SetTextureSrcRectRoundTrip)
{
    Mesh m;
    m.SetTextureSrcRect(RectI(10, 20, 30, 40));
    EXPECT_EQ(m.GetTextureSrcRect(), RectI(10, 20, 30, 40));
}

TEST(Mesh, CopyConstructorDeepCopiesBuffersAndMetadata)
{
    Mesh src;
    src.Resize(4, 2);
    src.SetTextureSrcRect(RectI(1, 2, 3, 4));
    src.vertexCount = 4;
    src.polyCount = 2;

    Vertex* srcVertices = src.GetVertices<Vertex>();
    for (UInt i = 0; i < 4; ++i)
    {
        srcVertices[i].x = (float)i;
        srcVertices[i].y = (float)(i * 10);
    }
    VertexIndex* srcIndexes = src.GetIndexes();
    for (UInt i = 0; i < 6; ++i)
        srcIndexes[i] = i;

    Mesh copy(src);

    EXPECT_EQ(copy.GetMaxVertexCount(), 4u);
    EXPECT_EQ(copy.GetMaxPolyCount(), 2u);
    EXPECT_EQ(copy.vertexCount, 4u);
    EXPECT_EQ(copy.polyCount, 2u);
    EXPECT_EQ(copy.GetTextureSrcRect(), RectI(1, 2, 3, 4));

    Vertex* copyVertices = copy.GetVertices<Vertex>();
    for (UInt i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(copyVertices[i].x, (float)i);
        EXPECT_FLOAT_EQ(copyVertices[i].y, (float)(i * 10));
    }
    for (UInt i = 0; i < 6; ++i)
        EXPECT_EQ(copy.GetIndexes()[i], i);

    EXPECT_NE(src.GetVertexData(), copy.GetVertexData());
    srcVertices[0].x = 999.0f;
    EXPECT_FLOAT_EQ(copy.GetVertices<Vertex>()[0].x, 0.0f);
}

TEST(Mesh, AssignmentOperatorDeepCopies)
{
    Mesh src;
    src.Resize(6, 3);
    src.SetTextureSrcRect(RectI(5, 6, 7, 8));
    src.vertexCount = 5;
    Vertex* srcVertices = src.GetVertices<Vertex>();
    srcVertices[0].x = 42.0f;

    Mesh dst;
    dst = src;

    EXPECT_EQ(dst.GetMaxVertexCount(), 6u);
    EXPECT_EQ(dst.GetMaxPolyCount(), 3u);
    EXPECT_EQ(dst.vertexCount, 5u);
    EXPECT_EQ(dst.GetTextureSrcRect(), RectI(5, 6, 7, 8));
    EXPECT_FLOAT_EQ(dst.GetVertices<Vertex>()[0].x, 42.0f);
    EXPECT_NE(dst.GetVertexData(), src.GetVertexData());

    srcVertices[0].x = -1.0f;
    EXPECT_FLOAT_EQ(dst.GetVertices<Vertex>()[0].x, 42.0f);
}

TEST(Mesh, SetMaxVertexCountResetsVertexCountAndPreservesPolyMax)
{
    Mesh m;
    m.Resize(4, 2);
    m.vertexCount = 4;
    m.polyCount = 2;

    m.SetMaxVertexCount(20);
    EXPECT_EQ(m.GetMaxVertexCount(), 20u);
    EXPECT_EQ(m.vertexCount, 0u);
    EXPECT_EQ(m.GetMaxPolyCount(), 2u);
    EXPECT_EQ(m.polyCount, 2u);
}

TEST(Mesh, SetMaxPolyCountResetsPolyCountAndPreservesVertexMax)
{
    Mesh m;
    m.Resize(4, 2);
    m.vertexCount = 4;
    m.polyCount = 2;

    m.SetMaxPolyCount(15);
    EXPECT_EQ(m.GetMaxPolyCount(), 15u);
    EXPECT_EQ(m.polyCount, 0u);
    EXPECT_EQ(m.GetMaxVertexCount(), 4u);
    EXPECT_EQ(m.vertexCount, 4u);
}

TEST(Mesh, ResizeReusesBuffersWhenNotGrowing)
{
    Mesh m;
    m.Resize(100, 50);

    const UInt8* vertexData = m.GetVertexData();
    const VertexIndex* indexData = m.GetIndexes();

    // Meshes refilled every frame at the same size must not hit the allocator
    m.Resize(24, 12);

    EXPECT_EQ(m.GetVertexData(), vertexData);
    EXPECT_EQ(m.GetIndexes(), indexData);
    EXPECT_GE(m.GetMaxVertexCount(), 24u);
    EXPECT_GE(m.GetMaxPolyCount(), 12u);
    EXPECT_EQ(m.vertexCount, 0u);
    EXPECT_EQ(m.polyCount, 0u);
}

TEST(Mesh, ResizeGrowsBuffersAndKeepsThemUsable)
{
    Mesh m;
    m.Resize(4, 2);
    m.Resize(64, 32);

    EXPECT_EQ(m.GetMaxVertexCount(), 64u);
    EXPECT_EQ(m.GetMaxPolyCount(), 32u);

    Vertex* vertices = m.GetVertices<Vertex>();
    ASSERT_NE(vertices, nullptr);
    vertices[63].x = 5.0f;
    EXPECT_FLOAT_EQ(m.GetVertices<Vertex>()[63].x, 5.0f);

    m.GetIndexes()[95] = 3;
    EXPECT_EQ(m.GetIndexes()[95], 3u);
}
