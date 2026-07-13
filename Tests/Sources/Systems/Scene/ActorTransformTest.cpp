#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    bool VecNear(const Vec2F& a, const Vec2F& b, float eps = kEps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps;
    }
}

// ===== Defaults =====

TEST(ActorTransform, DefaultsAreIdentityLike)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    EXPECT_TRUE(VecNear(a->transform->GetPosition2D(), Vec2F(0, 0)));
    EXPECT_TRUE(VecNear(a->transform->GetScale2D(), Vec2F(1, 1)));
    EXPECT_TRUE(VecNear(a->transform->GetPivot2D(), Vec2F(0.5f, 0.5f)));
    EXPECT_NEAR(a->transform->GetAngle(), 0.0f, kEps);
}

// ===== Setters round-trip =====

TEST(ActorTransform, PositionSetterGetterRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(15.5f, -7.25f));
    EXPECT_TRUE(VecNear(a->transform->GetPosition2D(), Vec2F(15.5f, -7.25f)));
}

TEST(ActorTransform, PositionXYAxisIndependent)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPositionX(5.0f);
    a->transform->SetPositionY(11.0f);
    EXPECT_FLOAT_EQ(a->transform->GetPositionX(), 5.0f);
    EXPECT_FLOAT_EQ(a->transform->GetPositionY(), 11.0f);
}

TEST(ActorTransform, SizeSetterGetterRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetSize2D(Vec2F(100, 50));
    EXPECT_TRUE(VecNear(a->transform->GetSize2D(), Vec2F(100, 50)));
    EXPECT_FLOAT_EQ(a->transform->GetWidth(), 100.0f);
    EXPECT_FLOAT_EQ(a->transform->GetHeight(), 50.0f);
}

TEST(ActorTransform, ScaleSetterGetterRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetScale2D(Vec2F(2.0f, 3.0f));
    EXPECT_TRUE(VecNear(a->transform->GetScale2D(), Vec2F(2.0f, 3.0f)));
}

TEST(ActorTransform, AngleDegreesSetterGetterRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetAngleDegrees(90.0f);
    EXPECT_NEAR(a->transform->GetAngleDegrees(), 90.0f, kEps);
}

TEST(ActorTransform, PivotSetterGetterRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPivot2D(Vec2F(0.0f, 0.0f));
    EXPECT_TRUE(VecNear(a->transform->GetPivot2D(), Vec2F(0.0f, 0.0f)));
}

// ===== Rect =====

TEST(ActorTransform, RectRoundTripUnrotated)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    RectF r(10, 20, 110, 70);
    a->transform->SetRect(r);
    auto got = a->transform->GetRect();
    EXPECT_NEAR(got.left, r.left, kEps);
    EXPECT_NEAR(got.top, r.top, kEps);
    EXPECT_NEAR(got.right, r.right, kEps);
    EXPECT_NEAR(got.bottom, r.bottom, kEps);
}

// ===== World vs Local =====

TEST(ActorTransform, WithoutParentWorldEqualsLocalAfterUpdate)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(5, 5));
    TickFrame();
    a->UpdateTransform();

    EXPECT_TRUE(VecNear(a->transform->GetWorldPosition2D(), a->transform->GetPosition2D()));
}

TEST(ActorTransform, ChildWorldPositionIncludesParentPosition)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    parent->transform->SetPosition2D(Vec2F(100, 50));
    child->transform->SetPosition2D(Vec2F(10, 0));
    TickFrame();

    auto world = child->transform->GetWorldPosition2D();
    EXPECT_TRUE(VecNear(world, Vec2F(110, 50)));
}

TEST(ActorTransform, SetParentKeepsWorldPositionWhenWorldStays)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(50, 50));
    TickFrame();

    auto worldBefore = a->transform->GetWorldPosition2D();

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    parent->transform->SetPosition2D(Vec2F(20, 0));
    TickFrame();

    a->SetParent(parent, /*worldPositionStays*/ true);
    TickFrame();

    auto worldAfter = a->transform->GetWorldPosition2D();
    EXPECT_TRUE(VecNear(worldBefore, worldAfter));
}

TEST(ActorTransform, SetParentDoesNotKeepWorldWhenFlagFalse)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(50, 50));
    TickFrame();

    auto worldBefore = a->transform->GetWorldPosition2D();

    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    parent->transform->SetPosition2D(Vec2F(20, 0));
    TickFrame();

    a->SetParent(parent, /*worldPositionStays*/ false);
    TickFrame();

    auto worldAfter = a->transform->GetWorldPosition2D();
    EXPECT_FALSE(VecNear(worldBefore, worldAfter));
}

TEST(ActorTransform, ChildPositionIsLocalRelativeToParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    parent->transform->SetPosition2D(Vec2F(100, 200));
    child->transform->SetPosition2D(Vec2F(5, 5));
    TickFrame();

    EXPECT_TRUE(VecNear(child->transform->GetPosition2D(), Vec2F(5, 5)));
    EXPECT_TRUE(VecNear(parent->transform->GetPosition2D(), Vec2F(100, 200)));
}

// ===== Children pivot/transform inheritance =====

TEST(ActorTransform, ParentMoveUpdatesChildWorldOnTick)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    parent->transform->SetPosition2D(Vec2F(0, 0));
    child->transform->SetPosition2D(Vec2F(5, 0));
    TickFrame();

    EXPECT_TRUE(VecNear(child->transform->GetWorldPosition2D(), Vec2F(5, 0)));

    parent->transform->SetPosition2D(Vec2F(100, 0));
    TickFrame();

    EXPECT_TRUE(VecNear(child->transform->GetWorldPosition2D(), Vec2F(105, 0)));
}

// ===== IsPointInside =====

TEST(ActorTransform, IsPointInsideForUnrotatedRect)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPivot2D(Vec2F(0, 0));
    a->transform->SetSize2D(Vec2F(100, 100));
    a->transform->SetPosition2D(Vec2F(0, 0));
    TickFrame();

    EXPECT_TRUE(a->transform->IsPointInside(Vec2F(50, 50)));
    EXPECT_FALSE(a->transform->IsPointInside(Vec2F(-10, 50)));
    EXPECT_FALSE(a->transform->IsPointInside(Vec2F(200, 50)));
}

// ===== Basis =====

TEST(ActorTransform, GetBasisDecomposesIntoPositionScale)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(0, 0));
    a->transform->SetSize2D(Vec2F(10, 20));
    a->transform->SetPivot2D(Vec2F(0, 0));
    a->transform->SetAngleDegrees(0);
    a->UpdateTransform();

    auto basis = a->transform->GetBasis();
    EXPECT_NEAR(basis.xv.x, 10.0f, kEps);
    EXPECT_NEAR(basis.yv.y, 20.0f, kEps);
}

// Regression: SetBasis on a 3D-rotated actor must decode size from the projected basis accounting
// for the x/y euler foreshortening, otherwise every basis round trip shrinks the actor
TEST(ActorTransform, SetBasisRoundTripPreserves3DRotatedSize)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetSize2D(Vec2F(100, 60));
    a->transform->SetEulerAngles(Vec3F(0.4f, 0.5f, 0.3f));
    a->UpdateTransform();

    Quat rotationBefore = a->transform->GetRotation();

    for (int i = 0; i < 5; i++)
    {
        a->transform->SetBasis(a->transform->GetBasis());
        a->UpdateTransform();
    }

    EXPECT_TRUE(VecNear(a->transform->GetSize2D(), Vec2F(100, 60), 0.01f));
    EXPECT_NEAR(a->transform->GetShear2D(), 0.0f, kEps);
    EXPECT_GT(Math::Abs(a->transform->GetRotation().Dot(rotationBefore)), 0.9999f);
}

TEST(ActorTransform, SetBasisRotatedByZKeepsSizeWith3DEuler)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetSize2D(Vec2F(100, 60));
    a->transform->SetEulerAngles(Vec3F(0.0f, 0.5f, 0.0f));
    a->UpdateTransform();

    // In-plane rotation of the projected basis: the 2D angle grows, size stays
    Basis rotated = a->transform->GetBasis()*Basis::Rotated(0.25f);
    a->transform->SetBasis(rotated);
    a->UpdateTransform();

    EXPECT_TRUE(VecNear(a->transform->GetSize2D(), Vec2F(100, 60), 0.01f));

    Vec3F euler = a->transform->GetEulerAngles();
    EXPECT_NEAR(euler.y, 0.5f, kEps);
    EXPECT_NEAR(euler.z > Math::PI() ? euler.z - 2.0f*Math::PI() : euler.z, 0.25f, kEps);
}
