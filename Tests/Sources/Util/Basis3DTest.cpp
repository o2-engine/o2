#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Matrix4.h"

using namespace o2;

static void ExpectVecNear(const Vec3F& a, const Vec3F& b, float tolerance = 1e-4f)
{
    EXPECT_NEAR(a.x, b.x, tolerance);
    EXPECT_NEAR(a.y, b.y, tolerance);
    EXPECT_NEAR(a.z, b.z, tolerance);
}

static void ExpectVec2Near(const Vec2F& a, const Vec2F& b, float tolerance = 1e-3f)
{
    EXPECT_NEAR(a.x, b.x, tolerance);
    EXPECT_NEAR(a.y, b.y, tolerance);
}

static void ExpectBasisNear(const Basis3D& a, const Basis3D& b, float tolerance = 1e-4f)
{
    ExpectVecNear(a.xv, b.xv, tolerance);
    ExpectVecNear(a.yv, b.yv, tolerance);
    ExpectVecNear(a.zv, b.zv, tolerance);
    ExpectVecNear(a.origin, b.origin, tolerance);
}

TEST(Basis3D, IdentityTransformsPointsToThemselves)
{
    Basis3D identity;
    Vec3F p(1.5f, -2.0f, 3.25f);
    ExpectVecNear(identity*p, p);
    ExpectBasisNear(identity, Basis3D::Identity());
}

TEST(Basis3D, TransformMatchesMat4Path)
{
    Basis3D b = Basis3D::Build(Vec3F(1, 2, 3), Vec3F(2, 0.5f, 3), Quat::FromEuler(Vec3F(0.3f, -0.7f, 1.1f)),
                               Vec3F(0.2f, -0.1f, 0.3f));

    Vec3F points[] = { Vec3F(), Vec3F(1, 0, 0), Vec3F(0, 1, 0), Vec3F(0, 0, 1), Vec3F(-3.5f, 12.0f, 4.25f) };
    Mat4 mat = b.ToMat4();

    for (auto& p : points)
        ExpectVecNear(b*p, mat.TransformPoint(p));
}

TEST(Basis3D, CompositionMatchesMat4Multiplication)
{
    Basis3D a = Basis3D::Build(Vec3F(1, 2, 3), Vec3F(2, 0.5f, 1), Quat::FromEuler(Vec3F(0.3f, -0.2f, 0.9f)));
    Basis3D b = Basis3D::Build(Vec3F(-5, 4, 0.5f), Vec3F(1, 3, 2), Quat::FromEuler(Vec3F(-0.1f, 0.6f, -1.3f)));

    // a*b applies a first, matching the 2D Basis convention; Mat4 composes in reverse order
    Basis3D composed = a*b;
    Mat4 composedMat = b.ToMat4()*a.ToMat4();

    Vec3F p(1.5f, -2.0f, 3.0f);
    ExpectVecNear(composed*p, composedMat.TransformPoint(p));
    ExpectVecNear(composed*p, b*(a*p));
}

TEST(Basis3D, InvertedComposesToIdentity)
{
    Basis3D b = Basis3D::Build(Vec3F(10, -5, 3), Vec3F(2, 0.5f, 3), Quat::FromEuler(Vec3F(0.4f, 0.7f, -0.2f)),
                               Vec3F(0.3f, 0.1f, -0.2f));

    ExpectBasisNear(b*b.Inverted(), Basis3D::Identity());
    ExpectBasisNear(b.Inverted()*b, Basis3D::Identity());

    Vec3F p(3.5f, -1.0f, 7.0f);
    ExpectVecNear(b.Inverted()*(b*p), p);
}

TEST(Basis3D, BuildDecomposeRoundTrip)
{
    Vec3F position(4, -7, 2.5f);
    Vec3F scale(2, 0.5f, 3);
    Quat rotation = Quat::FromEuler(Vec3F(0.3f, -0.6f, 1.2f));

    Basis3D b = Basis3D::Build(position, scale, rotation);

    Vec3F decomposedPos, decomposedScale;
    Quat decomposedRot;
    b.Decompose(&decomposedPos, &decomposedRot, &decomposedScale);

    ExpectVecNear(decomposedPos, position);
    ExpectVecNear(decomposedScale, scale);
    ExpectVecNear(decomposedRot*Vec3F(1, 0, 0), rotation*Vec3F(1, 0, 0));
    ExpectVecNear(decomposedRot*Vec3F(0, 0, 1), rotation*Vec3F(0, 0, 1));

    ExpectVecNear(b.GetScale(), scale);
    ExpectVecNear(b.GetRotation()*Vec3F(0, 1, 0), rotation*Vec3F(0, 1, 0));
    ExpectVecNear(b.GetShear(), Vec3F());
}

TEST(Basis3D, GetShearRecoversBuildShear)
{
    Vec3F shear(0.3f, 0.2f, -0.4f);
    Basis3D b = Basis3D::Build(Vec3F(1, 2, 3), Vec3F(2, 0.5f, 3), Quat::FromEuler(Vec3F(0.2f, -0.3f, 0.7f)), shear);

    ExpectVecNear(b.GetShear(), shear);
    ExpectVecNear(b.GetScale(), Vec3F(2, 0.5f, 3));
}

TEST(Basis3D, ShearPlanesDisplaceUnitPoints)
{
    float c = Math::Sqrt(1.0f - 0.25f);

    Basis3D shearXY = Basis3D::Build(Vec3F(), Vec3F::One(), Quat::Identity(), Vec3F(0.5f, 0, 0));
    ExpectVecNear(shearXY*Vec3F(0, 1, 0), Vec3F(0.5f, c, 0));
    ExpectVecNear(shearXY*Vec3F(1, 0, 0), Vec3F(1, 0, 0));
    ExpectVecNear(shearXY*Vec3F(0, 0, 1), Vec3F(0, 0, 1));

    Basis3D shearXZ = Basis3D::Build(Vec3F(), Vec3F::One(), Quat::Identity(), Vec3F(0, 0.5f, 0));
    ExpectVecNear(shearXZ*Vec3F(0, 0, 1), Vec3F(0.5f, 0, c));
    ExpectVecNear(shearXZ*Vec3F(1, 0, 0), Vec3F(1, 0, 0));
    ExpectVecNear(shearXZ*Vec3F(0, 1, 0), Vec3F(0, 1, 0));

    Basis3D shearYZ = Basis3D::Build(Vec3F(), Vec3F::One(), Quat::Identity(), Vec3F(0, 0, 0.5f));
    ExpectVecNear(shearYZ*Vec3F(0, 0, 1), Vec3F(0, 0.5f, c));
    ExpectVecNear(shearYZ*Vec3F(1, 0, 0), Vec3F(1, 0, 0));
    ExpectVecNear(shearYZ*Vec3F(0, 1, 0), Vec3F(0, 1, 0));
}

TEST(Basis3D, TwoDBuildConsistency)
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
        Basis expected = Basis::Build(c.position, c.scale, c.angle, c.shear);
        Basis projected = Basis3D::Build(Vec3F(c.position, 0), Vec3F(c.scale, 1),
                                         Quat::FromEuler(Vec3F(0, 0, c.angle)), Vec3F(c.shear, 0, 0)).ToBasis();

        ExpectVec2Near(projected.xv, expected.xv);
        ExpectVec2Near(projected.yv, expected.yv);
        ExpectVec2Near(projected.origin, expected.origin);
    }
}

TEST(Basis3D, ToBasisFromBasisRoundTrip)
{
    Basis basis2D = Basis::Build(Vec2F(10, -5), Vec2F(2, 0.5f), 0.7f, 0.3f);
    Basis3D embedded(basis2D);

    Basis projected = embedded.ToBasis();
    ExpectVec2Near(projected.xv, basis2D.xv);
    ExpectVec2Near(projected.yv, basis2D.yv);
    ExpectVec2Near(projected.origin, basis2D.origin);

    Vec2F p(3.5f, -1.25f);
    Vec3F transformed = embedded*Vec3F(p, 0.0f);
    ExpectVecNear(transformed, Vec3F(p*basis2D, 0.0f));
}

TEST(Basis3D, Mat4RoundTrip)
{
    Basis3D b = Basis3D::Build(Vec3F(1, 2, 3), Vec3F(2, 0.5f, 3), Quat::FromEuler(Vec3F(0.3f, -0.7f, 1.1f)),
                               Vec3F(0.2f, -0.1f, 0.3f));

    ExpectBasisNear(Basis3D(b.ToMat4()), b);
}

TEST(Basis3D, TranslateScaleRotateModifiers)
{
    Basis3D b;

    b.Translate(Vec3F(1, 2, 3));
    ExpectVecNear(b.origin, Vec3F(1, 2, 3));

    b.Scale(Vec3F(2, 3, 4));
    ExpectVecNear(b.xv, Vec3F(2, 0, 0));
    ExpectVecNear(b.yv, Vec3F(0, 3, 0));
    ExpectVecNear(b.zv, Vec3F(0, 0, 4));

    Quat rotation = Quat::FromAxisAngle(Vec3F::ZAxis(), Math::PI()*0.5f);
    b.Rotate(rotation);
    ExpectVecNear(b.xv, Vec3F(0, 2, 0));
    ExpectVecNear(b.yv, Vec3F(-3, 0, 0));
    ExpectVecNear(b.zv, Vec3F(0, 0, 4));
    ExpectVecNear(b.origin, Vec3F(1, 2, 3));

    ExpectBasisNear(Basis3D::Translated(Vec3F(5, 6, 7)), Basis3D(Vec3F(5, 6, 7)));
    ExpectVecNear(Basis3D::Scaled(Vec3F(2, 3, 4))*Vec3F(1, 1, 1), Vec3F(2, 3, 4));
    ExpectVecNear(Basis3D::Rotated(rotation)*Vec3F(1, 0, 0), Vec3F(0, 1, 0));
}
