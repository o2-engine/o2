#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

// Locks the on-disk format of the math Transform (Sprite, Text, Camera bases): flat Vec3F members
// named mPosition/mSize/mScale/mPivot/mEulerAngles/mShear. Legacy pre-3D payloads (2D vectors,
// float mAngle and mShear, camera position3D/rotation3D) must still load.

namespace
{
    Vec3F GetVec3(const DataValue& node, const char* name)
    {
        auto member = node.FindMember(name);
        EXPECT_TRUE(member != nullptr) << "missing member " << name;
        if (!member)
            return Vec3F();

        Vec3F result;
        member->Get(result);
        return result;
    }
}

TEST(MathTransformSerialization, LegacySpriteFormatLoads)
{
    // Captured from the pre-3D Transform serialization of a Sprite
    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "mPosition": { "x": 10.0, "y": 20.0 },
        "mSize": { "x": 100.0, "y": 50.0 },
        "mScale": { "x": 2.0, "y": 3.0 },
        "mPivot": { "x": 0.3, "y": 0.7 },
        "mAngle": 0.5,
        "mShear": 0.1,
        "mTextureSrcRect": { "left": 0, "bottom": 0, "right": 0, "top": 0 }
    })"));

    Sprite sprite;
    doc.Get(sprite);

    EXPECT_EQ(sprite.GetPosition(), Vec3F(10, 20, 0));
    EXPECT_EQ(sprite.GetSize(), Vec3F(100, 50, 0));
    EXPECT_EQ(sprite.GetScale(), Vec3F(2, 3, 1));
    EXPECT_EQ(sprite.GetPivot(), Vec3F(0.3f, 0.7f, 0));
    EXPECT_FLOAT_EQ(sprite.GetAngle(), 0.5f);
    EXPECT_EQ(sprite.GetEulerAngles(), Vec3F(0, 0, 0.5f));
    EXPECT_EQ(sprite.GetShear(), Vec3F(0.1f, 0, 0));
    EXPECT_FLOAT_EQ(sprite.GetShear2D(), 0.1f);
}

TEST(MathTransformSerialization, LegacyCameraFormatLoads)
{
    // Captured from the pre-unification Camera serialization: separate position3D/rotation3D
    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "mPosition": { "x": 5.0, "y": 6.0 },
        "mSize": { "x": 800.0, "y": 600.0 },
        "mScale": { "x": 1.0, "y": 1.0 },
        "mPivot": { "x": 0.5, "y": 0.5 },
        "projection": "Perspective",
        "position3D": { "x": 1.0, "y": 2.0, "z": 3.0 },
        "rotation3D": {
            "x": 0.03427079692482948,
            "y": 0.10602051764726639,
            "z": 0.14357219636440278,
            "w": 0.983347475528717
        }
    })"));

    Camera camera;
    doc.Get(camera);

    EXPECT_EQ(camera.projection, Camera::Projection::Perspective);
    EXPECT_EQ(camera.GetSize(), Vec3F(800, 600, 0));
    EXPECT_EQ(camera.GetPosition(), Vec3F(1, 2, 3));

    Quat expected = Quat::FromEuler(Vec3F(0.1f, 0.2f, 0.3f));
    Quat rotation = camera.GetRotation();
    EXPECT_NEAR(rotation.x, expected.x, 0.001f);
    EXPECT_NEAR(rotation.y, expected.y, 0.001f);
    EXPECT_NEAR(rotation.z, expected.z, 0.001f);
    EXPECT_NEAR(rotation.w, expected.w, 0.001f);
}

TEST(MathTransformSerialization, WritesChangedFieldsAsVec3)
{
    Sprite sprite;
    sprite.SetPosition(Vec3F(10, 20, 5));
    sprite.SetSize(Vec3F(100, 50, 30));
    sprite.SetScale(Vec3F(2, 3, 2));
    sprite.SetPivot(Vec3F(0.3f, 0.7f, 0.25f));
    sprite.SetEulerAngles(Vec3F(0.2f, 0.3f, 0.5f));
    sprite.SetShear(Vec3F(0.1f, 0.2f, 0.3f));

    DataDocument doc;
    doc.Set(sprite);

    EXPECT_EQ(GetVec3(doc, "mPosition"), Vec3F(10, 20, 5));
    EXPECT_EQ(GetVec3(doc, "mSize"), Vec3F(100, 50, 30));
    EXPECT_EQ(GetVec3(doc, "mScale"), Vec3F(2, 3, 2));
    EXPECT_EQ(GetVec3(doc, "mPivot"), Vec3F(0.3f, 0.7f, 0.25f));
    EXPECT_EQ(GetVec3(doc, "mEulerAngles"), Vec3F(0.2f, 0.3f, 0.5f));
    EXPECT_EQ(GetVec3(doc, "mShear"), Vec3F(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(doc.FindMember("mAngle"), nullptr);
}

TEST(MathTransformSerialization, SkipsDefaultValuedFields)
{
    Sprite sprite;
    sprite.SetSize(Vec2F());

    DataDocument doc;
    doc.Set(sprite);

    EXPECT_EQ(doc.FindMember("mPosition"), nullptr);
    EXPECT_EQ(doc.FindMember("mSize"), nullptr);
    EXPECT_EQ(doc.FindMember("mScale"), nullptr);
    EXPECT_EQ(doc.FindMember("mEulerAngles"), nullptr);
    EXPECT_EQ(doc.FindMember("mShear"), nullptr);

    EXPECT_EQ(GetVec3(doc, "mPivot"), Vec3F(0.5f, 0.5f, 0.0f));
}

TEST(MathTransformSerialization, RoundTrip)
{
    Sprite sprite;
    sprite.SetPosition(Vec3F(10, 20, 5));
    sprite.SetSize(Vec3F(100, 50, 30));
    sprite.SetScale(Vec3F(2, 3, 2));
    sprite.SetPivot(Vec3F(0.3f, 0.7f, 0.25f));
    sprite.SetEulerAngles(Vec3F(0.2f, 0.3f, 0.5f));
    sprite.SetShear(Vec3F(0.1f, 0.2f, 0.3f));

    DataDocument doc;
    doc.Set(sprite);

    Sprite restored;
    doc.Get(restored);

    EXPECT_EQ(restored.GetPosition(), Vec3F(10, 20, 5));
    EXPECT_EQ(restored.GetSize(), Vec3F(100, 50, 30));
    EXPECT_EQ(restored.GetScale(), Vec3F(2, 3, 2));
    EXPECT_EQ(restored.GetPivot(), Vec3F(0.3f, 0.7f, 0.25f));
    EXPECT_EQ(restored.GetEulerAngles(), Vec3F(0.2f, 0.3f, 0.5f));
    EXPECT_EQ(restored.GetShear(), Vec3F(0.1f, 0.2f, 0.3f));
}

TEST(MathTransformSerialization, DeltaWritesOnlyDifferences)
{
    Sprite origin;
    origin.SetPosition(Vec2F(10, 20));
    origin.SetSize(Vec2F(100, 50));
    origin.SetAngle(0.5f);

    Sprite changed(origin);
    changed.SetPosition2D(Vec2F(30, 40));

    DataDocument doc;
    changed.SerializeDelta(doc, origin);

    EXPECT_EQ(GetVec3(doc, "mPosition"), Vec3F(30, 40, 0));
    EXPECT_EQ(doc.FindMember("mSize"), nullptr);
    EXPECT_EQ(doc.FindMember("mEulerAngles"), nullptr);

    Sprite restored;
    restored.DeserializeDelta(doc, origin);

    EXPECT_EQ(restored.GetPosition(), Vec3F(30, 40, 0));
    EXPECT_EQ(restored.GetSize(), Vec3F(100, 50, 0));
    EXPECT_FLOAT_EQ(restored.GetAngle(), 0.5f);
}

TEST(MathTransformSerialization, LegacyDeltaFormatLoads)
{
    Sprite origin;
    origin.SetPosition(Vec3F(10, 20, 1));
    origin.SetSize(Vec2F(100, 50));
    origin.SetAngle(0.5f);

    DataDocument doc;
    ASSERT_TRUE(doc.LoadFromData(R"({
        "mPosition": { "x": 30.0, "y": 40.0 },
        "mAngle": 0.7
    })"));

    Sprite restored;
    restored.DeserializeDelta(doc, origin);

    EXPECT_EQ(restored.GetPosition2D(), Vec2F(30, 40));
    EXPECT_FLOAT_EQ(restored.GetPositionZ(), 1.0f); // z comes from origin, legacy vector has no z
    EXPECT_FLOAT_EQ(restored.GetAngle(), 0.7f);
    EXPECT_EQ(restored.GetSize2D(), Vec2F(100, 50));
}
