#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2/Utils/Math/Matrix4.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    bool Vec3Near(const Vec3F& a, const Vec3F& b, float eps = kEps)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps && Math::Abs(a.z - b.z) < eps;
    }
}

// ===== Defaults =====

TEST(ActorTransform3D, DefaultsAre2D)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    EXPECT_FALSE(a->transform->Is3D());
    EXPECT_FLOAT_EQ(a->transform->GetPositionZ(), 0.0f);
    EXPECT_TRUE(Vec3Near(a->transform->GetScale(), Vec3F(1, 1, 1)));
    EXPECT_NEAR(a->transform->GetEulerAngles().z, a->transform->GetAngle(), kEps);

    a->transform->SetScale2D(Vec2F(2.0f, 3.0f));
    EXPECT_TRUE(Vec3Near(a->transform->GetScale(), Vec3F(2, 3, 1)));
    EXPECT_FALSE(a->transform->Is3D());
}

// ===== 2D equivalence =====

TEST(ActorTransform3D, LocalTransform3DMatchesNonSizedBasisFor2D)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(13.5f, -7.25f));
    a->transform->SetAngle(0.7f);
    a->transform->SetScale2D(Vec2F(2.0f, 0.5f));
    a->transform->SetSize2D(Vec2F(100, 50));
    TickFrame();

    Basis nonSized = a->transform->GetNonSizedBasis();
    Mat4 local3D = a->transform->GetLocalTransform3D();

    Vec2F points[] = { Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, 1), Vec2F(-3.5f, 12.0f), Vec2F(8.25f, -4.5f) };
    for (auto& p : points)
    {
        Vec2F p2 = p*nonSized;
        Vec3F p3 = local3D.TransformPoint(Vec3F(p, 0.0f));
        EXPECT_TRUE(Vec3Near(p3, Vec3F(p2, 0.0f))) << "point (" << p.x << ", " << p.y << ")";
    }
}

// ===== Round-trips =====

TEST(ActorTransform3D, SettersRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetPositionZ(5.5f);
    EXPECT_FLOAT_EQ(a->transform->GetPositionZ(), 5.5f);
    EXPECT_TRUE(a->transform->Is3D());

    a->transform->SetPosition(Vec3F(1, 2, 3));
    EXPECT_TRUE(Vec3Near(a->transform->GetPosition(), Vec3F(1, 2, 3)));
    EXPECT_TRUE(Vec3Near(Vec3F(a->transform->GetPosition2D(), 0), Vec3F(1, 2, 0)));

    a->transform->SetScale(Vec3F(2, 3, 4));
    EXPECT_TRUE(Vec3Near(a->transform->GetScale(), Vec3F(2, 3, 4)));

    a->transform->SetEulerAngles(Vec3F(0.3f, -0.5f, 0.9f));
    EXPECT_TRUE(Vec3Near(a->transform->GetEulerAngles(), Vec3F(0.3f, -0.5f, 0.9f)));
    EXPECT_NEAR(a->transform->GetAngle(), 0.9f, kEps);

    a->transform->SetScaleZ(7.0f);
    EXPECT_FLOAT_EQ(a->transform->GetScaleZ(), 7.0f);
    EXPECT_TRUE(Vec3Near(a->transform->GetScale(), Vec3F(2, 3, 7)));
}

TEST(ActorTransform3D, Setters2DPreserveOtherComponents)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetPosition(Vec3F(1, 2, 3));
    a->transform->SetPosition2D(Vec2F(5, 6));
    EXPECT_TRUE(Vec3Near(a->transform->GetPosition(), Vec3F(5, 6, 3)));

    a->transform->SetSize(Vec3F(10, 20, 30));
    a->transform->SetSize2D(Vec2F(40, 50));
    EXPECT_TRUE(Vec3Near(a->transform->GetSize(), Vec3F(40, 50, 30)));

    a->transform->SetScale(Vec3F(2, 3, 4));
    a->transform->SetScale2D(Vec2F(5, 6));
    EXPECT_TRUE(Vec3Near(a->transform->GetScale(), Vec3F(5, 6, 4)));

    a->transform->SetPivot(Vec3F(0.1f, 0.2f, 0.3f));
    a->transform->SetPivot2D(Vec2F(0.4f, 0.5f));
    EXPECT_TRUE(Vec3Near(a->transform->GetPivot(), Vec3F(0.4f, 0.5f, 0.3f)));

    a->transform->SetShear(Vec3F(0.1f, 0.2f, 0.3f));
    a->transform->SetShear2D(0.7f);
    EXPECT_TRUE(Vec3Near(a->transform->GetShear(), Vec3F(0.7f, 0.2f, 0.3f)));
    EXPECT_FLOAT_EQ(a->transform->GetShear2D(), 0.7f);
}

TEST(ActorTransform3D, WorldPosition2DPreservesZ)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetPosition(Vec3F(1, 2, 3));
    TickFrame();

    a->transform->SetWorldPosition2D(Vec2F(7, 8));
    TickFrame();

    EXPECT_TRUE(Vec3Near(a->transform->GetPosition(), Vec3F(7, 8, 3)));
    EXPECT_TRUE(Vec3Near(Vec3F(a->transform->GetWorldPosition2D(), 0), Vec3F(7, 8, 0)));
}

TEST(ActorTransform3D, EulerAnglesDegreesConvertToRadians)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetEulerAnglesDegrees(Vec3F(90.0f, -45.0f, 180.0f));
    EXPECT_TRUE(Vec3Near(a->transform->GetEulerAngles(),
                         Vec3F(Math::PI()*0.5f, -Math::PI()*0.25f, Math::PI())));
    EXPECT_TRUE(Vec3Near(a->transform->GetEulerAnglesDegrees(), Vec3F(90.0f, -45.0f, 180.0f)));
    EXPECT_NEAR(a->transform->GetAngleDegrees(), 180.0f, kEps);

    a->transform->SetAngleDegrees(30.0f);
    EXPECT_NEAR(a->transform->GetEulerAnglesDegrees().z, 30.0f, kEps);
}

TEST(ActorTransform3D, EulerZBehavesLikeSetAngle)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetSize2D(Vec2F(10, 20));
    b->transform->SetSize2D(Vec2F(10, 20));

    a->transform->SetAngle(0.6f);
    b->transform->SetEulerAngles(Vec3F(0, 0, 0.6f));
    TickFrame();

    EXPECT_NEAR(b->transform->GetAngle(), 0.6f, kEps);
    EXPECT_FALSE(b->transform->Is3D());
    EXPECT_TRUE(a->transform->GetBasis() == b->transform->GetBasis());

    b->transform->SetAngle(-0.25f);
    EXPECT_NEAR(b->transform->GetEulerAngles().z, -0.25f, kEps);
}

TEST(ActorTransform3D, RotationQuaternionRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    Vec3F euler(0.3f, -0.4f, 0.5f);
    a->transform->SetRotation(Quat::FromEuler(euler));
    EXPECT_TRUE(Vec3Near(a->transform->GetEulerAngles(), euler));
    EXPECT_TRUE(a->transform->GetRotation() == Quat::FromEuler(euler));
}

TEST(ActorTransform3D, SizePivotShear3DRoundTrips)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetSize(Vec3F(10, 20, 30));
    EXPECT_TRUE(Vec3Near(a->transform->GetSize(), Vec3F(10, 20, 30)));
    EXPECT_EQ(a->transform->GetSize2D(), Vec2F(10, 20));
    EXPECT_FLOAT_EQ(a->transform->GetSizeZ(), 30.0f);
    EXPECT_TRUE(a->transform->Is3D());

    a->transform->SetSize2D(Vec2F(15, 25));
    EXPECT_TRUE(Vec3Near(a->transform->GetSize(), Vec3F(15, 25, 30)));

    a->transform->SetPivot(Vec3F(0.1f, 0.2f, 0.5f));
    EXPECT_TRUE(Vec3Near(a->transform->GetPivot(), Vec3F(0.1f, 0.2f, 0.5f)));
    EXPECT_EQ(a->transform->GetPivot2D(), Vec2F(0.1f, 0.2f));
    EXPECT_FLOAT_EQ(a->transform->GetPivotZ(), 0.5f);

    a->transform->SetPivot2D(Vec2F(0.3f, 0.4f));
    EXPECT_TRUE(Vec3Near(a->transform->GetPivot(), Vec3F(0.3f, 0.4f, 0.5f)));

    a->transform->SetShear(Vec3F(0.1f, 0.2f, 0.3f));
    EXPECT_TRUE(Vec3Near(a->transform->GetShear(), Vec3F(0.1f, 0.2f, 0.3f)));
    EXPECT_FLOAT_EQ(a->transform->GetShear2D(), 0.1f);

    a->transform->SetShear2D(0.5f);
    EXPECT_TRUE(Vec3Near(a->transform->GetShear(), Vec3F(0.5f, 0.2f, 0.3f)));
}

TEST(ActorTransform3D, ShearZDefaultsMake2D)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->transform->SetShear2D(0.4f);
    EXPECT_FALSE(a->transform->Is3D());

    a->transform->SetShear(Vec3F(0.4f, 0.1f, 0.0f));
    EXPECT_TRUE(a->transform->Is3D());
}

TEST(ActorTransform3D, SizedBasis3DBoxCornersMatchSizeAndPivot)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec3F(1, 2, 3));
    a->transform->SetSize(Vec3F(10, 20, 30));
    a->transform->SetPivot(Vec3F(0.5f, 0.5f, 0.5f));
    TickFrame();

    Basis3D sized = a->transform->GetBasis3D();
    EXPECT_TRUE(Vec3Near(sized.xv, Vec3F(10, 0, 0)));
    EXPECT_TRUE(Vec3Near(sized.yv, Vec3F(0, 20, 0)));
    EXPECT_TRUE(Vec3Near(sized.zv, Vec3F(0, 0, 30)));
    EXPECT_TRUE(Vec3Near(sized.origin, Vec3F(1 - 5, 2 - 10, 3 - 15)));
    EXPECT_TRUE(Vec3Near(sized*Vec3F(1, 1, 1), Vec3F(1 + 5, 2 + 10, 3 + 15)));
    EXPECT_TRUE(Vec3Near(sized*Vec3F(0.5f, 0.5f, 0.5f), Vec3F(1, 2, 3)));

    // Sized world corners equal the non-sized world transform of local box points
    Mat4 world = a->transform->GetWorldTransform3D();
    Basis3D worldSized = a->transform->GetWorldBasis3D();
    Vec3F size = a->transform->GetSize();
    Vec3F pivot = a->transform->GetPivot();

    for (auto& corner : { Vec3F(0, 0, 0), Vec3F(1, 0, 0), Vec3F(0, 1, 0), Vec3F(0, 0, 1), Vec3F(1, 1, 1) })
    {
        Vec3F local = (corner - pivot)*size;
        EXPECT_TRUE(Vec3Near(worldSized*corner, world.TransformPoint(local)));
    }
}

TEST(ActorTransform3D, SizedBasis3DRotatedMatchesNonSizedComposition)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec3F(5, -3, 2));
    a->transform->SetSize(Vec3F(10, 20, 30));
    a->transform->SetPivot(Vec3F(0.5f, 0.5f, 0.0f));
    a->transform->SetEulerAngles(Vec3F(0.3f, -0.2f, 0.6f));
    a->transform->SetScale(Vec3F(2, 1, 0.5f));
    TickFrame();

    Basis3D sized = a->transform->GetBasis3D();
    Basis3D nonSized = a->transform->GetNonSizedBasis3D();

    EXPECT_TRUE(Vec3Near(sized.xv, nonSized.xv*10.0f));
    EXPECT_TRUE(Vec3Near(sized.yv, nonSized.yv*20.0f));
    EXPECT_TRUE(Vec3Near(sized.zv, nonSized.zv*30.0f));
    EXPECT_TRUE(Vec3Near(sized.origin, nonSized.origin - sized.xv*0.5f - sized.yv*0.5f));
}

// ===== Hierarchy =====

TEST(ActorTransform3D, ChildWorldPosition3DMatchesMatrixComposition)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    Vec3F parentPos(10, -5, 5);
    Vec3F parentEuler(0.2f, Math::Deg2rad(90.0f), 0.4f);
    Vec3F parentScale(2, 1, 0.5f);
    Vec3F childPos(1, 2, 3);

    parent->transform->SetPosition(parentPos);
    parent->transform->SetEulerAngles(parentEuler);
    parent->transform->SetScale(parentScale);
    child->transform->SetPosition(childPos);
    TickFrame();

    Mat4 parentM = Mat4::TRS(parentPos, Quat::FromEuler(parentEuler), parentScale);
    Vec3F expected = parentM.TransformPoint(childPos);

    EXPECT_TRUE(Vec3Near(child->transform->GetWorldPosition(), expected));
    EXPECT_TRUE(parent->transform->GetWorldTransform3D() == parentM);
    EXPECT_TRUE(child->transform->GetWorldTransform3D() == parentM*child->transform->GetLocalTransform3D());
}

TEST(ActorTransform3D, SetWorldPosition3DRoundTripUnderParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    parent->transform->SetPosition(Vec3F(10, 20, -3));
    parent->transform->SetEulerAngles(Vec3F(0.1f, 0.2f, 0.3f));
    TickFrame();

    child->transform->SetWorldPosition(Vec3F(4, 5, 6));
    EXPECT_TRUE(Vec3Near(child->transform->GetWorldPosition(), Vec3F(4, 5, 6)));
}

TEST(ActorTransform3D, HierarchyWith3DShearParentComposes)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    Vec3F parentPos(10, -5, 5);
    Vec3F parentEuler(0.2f, 0.4f, -0.3f);
    Vec3F parentScale(2, 1, 0.5f);
    Vec3F parentShear(0.3f, 0.2f, -0.1f);
    Vec3F childPos(1, 2, 3);

    parent->transform->SetPosition(parentPos);
    parent->transform->SetEulerAngles(parentEuler);
    parent->transform->SetScale(parentScale);
    parent->transform->SetShear(parentShear);
    child->transform->SetPosition(childPos);
    TickFrame();

    Basis3D parentBasis = Basis3D::Build(parentPos, parentScale, parentEuler, parentShear);
    EXPECT_TRUE(Vec3Near(child->transform->GetWorldPosition(), parentBasis*childPos));
    EXPECT_TRUE(parent->transform->GetWorldTransform3D() == parentBasis.ToMat4());
    EXPECT_TRUE(child->transform->GetWorldNonSizedBasis3D() ==
                Basis3D::Build(childPos, Vec3F::One(), Quat::Identity())*parentBasis);

    child->transform->SetWorldPosition(Vec3F(4, 5, 6));
    EXPECT_TRUE(Vec3Near(child->transform->GetWorldPosition(), Vec3F(4, 5, 6)));
}

// ===== Serialization =====

TEST(ActorTransform3D, SerializationRoundTripPreserves3DFields)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec3F(1, 2, 3));
    a->transform->SetEulerAngles(Vec3F(0.25f, -0.5f, 0.75f));
    a->transform->SetScale(Vec3F(2, 3, 4));
    a->transform->SetSize(Vec3F(10, 20, 30));
    a->transform->SetPivot(Vec3F(0.1f, 0.2f, 0.3f));
    a->transform->SetShear(Vec3F(0.4f, 0.5f, 0.6f));

    DataDocument doc;
    doc.Set(*a->transform);

    auto b = mmake<Actor>(ActorCreateMode::InScene);
    doc.Get(*b->transform);

    EXPECT_TRUE(Vec3Near(b->transform->GetPosition(), Vec3F(1, 2, 3)));
    EXPECT_TRUE(Vec3Near(b->transform->GetEulerAngles(), Vec3F(0.25f, -0.5f, 0.75f)));
    EXPECT_TRUE(Vec3Near(b->transform->GetScale(), Vec3F(2, 3, 4)));
    EXPECT_TRUE(Vec3Near(b->transform->GetSize(), Vec3F(10, 20, 30)));
    EXPECT_TRUE(Vec3Near(b->transform->GetPivot(), Vec3F(0.1f, 0.2f, 0.3f)));
    EXPECT_TRUE(Vec3Near(b->transform->GetShear(), Vec3F(0.4f, 0.5f, 0.6f)));
    EXPECT_TRUE(b->transform->Is3D());
}

TEST(ActorTransform3D, DefaultActorSerializationBackwardCompatible)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(7, 8));
    a->transform->SetAngle(0.5f);

    DataDocument doc;
    doc.Set(*a->transform);

    auto b = mmake<Actor>(ActorCreateMode::InScene);
    doc.Get(*b->transform);

    EXPECT_FALSE(b->transform->Is3D());
    EXPECT_FLOAT_EQ(b->transform->GetPositionZ(), 0.0f);
    EXPECT_TRUE(Vec3Near(b->transform->GetScale(), Vec3F(1, 1, 1)));
    EXPECT_NEAR(b->transform->GetAngle(), 0.5f, kEps);
    EXPECT_NEAR(b->transform->GetPositionX(), 7.0f, kEps);
}

// ===== 3D rectangle boxes =====

TEST(ActorTransform3D, Rect3DBoxesFor3DRotatedActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec3F(1, 2, 3));
    a->transform->SetSize(Vec3F(10, 20, 30));
    a->transform->SetPivot(Vec3F(0, 0, 0));
    a->transform->SetEulerAngles(Vec3F(Math::PI()*0.5f, 0, 0));
    TickFrame();

    // The rect box ignores rotation: position offset by pivot and size
    AABB rect = a->transform->GetRect3D();
    EXPECT_TRUE(Vec3Near(rect.min, Vec3F(1, 2, 3)));
    EXPECT_TRUE(Vec3Near(rect.max, Vec3F(11, 22, 33)));

    AABB worldRect = a->transform->GetWorldRect3D();
    EXPECT_TRUE(Vec3Near(worldRect.min, rect.min));
    EXPECT_TRUE(Vec3Near(worldRect.max, rect.max));

    // 90 degrees about X maps the size box (10, 20, 30) to x: 10, y: -30, z: 20
    AABB worldBounds = a->transform->GetWorldAABB();
    EXPECT_TRUE(Vec3Near(worldBounds.min, Vec3F(1, 2 - 30, 3)));
    EXPECT_TRUE(Vec3Near(worldBounds.max, Vec3F(11, 2, 3 + 20)));
}

TEST(ActorTransform3D, Rect3DBoxesMatch2DApiFor2DActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition2D(Vec2F(10, 20));
    a->transform->SetSize2D(Vec2F(30, 40));
    a->transform->SetPivot2D(Vec2F(0.5f, 0.5f));
    a->transform->SetAngle(0.6f);
    TickFrame();

    RectF worldRect = a->transform->GetWorldRect();
    RectF worldRect3D = a->transform->GetWorldRect3D().ToRect();
    EXPECT_NEAR(worldRect3D.left, worldRect.left, kEps);
    EXPECT_NEAR(worldRect3D.right, worldRect.right, kEps);
    EXPECT_NEAR(worldRect3D.bottom, worldRect.bottom, kEps);
    EXPECT_NEAR(worldRect3D.top, worldRect.top, kEps);

    RectF aaRect = a->transform->GetAxisAlignedRect();
    RectF aaRect3D = a->transform->GetWorldAABB().ToRect();
    EXPECT_NEAR(aaRect3D.left, aaRect.left, kEps);
    EXPECT_NEAR(aaRect3D.right, aaRect.right, kEps);
    EXPECT_NEAR(aaRect3D.bottom, aaRect.bottom, kEps);
    EXPECT_NEAR(aaRect3D.top, aaRect.top, kEps);

    EXPECT_TRUE(Vec3Near(a->transform->GetWorldAABB().GetSize(),
                         Vec3F(aaRect.Width(), aaRect.Height(), 0)));
}

// ===== 2D regression =====

// Mesh actors keep the default zero transform size, so their 2D basis is degenerate:
// applying it back (undo/redo, move steps) must not stomp the rotation
TEST(ActorTransform3D, ZeroSizeBasisRoundTripKeepsRotation)
{
    SceneCleanGuard guard;

    Vector<Vec3F> rotations = { Vec3F(0.0f, 0.0f, 0.5f), Vec3F(0.4f, 0.3f, 0.6f) };
    for (auto& eulerAngles : rotations)
    {
        auto a = mmake<Actor>(ActorCreateMode::InScene);
        a->transform->SetPosition(Vec3F(10.0f, 20.0f, 30.0f));
        a->transform->SetEulerAngles(eulerAngles);
        TickFrame();

        a->transform->SetBasis(a->transform->GetBasis());
        EXPECT_TRUE(Vec3Near(a->transform->GetEulerAngles(), eulerAngles))
            << "SetBasis with zero size must keep rotation (" << eulerAngles.z << "), got "
            << a->transform->GetEulerAngles().z;

        a->transform->SetScale(Vec3F(0.0f, 0.0f, 1.0f));
        TickFrame();

        a->transform->SetNonSizedBasis(a->transform->GetNonSizedBasis());
        EXPECT_TRUE(Vec3Near(a->transform->GetEulerAngles(), eulerAngles))
            << "SetNonSizedBasis with zero scale must keep rotation";
    }
}

TEST(ActorTransform3D, Old2DApiKeepsBasisBehavior)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPivot2D(Vec2F(0, 0));
    a->transform->SetSize2D(Vec2F(10, 20));
    a->transform->SetPosition2D(Vec2F(10, 20));
    a->transform->SetAngle(Math::Deg2rad(90.0f));
    TickFrame();

    Basis basis = a->transform->GetBasis();
    EXPECT_NEAR(basis.origin.x, 10.0f, kEps);
    EXPECT_NEAR(basis.origin.y, 20.0f, kEps);
    EXPECT_NEAR(basis.xv.x, 0.0f, kEps);
    EXPECT_NEAR(basis.xv.y, 10.0f, kEps);
    EXPECT_NEAR(basis.yv.x, -20.0f, kEps);
    EXPECT_NEAR(basis.yv.y, 0.0f, kEps);
}
