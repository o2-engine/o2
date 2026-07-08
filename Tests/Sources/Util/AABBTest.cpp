#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Basis3D.h"

using namespace o2;

static void ExpectVecNear(const Vec3F& a, const Vec3F& b, float tolerance = 1e-4f)
{
    EXPECT_NEAR(a.x, b.x, tolerance);
    EXPECT_NEAR(a.y, b.y, tolerance);
    EXPECT_NEAR(a.z, b.z, tolerance);
}

static void ExpectRectNear(const RectF& a, const RectF& b, float tolerance = 1e-3f)
{
    EXPECT_NEAR(a.left, b.left, tolerance);
    EXPECT_NEAR(a.right, b.right, tolerance);
    EXPECT_NEAR(a.bottom, b.bottom, tolerance);
    EXPECT_NEAR(a.top, b.top, tolerance);
}

TEST(AABB, DefaultIsZeroAndCtorNormalizes)
{
    AABB empty;
    ExpectVecNear(empty.min, Vec3F());
    ExpectVecNear(empty.max, Vec3F());

    AABB box(Vec3F(5, -1, 3), Vec3F(1, 2, -3));
    ExpectVecNear(box.min, Vec3F(1, -1, -3));
    ExpectVecNear(box.max, Vec3F(5, 2, 3));
}

TEST(AABB, CenterSizeRoundTrip)
{
    Vec3F center(1, -2, 3);
    Vec3F size(10, 20, 30);

    AABB box = AABB::FromCenterSize(center, size);
    ExpectVecNear(box.min, Vec3F(-4, -12, -12));
    ExpectVecNear(box.max, Vec3F(6, 8, 18));
    ExpectVecNear(box.GetCenter(), center);
    ExpectVecNear(box.GetSize(), size);
}

TEST(AABB, FromRectToRectRoundTrip)
{
    RectF rect(-3.0f, 7.0f, 5.0f, 1.0f);
    AABB box = AABB::FromRect(rect, -2.0f, 4.0f);

    ExpectVecNear(box.min, Vec3F(-3, 1, -2));
    ExpectVecNear(box.max, Vec3F(5, 7, 4));
    ExpectRectNear(box.ToRect(), rect);

    AABB flat = AABB::FromRect(rect);
    EXPECT_FLOAT_EQ(flat.min.z, 0.0f);
    EXPECT_FLOAT_EQ(flat.max.z, 0.0f);
}

TEST(AABB, IncludeExpandsBounds)
{
    AABB box(Vec3F(0, 0, 0), Vec3F(1, 1, 1));

    box.Include(Vec3F(2, -3, 0.5f));
    ExpectVecNear(box.min, Vec3F(0, -3, 0));
    ExpectVecNear(box.max, Vec3F(2, 1, 1));

    box.Include(AABB(Vec3F(-1, 0, -5), Vec3F(0, 0, 7)));
    ExpectVecNear(box.min, Vec3F(-1, -3, -5));
    ExpectVecNear(box.max, Vec3F(2, 1, 7));

    AABB expanded = AABB(Vec3F(), Vec3F(1, 1, 1)).Expand(AABB(Vec3F(3, 3, 3), Vec3F(4, 4, 4)));
    ExpectVecNear(expanded.min, Vec3F());
    ExpectVecNear(expanded.max, Vec3F(4, 4, 4));

    ExpectVecNear((AABB(Vec3F(), Vec3F(1, 1, 1)) + AABB(Vec3F(3, 3, 3), Vec3F(4, 4, 4))).max, Vec3F(4, 4, 4));
}

TEST(AABB, OffsetOperators)
{
    AABB box(Vec3F(1, 2, 3), Vec3F(4, 5, 6));
    Vec3F offset(10, -20, 30);

    AABB moved = box + offset;
    ExpectVecNear(moved.min, Vec3F(11, -18, 33));
    ExpectVecNear(moved.max, Vec3F(14, -15, 36));
    ExpectVecNear((moved - offset).min, box.min);

    box += offset;
    EXPECT_TRUE(box == moved);
    box -= offset;
    EXPECT_TRUE(box != moved);
}

TEST(AABB, IntersectsAndIsInside)
{
    AABB box(Vec3F(0, 0, 0), Vec3F(10, 10, 10));

    EXPECT_TRUE(box.Intersects(AABB(Vec3F(5, 5, 5), Vec3F(15, 15, 15))));
    EXPECT_TRUE(box.Intersects(AABB(Vec3F(-1, -1, -1), Vec3F(0.5f, 0.5f, 0.5f))));
    EXPECT_FALSE(box.Intersects(AABB(Vec3F(11, 0, 0), Vec3F(12, 10, 10))));
    EXPECT_FALSE(box.Intersects(AABB(Vec3F(0, 0, 11), Vec3F(10, 10, 12))));

    EXPECT_TRUE(box.IsInside(Vec3F(5, 5, 5)));
    EXPECT_FALSE(box.IsInside(Vec3F(5, 5, 11)));
    EXPECT_FALSE(box.IsInside(Vec3F(-1, 5, 5)));
}

TEST(AABB, BoundOfPoints)
{
    Vec3F points[] = { Vec3F(1, 2, 3), Vec3F(-5, 8, 0), Vec3F(4, -7, 12), Vec3F(0, 0, -9) };
    AABB box = AABB::Bound(points, 4);

    ExpectVecNear(box.min, Vec3F(-5, -7, -9));
    ExpectVecNear(box.max, Vec3F(4, 8, 12));
}

TEST(AABB, BasisAABBMatchesHandComputedCorners)
{
    Basis3D identityBox(Vec3F(1, 2, 3), Vec3F(10, 0, 0), Vec3F(0, 20, 0), Vec3F(0, 0, 30));
    AABB bounds = identityBox.AABB();
    ExpectVecNear(bounds.min, Vec3F(1, 2, 3));
    ExpectVecNear(bounds.max, Vec3F(11, 22, 33));

    // 90 degrees about X: y axis goes to z, z axis goes to -y
    Basis3D rotated = Basis3D::Build(Vec3F(1, 2, 3), Vec3F(10, 20, 30),
                                     Quat::FromAxisAngle(Vec3F::XAxis(), Math::PI()*0.5f));
    AABB rotatedBounds = rotated.AABB();
    ExpectVecNear(rotatedBounds.min, Vec3F(1, 2 - 30, 3), 1e-3f);
    ExpectVecNear(rotatedBounds.max, Vec3F(11, 2, 3 + 20), 1e-3f);
}

TEST(AABB, BasisAABBConsistentWith2DBasisAABB)
{
    struct Case { Vec2F position; Vec2F scale; float angle; float shear; };
    Case cases[] =
    {
        { Vec2F(), Vec2F(1, 1), 0.0f, 0.0f },
        { Vec2F(10, -5), Vec2F(2, 0.5f), 0.7f, 0.0f },
        { Vec2F(-3, 8), Vec2F(1, 3), -1.2f, 0.4f },
        { Vec2F(100, 50), Vec2F(0.25f, 4), 2.9f, -0.6f },
    };

    for (auto& c : cases)
    {
        Basis basis2D = Basis::Build(c.position, c.scale, c.angle, c.shear);
        ExpectRectNear(Basis3D(basis2D).AABB().ToRect(), basis2D.AABB(), 0.001f);
    }
}

TEST(AABB, IntersectsRayHitFromOutsideGivesNearestDistance)
{
    AABB box(Vec3F(-1.0f, -1.0f, -1.0f), Vec3F(1.0f, 1.0f, 1.0f));

    float distance = 0.0f;
    ASSERT_TRUE(box.IntersectsRay(Vec3F(0.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, -1.0f), distance));
    EXPECT_NEAR(distance, 4.0f, 1e-4f);

    ASSERT_TRUE(box.IntersectsRay(Vec3F(-10.0f, 0.5f, 0.5f), Vec3F(1.0f, 0.0f, 0.0f), distance));
    EXPECT_NEAR(distance, 9.0f, 1e-4f);

    Vec3F origin(5.0f, 5.0f, 5.0f);
    ASSERT_TRUE(box.IntersectsRay(origin, (Vec3F() - origin).Normalized(), distance));
    EXPECT_NEAR(distance, origin.Length() - Vec3F(1.0f, 1.0f, 1.0f).Length(), 1e-3f);
}

TEST(AABB, IntersectsRayMissAndBehind)
{
    AABB box(Vec3F(-1.0f, -1.0f, -1.0f), Vec3F(1.0f, 1.0f, 1.0f));

    float distance = 0.0f;
    EXPECT_FALSE(box.IntersectsRay(Vec3F(5.0f, 5.0f, 5.0f), Vec3F(0.0f, 0.0f, -1.0f), distance));
    EXPECT_FALSE(box.IntersectsRay(Vec3F(0.0f, 2.0f, 5.0f), Vec3F(0.0f, 0.0f, -1.0f), distance));

    // Box behind the ray misses
    EXPECT_FALSE(box.IntersectsRay(Vec3F(0.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, 1.0f), distance));
}

TEST(AABB, IntersectsRayInsideGivesZero)
{
    AABB box(Vec3F(-1.0f, -1.0f, -1.0f), Vec3F(1.0f, 1.0f, 1.0f));

    float distance = -1.0f;
    ASSERT_TRUE(box.IntersectsRay(Vec3F(0.2f, -0.3f, 0.5f), Vec3F(0.0f, 1.0f, 0.0f), distance));
    EXPECT_NEAR(distance, 0.0f, 1e-4f);
}

TEST(AABB, IntersectsRayParallelInsideSlabHitsOutsideMisses)
{
    AABB box(Vec3F(-1.0f, -1.0f, -1.0f), Vec3F(1.0f, 1.0f, 1.0f));

    float distance = 0.0f;
    EXPECT_TRUE(box.IntersectsRay(Vec3F(0.0f, 0.0f, -5.0f), Vec3F(0.0f, 0.0f, 1.0f), distance));
    EXPECT_FALSE(box.IntersectsRay(Vec3F(0.0f, 2.0f, -5.0f), Vec3F(0.0f, 0.0f, 1.0f), distance));
}

TEST(AABB, TransformedAppliesScaleRotationTranslation)
{
    AABB box(Vec3F(-1.0f, -2.0f, -3.0f), Vec3F(1.0f, 2.0f, 3.0f));

    // 90 degrees around Z swaps x and y extents, uniform scale 2, translated
    Basis3D basis = Basis3D::Build(Vec3F(10.0f, 20.0f, 30.0f), Vec3F(2.0f, 2.0f, 2.0f),
                                   Quat::FromAxisAngle(Vec3F::ZAxis(), Math::PI()*0.5f));

    AABB transformed = box.Transformed(basis);

    ExpectVecNear(transformed.min, Vec3F(10.0f - 4.0f, 20.0f - 2.0f, 30.0f - 6.0f), 1e-3f);
    ExpectVecNear(transformed.max, Vec3F(10.0f + 4.0f, 20.0f + 2.0f, 30.0f + 6.0f), 1e-3f);
}
