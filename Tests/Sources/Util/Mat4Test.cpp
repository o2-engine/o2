#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Matrix4.h"

using namespace o2;

static void ExpectVecNear(const Vec3F& a, const Vec3F& b, float tolerance = 1e-5f)
{
    EXPECT_NEAR(a.x, b.x, tolerance);
    EXPECT_NEAR(a.y, b.y, tolerance);
    EXPECT_NEAR(a.z, b.z, tolerance);
}

static void ExpectMatNear(const Mat4& a, const Mat4& b, float tolerance = 1e-5f)
{
    for (int i = 0; i < 16; i++)
        EXPECT_NEAR(a.m[i], b.m[i], tolerance) << "element " << i;
}

TEST(Mat4, Identity) {
    Mat4 m;
    Vec3F v(1.0f, 2.0f, 3.0f);
    ExpectVecNear(m.TransformPoint(v), v);
    ExpectVecNear(m.TransformDirection(v), v);
    ExpectMatNear(m, Mat4::Identity());
}

TEST(Mat4, Translation) {
    Mat4 m = Mat4::Translation(Vec3F(10.0f, -5.0f, 2.0f));
    ExpectVecNear(m.TransformPoint(Vec3F(1.0f, 1.0f, 1.0f)), Vec3F(11.0f, -4.0f, 3.0f));
    ExpectVecNear(m.TransformDirection(Vec3F(1.0f, 1.0f, 1.0f)), Vec3F(1.0f, 1.0f, 1.0f));
}

TEST(Mat4, RotationAndScale) {
    Mat4 rot = Mat4::Rotation(Quat::FromAxisAngle(Vec3F::ZAxis(), Math::PI()*0.5f));
    ExpectVecNear(rot.TransformPoint(Vec3F::XAxis()), Vec3F::YAxis());

    Mat4 scale = Mat4::Scaling(Vec3F(2.0f, 3.0f, 4.0f));
    ExpectVecNear(scale.TransformPoint(Vec3F(1.0f, 1.0f, 1.0f)), Vec3F(2.0f, 3.0f, 4.0f));
}

TEST(Mat4, RotationMatchesQuaternion) {
    Quat q = Quat::FromEuler(Vec3F(0.4f, -0.7f, 1.3f));
    Mat4 m = Mat4::Rotation(q);
    Vec3F v(1.0f, 2.0f, 3.0f);
    ExpectVecNear(m.TransformPoint(v), q*v);
}

TEST(Mat4, TRSEqualsSequential) {
    Vec3F pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::FromEuler(Vec3F(0.2f, 0.5f, -0.9f));
    Vec3F scale(2.0f, 0.5f, 3.0f);

    Mat4 trs = Mat4::TRS(pos, rot, scale);
    Mat4 sequential = Mat4::Translation(pos)*Mat4::Rotation(rot)*Mat4::Scaling(scale);
    ExpectMatNear(trs, sequential);

    Vec3F v(1.0f, -1.0f, 2.0f);
    Vec3F expected = rot*(scale*v) + pos;
    ExpectVecNear(trs.TransformPoint(v), expected, 1e-4f);
}

TEST(Mat4, MulConsistencyWithMtxMultiply) {
    Mat4 a = Mat4::TRS(Vec3F(1.0f, 2.0f, 3.0f), Quat::FromEuler(Vec3F(0.3f, 0.6f, 0.9f)), Vec3F(1.5f, 2.0f, 0.5f));
    Mat4 b = Mat4::Perspective(Math::Deg2rad(60.0f), 1.5f, 0.1f, 100.0f);

    Mat4 mul = a*b;

    float expected[16];
    Math::mtxMultiply(expected, a.Data(), b.Data());

    for (int i = 0; i < 16; i++)
        EXPECT_NEAR(mul.m[i], expected[i], 1e-5f) << "element " << i;
}

TEST(Mat4, Inverted) {
    Mat4 m = Mat4::TRS(Vec3F(5.0f, -2.0f, 1.0f), Quat::FromEuler(Vec3F(0.7f, 0.1f, -0.4f)), Vec3F(2.0f, 3.0f, 0.5f));
    ExpectMatNear(m*m.Inverted(), Mat4::Identity(), 1e-4f);

    Vec3F v(1.0f, 2.0f, 3.0f);
    ExpectVecNear(m.Inverted().TransformPoint(m.TransformPoint(v)), v, 1e-4f);
}

TEST(Mat4, OrthoMatchesMathOrthoProjMatrix) {
    Mat4 m = Mat4::Ortho(-10.0f, 30.0f, -5.0f, 15.0f, 0.1f, 100.0f);

    float expected[16];
    Math::OrthoProjMatrix(expected, -10.0f, 30.0f, -5.0f, 15.0f, 0.1f, 100.0f);

    for (int i = 0; i < 16; i++)
        EXPECT_FLOAT_EQ(m.m[i], expected[i]) << "element " << i;
}

TEST(Mat4, PerspectiveKnownValues) {
    float nearz = 1.0f, farz = 10.0f;
    Mat4 m = Mat4::Perspective(Math::PI()*0.5f, 1.0f, nearz, farz);

    EXPECT_NEAR(m.m[0], 1.0f, 1e-5f);
    EXPECT_NEAR(m.m[5], 1.0f, 1e-5f);
    EXPECT_NEAR(m.m[10], (farz + nearz)/(nearz - farz), 1e-5f);
    EXPECT_NEAR(m.m[14], 2.0f*farz*nearz/(nearz - farz), 1e-5f);
    EXPECT_NEAR(m.m[11], -1.0f, 1e-5f);
    EXPECT_NEAR(m.m[15], 0.0f, 1e-5f);

    ExpectVecNear(m.TransformPoint(Vec3F(0.0f, 0.0f, -nearz)), Vec3F(0.0f, 0.0f, -1.0f), 1e-4f);
    ExpectVecNear(m.TransformPoint(Vec3F(0.0f, 0.0f, -farz)), Vec3F(0.0f, 0.0f, 1.0f), 1e-4f);
}

TEST(Mat4, LookAt) {
    Mat4 view = Mat4::LookAt(Vec3F(0.0f, 0.0f, 5.0f), Vec3F::Zero(), Vec3F::YAxis());

    ExpectVecNear(view.TransformPoint(Vec3F(0.0f, 0.0f, 5.0f)), Vec3F::Zero());
    ExpectVecNear(view.TransformPoint(Vec3F::Zero()), Vec3F(0.0f, 0.0f, -5.0f));
    ExpectVecNear(view.TransformPoint(Vec3F(1.0f, 0.0f, 5.0f)), Vec3F(1.0f, 0.0f, 0.0f));
    ExpectVecNear(view.TransformPoint(Vec3F(0.0f, 1.0f, 5.0f)), Vec3F(0.0f, 1.0f, 0.0f));
}

TEST(Mat4, DecomposeRoundTrip) {
    Vec3F pos(3.0f, -1.0f, 7.0f);
    Quat rot = Quat::FromEuler(Vec3F(0.3f, 0.5f, -0.8f));
    Vec3F scale(2.0f, 0.5f, 1.5f);

    Vec3F outPos, outScale;
    Quat outRot;
    Mat4::TRS(pos, rot, scale).Decompose(outPos, outRot, outScale);

    ExpectVecNear(outPos, pos, 1e-4f);
    ExpectVecNear(outScale, scale, 1e-4f);

    if (outRot.Dot(rot) < 0.0f)
        outRot = Quat(-outRot.x, -outRot.y, -outRot.z, -outRot.w);

    EXPECT_TRUE(outRot == rot);
}

TEST(Mat4, ConsistencyWithVec2Rotate) {
    float angle = 0.7f;
    Vec2F v2(3.0f, -2.0f);

    Mat4 m = Mat4::Rotation(Quat::FromEuler(Vec3F(0.0f, 0.0f, angle)));
    Vec2F rotated3d = m.TransformPoint(Vec3F(v2)).XY();

    Vec2F rotated2d = v2.Rotate(angle);
    EXPECT_NEAR(rotated3d.x, rotated2d.x, 1e-4f);
    EXPECT_NEAR(rotated3d.y, rotated2d.y, 1e-4f);

    Vec2F rotatedBasis = Basis::Rotated(angle).Transform(v2);
    EXPECT_NEAR(rotated3d.x, rotatedBasis.x, 1e-4f);
    EXPECT_NEAR(rotated3d.y, rotatedBasis.y, 1e-4f);
}
