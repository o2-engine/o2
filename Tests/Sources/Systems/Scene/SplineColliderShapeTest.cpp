#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Box2D/Collision/Shapes/b2ChainShape.h"
#include "o2/Scene/Physics/SplineCollider.h"

using namespace o2;

namespace
{
    struct SplineColliderProbe: SplineCollider
    {
        b2Shape* Shape(const Basis& basis) { return GetShape(basis); }
    };
}

namespace
{
    float MinDistanceToVertex(b2ChainShape* shape, const Vec2F& point)
    {
        float minDist = 1e10f;
        for (int i = 0; i < shape->m_count; i++)
        {
            Vec2F v(shape->m_vertices[i].x, shape->m_vertices[i].y);
            minDist = Math::Min(minDist, (v - point).Length());
        }
        return minDist;
    }
}

// The loop chain must sample the closing bezier segment (last key -> first key)
// instead of replacing it with a straight chord
TEST(SplineColliderShape, LoopIncludesClosingSegmentCurve)
{
    // Triangle (-100,0), (100,0), (0,100): every segment yields ~20 approximation
    // points; before the fix the closing segment contributed nothing (~39 verts)
    auto sharp = mmake<SplineColliderProbe>();
    sharp->spline->AppendKey(Vec2F(0.0f, 100.0f), 0.0f, Vec2F(), Vec2F());
    sharp->SetIsLoop(true);

    auto sharpShape = dynamic_cast<b2ChainShape*>(sharp->Shape(Basis::Identity()));
    ASSERT_NE(sharpShape, nullptr);
    EXPECT_GE(sharpShape->m_count, 55);
    EXPECT_LT(MinDistanceToVertex(sharpShape, Vec2F(-50.0f, 50.0f)), 6.0f)
        << "closing chord midpoint must be sampled";

    // Curve the closing segment (key2 -> key0) with key0's prev support pulling
    // far left: the curve must leave the triangle bounds (all base verts x >= -100)
    auto rounded = mmake<SplineColliderProbe>();
    rounded->spline->AppendKey(Vec2F(0.0f, 100.0f), 0.0f, Vec2F(), Vec2F());
    rounded->SetIsLoop(true);

    auto key = rounded->spline->GetKey(0);
    key.prevSupport = Vec2F(-60.0f, 20.0f);
    rounded->spline->SetKey(key, 0);

    auto roundedShape = dynamic_cast<b2ChainShape*>(rounded->Shape(Basis::Identity()));
    ASSERT_NE(roundedShape, nullptr);

    float minX = 0.0f;
    for (int i = 0; i < roundedShape->m_count; i++)
        minX = Math::Min(minX, roundedShape->m_vertices[i].x);

    EXPECT_LT(minX, -102.0f) << "closing segment must follow the bezier curve, not the chord";
}

TEST(SplineColliderShape, OpenChainUnaffectedByClosingSegment)
{
    auto open = mmake<SplineColliderProbe>();
    open->spline->AppendKey(Vec2F(0.0f, 100.0f), 0.0f, Vec2F(), Vec2F());

    auto key = open->spline->GetKey(0);
    key.prevSupport = Vec2F(-20.0f, -40.0f);
    key.nextSupport = Vec2F(40.0f, 0.0f);
    open->spline->SetKey(key, 0);

    auto shape = dynamic_cast<b2ChainShape*>(open->Shape(Basis::Identity()));
    ASSERT_NE(shape, nullptr);

    // Two segments, 20 approximation points each, shared middle vertex
    EXPECT_LE(shape->m_count, 39);
}
