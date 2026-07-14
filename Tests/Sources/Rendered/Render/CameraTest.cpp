#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Matrix4.h"

using namespace o2;

TEST(Camera, DefaultUsesCurrentResolution)
{
    Vec2F resolution = (Vec2F)o2Render.GetCurrentResolution();

    Camera cam = Camera::Default();
    EXPECT_FLOAT_EQ(cam.GetSize().x, resolution.x);
    EXPECT_FLOAT_EQ(cam.GetSize().y, resolution.y);
    EXPECT_EQ(cam.GetPosition(), Vec2F());
    EXPECT_FLOAT_EQ(cam.GetAngle(), 0.0f);
}

TEST(Camera, FixedSizeHonorsArgumentRegardlessOfResolution)
{
    Camera cam = Camera::FixedSize(Vec2F(800, 600));
    EXPECT_EQ(cam.GetSize(), Vec2F(800, 600));
}

TEST(Camera, FittedSizeFitsBothAxes)
{
    Vec2F resolution = (Vec2F)o2Render.GetCurrentResolution();
    if (resolution.x <= 0.0f || resolution.y <= 0.0f)
        GTEST_SKIP() << "Render resolution not initialized in test environment";

    Vec2F target(1000.0f, 100.0f);

    Vec2F expected = resolution * (target.x / resolution.x);
    if (expected.y < target.y)
        expected = resolution * (target.y / resolution.y);

    Camera cam = Camera::FittedSize(target);

    EXPECT_NEAR(cam.GetSize().x, expected.x, 0.01f);
    EXPECT_NEAR(cam.GetSize().y, expected.y, 0.01f);

    EXPECT_GE(cam.GetSize().x + 0.01f, target.x);
    EXPECT_GE(cam.GetSize().y + 0.01f, target.y);
}

TEST(Camera, PhysicalCorrectPixelsEqualsResolution)
{
    Vec2F resolution = (Vec2F)o2Render.GetCurrentResolution();

    Camera cam = Camera::PhysicalCorrect(Units::Pixels);
    EXPECT_FLOAT_EQ(cam.GetSize().x, resolution.x);
    EXPECT_FLOAT_EQ(cam.GetSize().y, resolution.y);
}

TEST(Camera, PhysicalCorrectInchesEqualsResolutionOverDpi)
{
    Vec2F resolution = (Vec2F)o2Render.GetCurrentResolution();
    Vec2F dpi = (Vec2F)o2Render.GetDPI();
    ASSERT_GT(dpi.x, 0.0f);
    ASSERT_GT(dpi.y, 0.0f);

    Camera cam = Camera::PhysicalCorrect(Units::Inches);
    EXPECT_NEAR(cam.GetSize().x, resolution.x / dpi.x, 0.001f);
    EXPECT_NEAR(cam.GetSize().y, resolution.y / dpi.y, 0.001f);
}

TEST(Camera, PhysicalCorrectUnitsScaleProportionally)
{
    Vec2F resolution = (Vec2F)o2Render.GetCurrentResolution();
    if (resolution.x <= 0.0f)
        GTEST_SKIP() << "Render resolution not initialized in test environment";

    Camera inches = Camera::PhysicalCorrect(Units::Inches);
    Camera centimeters = Camera::PhysicalCorrect(Units::Centimeters);
    Camera millimeters = Camera::PhysicalCorrect(Units::Millimeters);

    ASSERT_GT(inches.GetSize().x, 0.0f);
    ASSERT_GT(centimeters.GetSize().x, 0.0f);
    ASSERT_GT(millimeters.GetSize().x, 0.0f);

    EXPECT_NEAR(centimeters.GetSize().x / inches.GetSize().x, 2.54f, 0.01f);
    EXPECT_NEAR(millimeters.GetSize().x / centimeters.GetSize().x, 10.0f, 0.01f);
}

TEST(Camera, EqualityBasedOnTransform)
{
    Camera a(Vec2F(10, 20), Vec2F(800, 600), 0.5f);
    Camera b(Vec2F(10, 20), Vec2F(800, 600), 0.5f);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    Camera diffPos(Vec2F(11, 20), Vec2F(800, 600), 0.5f);
    EXPECT_TRUE(a != diffPos);

    Camera diffSize(Vec2F(10, 20), Vec2F(800, 700), 0.5f);
    EXPECT_TRUE(a != diffSize);

    Camera diffAngle(Vec2F(10, 20), Vec2F(800, 600), 1.5f);
    EXPECT_TRUE(a != diffAngle);
}

TEST(Camera, OrthoProjectionMatrixMatchesRenderValues)
{
    Camera cam;
    Mat4 proj = cam.GetProjectionMatrix(Vec2F(800, 600));

    float expected[16];
    Math::OrthoProjMatrix(expected, 0.0f, 800.0f, 600.0f, 0.0f, -Camera::ortho2DHalfDepth, Camera::ortho2DHalfDepth);

    for (int i = 0; i < 16; i++)
        EXPECT_NEAR(proj.m[i], expected[i], 0.0001f) << "element " << i;
}

TEST(Camera, PerspectiveProjectionMatrixMatchesMath)
{
    Camera cam = Camera::Perspective(Math::Deg2rad(60.0f), 0.5f, 500.0f);
    EXPECT_EQ(cam.projection, Camera::Projection::Perspective);

    Mat4 proj = cam.GetProjectionMatrix(Vec2F(800, 600));

    float expected[16];
    Math::PerspectiveProjMatrix(expected, Math::Deg2rad(60.0f), 800.0f/600.0f, 0.5f, 500.0f);

    for (int i = 0; i < 16; i++)
        EXPECT_NEAR(proj.m[i], expected[i], 0.0001f) << "element " << i;
}

TEST(Camera, ViewMatrix3DIsInverseOfPositionRotationTransform)
{
    Camera cam = Camera::Perspective(Math::Deg2rad(60.0f), 0.1f, 1000.0f);
    cam.position = Vec3F(10.0f, -5.0f, 30.0f);
    cam.rotation = Quat::FromEuler(Vec3F(0.3f, 1.1f, -0.4f));

    Mat4 view = cam.GetViewMatrix3D();
    Mat4 trs = Mat4::TRS(cam.GetPosition(), cam.GetRotation(), Vec3F(1.0f, 1.0f, 1.0f));
    Mat4 identity = view*trs;

    for (int i = 0; i < 16; i++)
        EXPECT_NEAR(identity.m[i], Mat4::Identity().m[i], 0.001f) << "element " << i;
}

TEST(Camera, EqualityDetectsProjectionFields)
{
    Camera a;
    Camera b;
    EXPECT_TRUE(a == b);

    b.projection = Camera::Projection::Perspective;
    EXPECT_TRUE(a != b);

    b = a;
    b.fov = Math::Deg2rad(90.0f);
    EXPECT_TRUE(a != b);

    b = a;
    b.nearClip = 5.0f;
    EXPECT_TRUE(a != b);

    b = a;
    b.farClip = 100.0f;
    EXPECT_TRUE(a != b);

    b = a;
    b.position = Vec3F(1.0f, 2.0f, 3.0f);
    EXPECT_TRUE(a != b);

    b = a;
    b.rotation = Quat::FromEuler(Vec3F(0.0f, 0.5f, 0.0f));
    EXPECT_TRUE(a != b);
}
