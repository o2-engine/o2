#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Scene/Scene3DTestHelpers.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

TEST(Mesh3DDraw, Test3DSceneDrawSmoke)
{
    SceneCleanGuard guard;
    auto scene = BuildTest3DScene();
    TickFrame();

    o2Render.Begin();
    scene.camera->SetupAndDraw();
    o2Render.End();

    EXPECT_GT(scene.box1->GetComponent<MeshPrimitiveComponent>()->GetMesh().vertexCount, 0u);
}

TEST(Mesh3DDraw, NearBoxOccludesFarPlaneAtCenter)
{
    SceneCleanGuard guard;

    auto camera = mmake<CameraActor>();
    camera->SetName("camera");
    camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
    camera->transform->SetPosition(Vec3F(0, 0, 500));
    camera->fillColor = Color4::Black();

    // Far red plane fills the view behind a near green box; draw order is by actors order,
    // so the box is drawn first and depth test must keep it visible
    auto box = mmake<Actor>(ActorCreateMode::InScene);
    box->SetName("nearBox");
    auto boxComponent = box->AddComponent<MeshPrimitiveComponent>();
    boxComponent->SetPrimitiveType(PrimitiveType3D::Box);
    boxComponent->SetSize(Vec3F(100, 100, 100));
    boxComponent->SetColor(Color4::Green());
    boxComponent->SetShaded(false);
    box->transform->SetPosition(Vec3F(0, 0, 100));

    auto plane = mmake<Actor>(ActorCreateMode::InScene);
    plane->SetName("farPlane");
    auto planeComponent = plane->AddComponent<MeshPrimitiveComponent>();
    planeComponent->SetPrimitiveType(PrimitiveType3D::Plane);
    planeComponent->SetSize(Vec3F(4000, 4000, 0));
    planeComponent->SetColor(Color4::Red());
    planeComponent->SetShaded(false);
    plane->transform->SetPosition(Vec3F(0, 0, -300));

    TickFrame();

    Ref<Bitmap> captured;
    o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

    o2Render.Begin();
    camera->SetupAndDraw();
    o2Render.End();

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    ASSERT_GT(size.x, 0);
    ASSERT_GT(size.y, 0);

    const UInt8* data = captured->GetData();

    // Near green box must win at center: green channel is byte 1 in both RGBA and BGRA layouts
    int centerOffset = ((size.y/2)*size.x + size.x/2)*4;
    EXPECT_GT((int)data[centerOffset + 1], 200);

    // Far red plane must win at corner area not covered by the box (red, so green stays low)
    int cornerOffset = ((size.y/8)*size.x + size.x/8)*4;
    EXPECT_LT((int)data[cornerOffset + 1], 60);
}

// Reproduces the editor crash: a mesh far above the batch buffers capacity drawn in a scene
TEST(Mesh3DDraw, OversizedObjMeshSceneDrawSmoke)
{
    namespace fs = std::filesystem;

    fs::path objPath;
    fs::path dir = fs::current_path();
    for (int i = 0; i < 8 && !dir.empty(); i++, dir = dir.parent_path())
    {
        auto candidate = dir/"Assets"/"12221_Cat_v1_l3.obj";
        if (fs::exists(candidate))
        {
            objPath = candidate;
            break;
        }
    }

    if (objPath.empty())
        GTEST_SKIP() << "project cat obj not found";

    std::ifstream file(objPath);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    SceneCleanGuard guard;
    auto scene = BuildTest3DScene();

    AssetRef<Mesh3DAsset> asset;
    asset.CreateInstance();
    ASSERT_TRUE(asset->LoadFromObj(String(text.c_str())));

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    actor->transform->eulerAngles = Vec3F(Math::Deg2rad(-90.0f), 0.0f, 0.0f);
    auto component = actor->AddComponent<Mesh3DComponent>();
    component->SetMeshAsset(asset);
    TickFrame();

    o2Render.Begin();
    scene.camera->SetupAndDraw();
    o2Render.End();

    EXPECT_GT(component->GetMesh().vertexCount, 35000u);
}
