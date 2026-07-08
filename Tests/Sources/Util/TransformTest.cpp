#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Transform.h"

using namespace o2;

TEST(Transform3D, DefaultsMatch2DConstructor)
{
    Transform t(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));

    EXPECT_EQ(t.GetPosition(), Vec3F(10, 20, 0));
    EXPECT_EQ(t.GetSize(), Vec3F(100, 50, 0));
    EXPECT_EQ(t.GetScale(), Vec3F(2, 3, 1));
    EXPECT_EQ(t.GetPivot(), Vec3F(0.3f, 0.7f, 0));
    EXPECT_EQ(t.GetEulerAngles(), Vec3F(0, 0, 0.5f));
    EXPECT_EQ(t.GetShear(), Vec3F(0, 0, 0));
}

TEST(Transform3D, Vec3RoundTrips)
{
    Transform t;
    t.SetPosition(Vec3F(1, 2, 3));
    t.SetSize(Vec3F(10, 20, 30));
    t.SetScale(Vec3F(2, 3, 4));
    t.SetPivot(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetEulerAngles(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetShear(Vec3F(0.1f, 0.2f, 0.3f));

    EXPECT_EQ(t.GetPosition(), Vec3F(1, 2, 3));
    EXPECT_EQ(t.GetSize(), Vec3F(10, 20, 30));
    EXPECT_EQ(t.GetScale(), Vec3F(2, 3, 4));
    EXPECT_EQ(t.GetPivot(), Vec3F(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(t.GetEulerAngles(), Vec3F(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(t.GetShear(), Vec3F(0.1f, 0.2f, 0.3f));
}

TEST(Transform3D, Helpers2DPreserveOtherComponents)
{
    Transform t;
    t.SetPosition(Vec3F(1, 2, 3));
    t.SetSize(Vec3F(10, 20, 30));
    t.SetScale(Vec3F(2, 3, 4));
    t.SetPivot(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetEulerAngles(Vec3F(0.1f, 0.2f, 0.3f));
    t.SetShear(Vec3F(0.1f, 0.2f, 0.3f));

    t.SetPosition2D(Vec2F(5, 6));
    t.SetSize2D(Vec2F(50, 60));
    t.SetScale2D(Vec2F(5, 6));
    t.SetPivot2D(Vec2F(0.5f, 0.6f));
    t.SetAngle(0.7f);
    t.SetShear2D(0.5f);

    EXPECT_EQ(t.GetPosition(), Vec3F(5, 6, 3));
    EXPECT_EQ(t.GetSize(), Vec3F(50, 60, 30));
    EXPECT_EQ(t.GetScale(), Vec3F(5, 6, 4));
    EXPECT_EQ(t.GetPivot(), Vec3F(0.5f, 0.6f, 0.3f));
    EXPECT_EQ(t.GetEulerAngles(), Vec3F(0.1f, 0.2f, 0.7f));
    EXPECT_EQ(t.GetShear(), Vec3F(0.5f, 0.2f, 0.3f));

    EXPECT_EQ(t.GetPosition2D(), Vec2F(5, 6));
    EXPECT_EQ(t.GetSize2D(), Vec2F(50, 60));
    EXPECT_EQ(t.GetScale2D(), Vec2F(5, 6));
    EXPECT_EQ(t.GetPivot2D(), Vec2F(0.5f, 0.6f));
    EXPECT_FLOAT_EQ(t.GetAngle(), 0.7f);
    EXPECT_FLOAT_EQ(t.GetShear2D(), 0.5f);
}

TEST(Transform3D, RotationQuaternionRoundTrip)
{
    Transform t;
    t.SetEulerAngles(Vec3F(0.2f, 0.4f, 0.6f));

    Quat q = t.GetRotation();

    Transform r;
    r.SetRotation(q);

    Vec3F euler = r.GetEulerAngles();
    EXPECT_NEAR(euler.x, 0.2f, 0.001f);
    EXPECT_NEAR(euler.y, 0.4f, 0.001f);
    EXPECT_NEAR(euler.z, 0.6f, 0.001f);
}

TEST(Transform3D, Basis3DProjectsToBasis)
{
    Transform t(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));
    t.SetPositionZ(5.0f);
    t.SetEulerAngles(Vec3F(0.1f, 0.2f, 0.5f));

    EXPECT_EQ(t.GetBasis3D().ToBasis(), t.GetBasis());
    EXPECT_EQ(t.GetNonSizedBasis3D().ToBasis(), t.GetNonSizedBasis());
}

TEST(Transform3D, Pure2DBasisMatchesOldFormula)
{
    Vec2F position(10, 20), size(100, 50), scale(2, 3), pivot(0.3f, 0.7f);
    float angle = 0.5f, shear = 0.1f;

    Transform t(size, position, angle, scale, pivot);
    t.SetShear2D(shear);

    // The pre-3D Transform::UpdateTransform formula
    Basis nonSized = Basis::Build(position, scale, angle, shear);
    Basis sized(nonSized.origin, nonSized.xv*size.x, nonSized.yv*size.y);
    sized.origin = sized.origin - sized.xv*pivot.x - sized.yv*pivot.y;
    nonSized.origin = sized.origin;

    Basis result = t.GetBasis();
    EXPECT_FLOAT_EQ(result.xv.x, sized.xv.x);
    EXPECT_FLOAT_EQ(result.xv.y, sized.xv.y);
    EXPECT_FLOAT_EQ(result.yv.x, sized.yv.x);
    EXPECT_FLOAT_EQ(result.yv.y, sized.yv.y);
    EXPECT_FLOAT_EQ(result.origin.x, sized.origin.x);
    EXPECT_FLOAT_EQ(result.origin.y, sized.origin.y);

    Basis nonSizedResult = t.GetNonSizedBasis();
    EXPECT_FLOAT_EQ(nonSizedResult.xv.x, nonSized.xv.x);
    EXPECT_FLOAT_EQ(nonSizedResult.xv.y, nonSized.xv.y);
    EXPECT_FLOAT_EQ(nonSizedResult.yv.x, nonSized.yv.x);
    EXPECT_FLOAT_EQ(nonSizedResult.yv.y, nonSized.yv.y);
    EXPECT_FLOAT_EQ(nonSizedResult.origin.x, nonSized.origin.x);
    EXPECT_FLOAT_EQ(nonSizedResult.origin.y, nonSized.origin.y);
}

TEST(Transform3D, SetBasisPreserves3DComponents)
{
    Transform t;
    t.SetPositionZ(5.0f);
    t.SetEulerAngles(Vec3F(0.1f, 0.2f, 0));
    t.SetSizeZ(30.0f);

    t.SetBasis(Basis(Vec2F(10, 20), Vec2F(100, 0), Vec2F(0, 50)));

    EXPECT_FLOAT_EQ(t.GetPositionZ(), 5.0f);
    EXPECT_NEAR(t.GetEulerAngles().x, 0.1f, 0.001f);
    EXPECT_NEAR(t.GetEulerAngles().y, 0.2f, 0.001f);
    EXPECT_FLOAT_EQ(t.GetSizeZ(), 30.0f);
}

TEST(Transform3D, EqualityComparesAllComponents)
{
    Transform a(Vec2F(100, 50), Vec2F(10, 20));
    Transform b(a);
    EXPECT_TRUE(a == b);

    b.SetPositionZ(1.0f);
    EXPECT_TRUE(a != b);

    b = a;
    b.SetEulerAngles(Vec3F(0.1f, 0, 0));
    EXPECT_TRUE(a != b);

    b = a;
    b.SetScaleZ(2.0f);
    EXPECT_TRUE(a != b);

    b = a;
    b.SetShear(Vec3F(0, 0.1f, 0));
    EXPECT_TRUE(a != b);
}

TEST(Transform3D, PointConversionsMatch2D)
{
    Transform t(Vec2F(100, 50), Vec2F(10, 20), 0.5f, Vec2F(2, 3), Vec2F(0.3f, 0.7f));

    Vec2F world = t.Local2WorldPoint(Vec2F(30, 40));
    Vec2F local = t.World2LocalPoint(world);
    EXPECT_NEAR(local.x, 30.0f, 0.01f);
    EXPECT_NEAR(local.y, 40.0f, 0.01f);

    Vec2F worldDir = t.Local2WorldDir(Vec2F(1, 0));
    Vec2F localDir = t.World2LocalDir(worldDir);
    EXPECT_NEAR(localDir.x, 1.0f, 0.001f);
    EXPECT_NEAR(localDir.y, 0.0f, 0.001f);
}
