#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

TEST(Rect, ConstructorAutoNormalizes) {
    // Rect normalizes so left <= right and bottom <= top, regardless of input order.
    RectF r(100.0f, 50.0f, 10.0f, 200.0f);
    EXPECT_LE(r.left, r.right);
    EXPECT_LE(r.bottom, r.top);

    EXPECT_FLOAT_EQ(r.left, 10.0f);
    EXPECT_FLOAT_EQ(r.right, 100.0f);
    EXPECT_FLOAT_EQ(r.top, 200.0f);
    EXPECT_FLOAT_EQ(r.bottom, 50.0f);
}

TEST(Rect, WidthHeightAreNonNegativeAfterNormalize) {
    RectF r(100.0f, 50.0f, 10.0f, 200.0f);
    EXPECT_GT(r.Width(), 0.0f);
    EXPECT_GT(r.Height(), 0.0f);
    EXPECT_FLOAT_EQ(r.Width(), 90.0f);
    EXPECT_FLOAT_EQ(r.Height(), 150.0f);
}

TEST(Rect, SizePositionCenter) {
    RectF r(10.0f, 200.0f, 100.0f, 50.0f);
    EXPECT_EQ(r.Size(), Vec2F(90.0f, 150.0f));
    EXPECT_EQ(r.Position(), Vec2F(10.0f, 200.0f));
    EXPECT_EQ(r.Center(), Vec2F(55.0f, 125.0f));
}

TEST(Rect, IsInsideStrictlyExcludesBoundary) {
    RectF r(0.0f, 100.0f, 100.0f, 0.0f); // [0..100] x [0..100]

    EXPECT_TRUE(r.IsInside(Vec2F(50.0f, 50.0f)));
    EXPECT_FALSE(r.IsInside(Vec2F(0.0f, 50.0f)));    // on left edge
    EXPECT_FALSE(r.IsInside(Vec2F(100.0f, 50.0f)));  // on right edge
    EXPECT_FALSE(r.IsInside(Vec2F(50.0f, 0.0f)));    // on bottom edge
    EXPECT_FALSE(r.IsInside(Vec2F(50.0f, 100.0f)));  // on top edge
    EXPECT_FALSE(r.IsInside(Vec2F(-1.0f, 50.0f)));
    EXPECT_FALSE(r.IsInside(Vec2F(101.0f, 50.0f)));
}

TEST(Rect, IsIntersectsTouchingRectangles) {
    // IsIntersects uses non-strict comparison, so touching rects count as intersecting.
    RectF a(0.0f, 100.0f, 100.0f, 0.0f);
    RectF b(100.0f, 100.0f, 200.0f, 0.0f); // touches a on right edge
    EXPECT_TRUE(a.IsIntersects(b));

    RectF c(101.0f, 100.0f, 200.0f, 0.0f); // strictly to the right
    EXPECT_FALSE(a.IsIntersects(c));
}

TEST(Rect, GetIntersectionWithOverlap) {
    RectF a(0.0f, 100.0f, 100.0f, 0.0f);
    RectF b(50.0f, 80.0f, 200.0f, 20.0f);
    RectF inter = a.GetIntersection(b);

    EXPECT_FLOAT_EQ(inter.left, 50.0f);
    EXPECT_FLOAT_EQ(inter.right, 100.0f);
    EXPECT_FLOAT_EQ(inter.bottom, 20.0f);
    EXPECT_FLOAT_EQ(inter.top, 80.0f);
}

TEST(Rect, GetIntersectionDisjointReturnsZero) {
    RectF a(0.0f, 10.0f, 10.0f, 0.0f);
    RectF b(50.0f, 60.0f, 60.0f, 50.0f);
    RectF inter = a.GetIntersection(b);
    EXPECT_TRUE(inter.IsZero());
}

TEST(Rect, ExpandUnionContainsBoth) {
    RectF a(0.0f, 100.0f, 50.0f, 0.0f);
    RectF b(40.0f, 200.0f, 200.0f, 80.0f);
    RectF u = a.Expand(b);

    EXPECT_FLOAT_EQ(u.left, 0.0f);
    EXPECT_FLOAT_EQ(u.right, 200.0f);
    EXPECT_FLOAT_EQ(u.top, 200.0f);
    EXPECT_FLOAT_EQ(u.bottom, 0.0f);
}

TEST(Rect, MoveByVectorTranslatesAllSides) {
    RectF r(10.0f, 100.0f, 50.0f, 20.0f);
    RectF moved = r + Vec2F(5.0f, -10.0f);

    EXPECT_FLOAT_EQ(moved.left, 15.0f);
    EXPECT_FLOAT_EQ(moved.right, 55.0f);
    EXPECT_FLOAT_EQ(moved.top, 90.0f);
    EXPECT_FLOAT_EQ(moved.bottom, 10.0f);

    EXPECT_FLOAT_EQ(moved.Width(), r.Width());
    EXPECT_FLOAT_EQ(moved.Height(), r.Height());
}

TEST(Rect, ScaleAroundOriginPreservesOrigin) {
    RectF r(0.0f, 100.0f, 100.0f, 0.0f);
    Vec2F origin(50.0f, 50.0f);

    RectF scaled = r.Scale(Vec2F(2.0f, 2.0f), origin);

    // Origin point must stay inside the scaled rect at the same relative position.
    // The rect doubles in size but stays centered at origin.
    EXPECT_FLOAT_EQ(scaled.Center().x, origin.x);
    EXPECT_FLOAT_EQ(scaled.Center().y, origin.y);
    EXPECT_FLOAT_EQ(scaled.Width(), r.Width() * 2.0f);
    EXPECT_FLOAT_EQ(scaled.Height(), r.Height() * 2.0f);
}

TEST(Rect, BoundFromPointsCoversAllPoints) {
    Vec2F points[] = {
        Vec2F(10.0f, 20.0f),
        Vec2F(-5.0f, 100.0f),
        Vec2F(30.0f, 5.0f),
        Vec2F(0.0f, 0.0f),
    };

    RectF b = RectF::Bound(points, 4);
    EXPECT_FLOAT_EQ(b.left, -5.0f);
    EXPECT_FLOAT_EQ(b.right, 30.0f);
    EXPECT_FLOAT_EQ(b.top, 100.0f);
    EXPECT_FLOAT_EQ(b.bottom, 0.0f);

    for (auto& p : points)
        EXPECT_TRUE(b.left <= p.x && p.x <= b.right && b.bottom <= p.y && p.y <= b.top);
}

TEST(Rect, Equality) {
    RectF a(0.0f, 100.0f, 50.0f, 20.0f);
    RectF b(0.0f, 100.0f, 50.0f, 20.0f);
    RectF c(0.0f, 100.0f, 51.0f, 20.0f);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

TEST(Rect, IsZeroDetectsAllZeros) {
    RectF z(0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(z.IsZero());

    RectF nonZero(0.0f, 1.0f, 0.0f, 0.0f);
    EXPECT_FALSE(nonZero.IsZero());
}

TEST(Rect, SetPositionPreservesSize) {
    RectF r(10.0f, 100.0f, 50.0f, 20.0f);
    Vec2F sizeBefore = r.Size();

    r.SetPosition(Vec2F(0.0f, 0.0f));

    EXPECT_EQ(r.Size(), sizeBefore);
    EXPECT_EQ(r.Position(), Vec2F(0.0f, 0.0f));
}
