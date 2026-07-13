#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/SkinnedMeshComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<Bitmap> RenderSceneFrame(const Ref<CameraActor>& camera)
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        camera->SetupAndDraw();
        o2Render.End();

        return captured;
    }

    const UInt8* GetPixel(const Ref<Bitmap>& bitmap, int x, int y)
    {
        Vec2I size = bitmap->GetSize();
        return bitmap->GetData() + (y*size.x + x)*4;
    }

    int CountDifferentPixels(const Ref<Bitmap>& a, const Ref<Bitmap>& b)
    {
        Vec2I size = a->GetSize();
        int count = 0;
        for (int y = 0; y < size.y; y += 4)
        {
            for (int x = 0; x < size.x; x += 4)
            {
                const UInt8* pa = GetPixel(a, x, y);
                const UInt8* pb = GetPixel(b, x, y);
                if (Math::Abs((int)pa[0] - pb[0]) + Math::Abs((int)pa[1] - pb[1]) +
                    Math::Abs((int)pa[2] - pb[2]) > 60)
                {
                    count++;
                }
            }
        }

        return count;
    }

    // Two-bone model: a 200x200 quad in the XY plane, top vertices bound to the child
    // joint. The "slide" clip moves the child (and the top edge) right by 300 over 1 second
    SkinnedModelData BuildTwoBoneQuadModel()
    {
        SkinnedModelData model;

        model.positions = { Vec3F(-100, -100, 0), Vec3F(-100, 100, 0), Vec3F(100, -100, 0), Vec3F(100, 100, 0) };
        model.normals = { Vec3F(0, 0, 1), Vec3F(0, 0, 1), Vec3F(0, 0, 1), Vec3F(0, 0, 1) };
        model.uvs = { Vec2F(0, 0), Vec2F(0, 1), Vec2F(1, 0), Vec2F(1, 1) };
        model.indices = { 0, 1, 2, 1, 3, 2 };

        SkinnedModelData::Node root;
        root.name = "root";

        SkinnedModelData::Node child;
        child.name = "child";
        child.parent = 0;
        child.position = Vec3F(0, 100, 0);

        model.nodes = { root, child };
        model.joints = { 0, 1 };

        Mat4 childInverseBind = Mat4::Translation(Vec3F(0, -100, 0));
        model.inverseBindMatrices = { Mat4::Identity(), childInverseBind };

        model.influences.Resize(4);
        for (int i = 0; i < 4; i++)
        {
            bool topVertex = model.positions[i].y > 0.0f;
            model.influences[i].joints[0] = topVertex ? 1 : 0;
            model.influences[i].weights[0] = 1.0f;
        }

        SkinnedModelData::AnimationChannel channel;
        channel.node = 1;
        channel.path = SkinnedModelData::AnimationChannel::Path::Translation;
        channel.times = { 0.0f, 1.0f };
        channel.vectors = { Vec3F(0, 100, 0), Vec3F(300, 100, 0) };

        SkinnedModelData::AnimationClip clip;
        clip.name = "slide";
        clip.duration = 1.0f;
        clip.channels.Add(channel);
        model.animations.Add(clip);

        return model;
    }

    struct SkinnedScene
    {
        Ref<CameraActor> camera;
        Ref<SkinnedMeshComponent> mesh;
    };

    SkinnedScene BuildSkinnedScene(bool withLight)
    {
        SkinnedScene scene;

        scene.camera = mmake<CameraActor>();
        scene.camera->SetPerspective(Math::Deg2rad(60.0f), 0.1f, 2000.0f);
        scene.camera->transform->SetPosition(Vec3F(0, 0, 500));
        scene.camera->fillColor = Color4::Black();

        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->SetName("skinned");
        scene.mesh = actor->AddComponent<SkinnedMeshComponent>();

        AssetRef<SkinnedModelAsset> modelRef;
        modelRef.CreateInstance();
        modelRef->SetModelData(BuildTwoBoneQuadModel());

        scene.mesh->SetModelAsset(modelRef);
        scene.mesh->SetColor(Color4::White());
        scene.mesh->SetShaded(false);
        scene.mesh->SetAnimation("slide");
        scene.mesh->SetPlaying(false);

        if (withLight)
        {
            auto lightActor = mmake<Actor>(ActorCreateMode::InScene);
            auto light = lightActor->AddComponent<LightComponent>();
            light->SetLightType(LightComponent::Type::Directional);
            light->SetColor(Color4::White());
            light->SetIntensity(1.0f);
        }

        return scene;
    }
}

// Skinned mesh must be drawn by the forward pipeline
TEST(SkinnedMeshDraw, SkinnedMeshDrawsInForwardPass)
{
    SceneCleanGuard guard;
    auto scene = BuildSkinnedScene(false);
    TickFrame();

    auto captured = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    EXPECT_GT((int)center[1], 200) << "skinned quad must be visible at screen center";
}

// Advancing the animation time must move the skinned vertices and change the image
TEST(SkinnedMeshDraw, AnimationChangesImage)
{
    SceneCleanGuard guard;
    auto scene = BuildSkinnedScene(false);
    TickFrame();

    scene.mesh->SetAnimationTime(0.0f);
    auto frameStart = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(frameStart);

    scene.mesh->SetAnimationTime(1.0f);
    auto frameEnd = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(frameEnd);

    int different = CountDifferentPixels(frameStart, frameEnd);
    EXPECT_GT(different, 50) << "animation must visibly move the skinned geometry, changed samples: " << different;

    // The frame with no time change stays identical
    auto frameEndAgain = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(frameEndAgain);
    EXPECT_LT(CountDifferentPixels(frameEnd, frameEndAgain), 5);
}

// GPU skinning must match the CPU reference skinning on the same pose
TEST(SkinnedMeshDraw, GPUSkinnedImageMatchesCPUReference)
{
    SceneCleanGuard guard;
    auto scene = BuildSkinnedScene(false);
    scene.mesh->SetShaded(true);
    scene.mesh->SetAnimationTime(0.5f);
    TickFrame();

    ASSERT_TRUE(scene.mesh->IsGPUSkinningEnabled());
    auto gpuFrame = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(gpuFrame);

    scene.mesh->SetGPUSkinningEnabled(false);
    auto cpuFrame = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(cpuFrame);

    int different = CountDifferentPixels(gpuFrame, cpuFrame);
    EXPECT_LT(different, 8) << "GPU and CPU skinning must produce the same image, changed samples: " << different;

    // Same scene sanity: the mesh is actually visible in both frames
    Vec2I size = gpuFrame->GetSize();
    EXPECT_GT((int)GetPixel(gpuFrame, size.x/2, size.y/2)[1], 100);
    EXPECT_GT((int)GetPixel(cpuFrame, size.x/2, size.y/2)[1], 100);
}

// Moving a bone actor must visibly change the GPU skinned image
TEST(SkinnedMeshDraw, BoneActorsDriveGPUPose)
{
    SceneCleanGuard guard;
    auto scene = BuildSkinnedScene(false);
    scene.mesh->CreateBoneActors();
    TickFrame();

    ASSERT_TRUE(scene.mesh->IsUsingBoneActors());

    auto bindFrame = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(bindFrame);

    auto childBone = scene.mesh->GetActor()->GetChild("root/child");
    ASSERT_TRUE(childBone);
    childBone->transform->SetPosition(Vec3F(250, 100, 0));
    TickFrame();

    auto posedFrame = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(posedFrame);

    int different = CountDifferentPixels(bindFrame, posedFrame);
    EXPECT_GT(different, 50) << "moved bone actor must visibly change the image, changed samples: " << different;
}

// Deferred pipeline must light the skinned mesh like other 3D content
TEST(SkinnedMeshDraw, DeferredPipelineLightsSkinnedMesh)
{
    if (!o2Render.IsMRTSupported())
        GTEST_SKIP() << "deferred pipeline requires MRT support";

    SceneCleanGuard guard;
    auto scene = BuildSkinnedScene(true);
    scene.camera->SetRenderPipeline(mmake<DeferredPipeline>());
    scene.mesh->SetShaded(true);
    TickFrame();

    auto captured = RenderSceneFrame(scene.camera);
    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    const UInt8* center = GetPixel(captured, size.x/2, size.y/2);
    int brightness = (int)center[0] + center[1] + center[2];
    EXPECT_GT(brightness, 60) << "deferred pipeline must render and light the skinned mesh";

    const UInt8* corner = GetPixel(captured, 5, 5);
    int cornerBrightness = (int)corner[0] + corner[1] + corner[2];
    EXPECT_LT(cornerBrightness, 60) << "background must stay dark";
}
