#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <cmath>
#include <cstring>

#include "o2/Utils/Math/Vertex.h"

using namespace o2;

// Mesh vertex buffers are raw uninitialized heap bytes, so Set() must write every field:
// a skipped z or normal keeps garbage, and a NaN z makes the GPU drop the polygon
namespace
{
    Vertex* PoisonedVertex(unsigned char* raw)
    {
        memset(raw, 0xFF, sizeof(Vertex)); // 0xFFFFFFFF is a float NaN pattern
        return reinterpret_cast<Vertex*>(raw);
    }

    void ExpectFullyInitialized(const Vertex& v, float expectedZ, const Vec2F& expectedNormal)
    {
        EXPECT_EQ(v.z, expectedZ);
        EXPECT_EQ(v.nx, expectedNormal.x);
        EXPECT_EQ(v.ny, expectedNormal.y);
        EXPECT_EQ(v.nz, 0.0f);
        EXPECT_FALSE(std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.tu) || std::isnan(v.tv));
    }
}

TEST(VertexSetInit, PositionColorUV)
{
    alignas(Vertex) unsigned char raw[sizeof(Vertex)];
    Vertex* v = PoisonedVertex(raw);
    v->Set(Vec2F(1.0f, 2.0f), 0xFFFFFFFF, 0.25f, 0.75f);
    ExpectFullyInitialized(*v, 1.0f, Vec2F(1.0f, 0.0f));
}

TEST(VertexSetInit, PositionColorUVNormal)
{
    alignas(Vertex) unsigned char raw[sizeof(Vertex)];
    Vertex* v = PoisonedVertex(raw);
    v->Set(Vec2F(1.0f, 2.0f), 0xFFFFFFFF, 0.25f, 0.75f, Vec2F(0.0f, 1.0f));
    ExpectFullyInitialized(*v, 1.0f, Vec2F(0.0f, 1.0f));
}

TEST(VertexSetInit, PlainFloatsColorUV)
{
    alignas(Vertex) unsigned char raw[sizeof(Vertex)];
    Vertex* v = PoisonedVertex(raw);
    v->Set(1.0f, 2.0f, 0xFFFFFFFF, 0.25f, 0.75f);
    ExpectFullyInitialized(*v, 1.0f, Vec2F(1.0f, 0.0f));
}

TEST(VertexSetInit, PositionZColorUV)
{
    alignas(Vertex) unsigned char raw[sizeof(Vertex)];
    Vertex* v = PoisonedVertex(raw);
    v->Set(Vec2F(1.0f, 2.0f), 5.0f, 0xFFFFFFFF, 0.25f, 0.75f);
    ExpectFullyInitialized(*v, 5.0f, Vec2F(1.0f, 0.0f));
}
