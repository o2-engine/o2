#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Math/Math.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(CameraActor, PerspectiveTypeSerializationRoundTrip)
{
    SceneCleanGuard guard;

    auto source = mmake<CameraActor>();
    source->SetPerspective(Math::Deg2rad(45.0f), 1.0f, 500.0f);

    DataDocument data;
    source->Serialize(data);

    auto restored = mmake<CameraActor>();
    restored->Deserialize(data);

    EXPECT_EQ(restored->GetCameraType(), CameraActor::Type::Perspective);
    EXPECT_NEAR(restored->GetFov(), Math::Deg2rad(45.0f), 0.0001f);
    EXPECT_NEAR(restored->GetNearClip(), 1.0f, 0.0001f);
    EXPECT_NEAR(restored->GetFarClip(), 500.0f, 0.0001f);
}

TEST(CameraActor, PerspectiveRenderCameraUsesWorld3DTransform)
{
    SceneCleanGuard guard;

    auto cameraActor = mmake<CameraActor>();
    cameraActor->SetPerspective(Math::Deg2rad(60.0f), 0.5f, 200.0f);
    cameraActor->transform->SetPosition(Vec3F(10.0f, 20.0f, 30.0f));
    TickFrame();

    Camera camera = cameraActor->GetRenderCamera();

    EXPECT_EQ(camera.projection, Camera::Projection::Perspective);
    EXPECT_NEAR(camera.fov, Math::Deg2rad(60.0f), 0.0001f);
    EXPECT_NEAR(camera.nearClip, 0.5f, 0.0001f);
    EXPECT_NEAR(camera.farClip, 200.0f, 0.0001f);

    EXPECT_NEAR(camera.GetPosition().x, 10.0f, 0.001f);
    EXPECT_NEAR(camera.GetPosition().y, 20.0f, 0.001f);
    EXPECT_NEAR(camera.GetPosition().z, 30.0f, 0.001f);
}
