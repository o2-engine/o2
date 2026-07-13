#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Math/Vector3.h"

using namespace o2;

TEST(Vec3F, ConstructorAndAccess) {
    Vec3F v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);

    Vec3F zero;
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
    EXPECT_FLOAT_EQ(zero.z, 0.0f);

    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FLOAT_EQ(v[2], 3.0f);
}

TEST(Vec3F, Arithmetic) {
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);

    Vec3F sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    Vec3F diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 3.0f);
    EXPECT_FLOAT_EQ(diff.y, 3.0f);
    EXPECT_FLOAT_EQ(diff.z, 3.0f);

    Vec3F scaled = a*2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);

    Vec3F mul = a*b;
    EXPECT_FLOAT_EQ(mul.x, 4.0f);
    EXPECT_FLOAT_EQ(mul.y, 10.0f);
    EXPECT_FLOAT_EQ(mul.z, 18.0f);

    Vec3F div = b/a;
    EXPECT_FLOAT_EQ(div.x, 4.0f);
    EXPECT_FLOAT_EQ(div.y, 2.5f);
    EXPECT_FLOAT_EQ(div.z, 2.0f);

    Vec3F neg = -a;
    EXPECT_FLOAT_EQ(neg.x, -1.0f);
    EXPECT_FLOAT_EQ(neg.y, -2.0f);
    EXPECT_FLOAT_EQ(neg.z, -3.0f);

    Vec3F acc = a;
    acc += b;
    EXPECT_TRUE(acc == sum);
}

TEST(Vec3F, DotAndCross) {
    Vec3F x = Vec3F::XAxis();
    Vec3F y = Vec3F::YAxis();

    EXPECT_FLOAT_EQ(x.Dot(y), 0.0f);
    EXPECT_TRUE(x.Cross(y) == Vec3F::ZAxis());
    EXPECT_TRUE(y.Cross(x) == Vec3F::ZAxis()*-1.0f);

    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(a.Dot(b), 32.0f);

    Vec3F c = a.Cross(b);
    EXPECT_FLOAT_EQ(c.x, -3.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
    EXPECT_FLOAT_EQ(c.z, -3.0f);
}

TEST(Vec3F, LengthAndNormalize) {
    Vec3F v(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.Length(), 3.0f);
    EXPECT_FLOAT_EQ(v.SqrLength(), 9.0f);

    Vec3F n = v.Normalized();
    EXPECT_NEAR(n.Length(), 1.0f, 1e-5f);
    EXPECT_NEAR(n.x, 1.0f/3.0f, 1e-5f);

    Vec3F z;
    EXPECT_TRUE(z.Normalized() == Vec3F::Zero());
}

TEST(Vec3F, Lerp) {
    Vec3F a(0.0f, 0.0f, 0.0f);
    Vec3F b(2.0f, 4.0f, 6.0f);

    Vec3F mid = Vec3F::Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 1.0f);
    EXPECT_FLOAT_EQ(mid.y, 2.0f);
    EXPECT_FLOAT_EQ(mid.z, 3.0f);

    EXPECT_TRUE(Vec3F::Lerp(a, b, 0.0f) == a);
    EXPECT_TRUE(Vec3F::Lerp(a, b, 1.0f) == b);
}

TEST(Vec3F, Vec2Conversion) {
    Vec2F v2(1.0f, 2.0f);
    Vec3F v3(v2);
    EXPECT_FLOAT_EQ(v3.x, 1.0f);
    EXPECT_FLOAT_EQ(v3.y, 2.0f);
    EXPECT_FLOAT_EQ(v3.z, 0.0f);

    Vec3F v3z(v2, 5.0f);
    EXPECT_FLOAT_EQ(v3z.z, 5.0f);

    Vec2F back = v3z.XY();
    EXPECT_TRUE(back == v2);

    Vec3I vi = (Vec3I)Vec3F(1.5f, 2.5f, 3.5f);
    EXPECT_EQ(vi.x, 1);
    EXPECT_EQ(vi.y, 2);
    EXPECT_EQ(vi.z, 3);
}

TEST(Vec3F, InvertedAndStatics) {
    Vec3F v(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(v.InvertedX() == Vec3F(-1.0f, 2.0f, 3.0f));
    EXPECT_TRUE(v.InvertedY() == Vec3F(1.0f, -2.0f, 3.0f));
    EXPECT_TRUE(v.InvertedZ() == Vec3F(1.0f, 2.0f, -3.0f));
    EXPECT_TRUE(v.Inverted() == -v);

    EXPECT_TRUE(Vec3F::One() == Vec3F(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(Vec3F::Zero() == Vec3F());
}

TEST(Vec3F, AxisByIndex) {
    EXPECT_TRUE(Vec3F::Axis(0) == Vec3F::XAxis());
    EXPECT_TRUE(Vec3F::Axis(1) == Vec3F::YAxis());
    EXPECT_TRUE(Vec3F::Axis(2) == Vec3F::ZAxis());
}
