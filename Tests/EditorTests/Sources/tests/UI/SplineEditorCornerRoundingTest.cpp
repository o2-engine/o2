#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/UI/SplineEditor/SplineEditor.h"

using namespace o2;
using namespace Editor;

// Pure math of the corner rounding handles: virtual corner reconstruction and
// rounded corner calculation, on a right-angle corner at (0, 0) with neighbors
// at (-100, 0) and (0, 100)
namespace
{
    const Vec2F kPrev(-100.0f, 0.0f);
    const Vec2F kNext(0.0f, 100.0f);
    const Vec2F kCorner(0.0f, 0.0f);
    const Vec2F kDirPrev(-1.0f, 0.0f);
    const Vec2F kDirNext(0.0f, 1.0f);

    bool NearVec(const Vec2F& a, const Vec2F& b, float eps = 0.01f)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps;
    }
}

TEST(SplineEditorCornerRounding, ZeroSupportsCornerIsThePoint)
{
    Vec2F corner;
    float value = -1.0f;
    bool ok = SplineEditor::ReconstructCorner(kPrev, kNext, kCorner, kCorner, kCorner, corner, value);

    ASSERT_TRUE(ok);
    EXPECT_TRUE(NearVec(corner, kCorner));
    EXPECT_FLOAT_EQ(value, 0.0f);
}

TEST(SplineEditorCornerRounding, RoundedCornerLiesOnBisectorWithTangentSupports)
{
    Vec2F pos, prevSupport, nextSupport;
    SplineEditor::CalcRoundedCorner(kCorner, kDirPrev, kDirNext, 10.0f, pos, prevSupport, nextSupport);

    // Point moves along the bisector into the corner
    float invSqrt2 = 1.0f/Math::Sqrt(2.0f);
    EXPECT_TRUE(NearVec(pos, Vec2F(-invSqrt2, invSqrt2)*10.0f));

    // Supports are symmetric and land exactly on the corner edges
    EXPECT_NEAR(prevSupport.y, 0.0f, 0.01f) << "prev support lies on the horizontal edge";
    EXPECT_NEAR(nextSupport.x, 0.0f, 0.01f) << "next support lies on the vertical edge";
    EXPECT_NEAR((prevSupport - pos).Length(), (nextSupport - pos).Length(), 0.01f);

    // Curve tangent at the point is perpendicular to the bisector
    Vec2F tangent = (nextSupport - prevSupport).Normalized();
    Vec2F bisector = Vec2F(-invSqrt2, invSqrt2);
    EXPECT_NEAR(Math::Abs(tangent.Dot(bisector)), 0.0f, 0.001f);
}

TEST(SplineEditorCornerRounding, ReconstructRecoversRoundedCorner)
{
    Vec2F pos, prevSupport, nextSupport;
    SplineEditor::CalcRoundedCorner(kCorner, kDirPrev, kDirNext, 25.0f, pos, prevSupport, nextSupport);

    Vec2F corner;
    float value = 0.0f;
    bool ok = SplineEditor::ReconstructCorner(kPrev, kNext, pos, prevSupport, nextSupport, corner, value);

    ASSERT_TRUE(ok);
    EXPECT_TRUE(NearVec(corner, kCorner, 0.1f));
    EXPECT_NEAR(value, 25.0f, 0.1f);
}

TEST(SplineEditorCornerRounding, ZeroValueRestoresSharpCorner)
{
    Vec2F pos, prevSupport, nextSupport;
    SplineEditor::CalcRoundedCorner(kCorner, kDirPrev, kDirNext, 0.0f, pos, prevSupport, nextSupport);

    EXPECT_TRUE(NearVec(pos, kCorner));
    EXPECT_TRUE(NearVec(prevSupport, kCorner));
    EXPECT_TRUE(NearVec(nextSupport, kCorner));
}

TEST(SplineEditorCornerRounding, CollinearSupportsAreInvalid)
{
    // Both edge lines coincide with the X axis — the corner is undefined
    Vec2F corner;
    float value = 0.0f;
    bool ok = SplineEditor::ReconstructCorner(Vec2F(-100.0f, 0.0f), Vec2F(100.0f, 0.0f), Vec2F(0.0f, 0.0f),
                                              Vec2F(-10.0f, 0.0f), Vec2F(10.0f, 0.0f), corner, value);

    EXPECT_FALSE(ok);
}

TEST(SplineEditorCornerRounding, WideAngleCornerKeepsSupportsOnEdges)
{
    // 120-degree corner: neighbors at (-100, 0) and (50, 86.6)
    Vec2F next = Vec2F(Math::Cos(Math::Deg2rad(60.0f)), Math::Sin(Math::Deg2rad(60.0f)))*100.0f;
    Vec2F dirNext = next.Normalized();

    Vec2F pos, prevSupport, nextSupport;
    SplineEditor::CalcRoundedCorner(kCorner, kDirPrev, dirNext, 15.0f, pos, prevSupport, nextSupport);

    // Supports stay on the edge lines: prev on Y=0, next on the 60-degree line
    EXPECT_NEAR(prevSupport.y, 0.0f, 0.01f);
    float cross = nextSupport.x*dirNext.y - nextSupport.y*dirNext.x;
    EXPECT_NEAR(cross, 0.0f, 0.01f);
}
