#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Pipeline/DeferredPasses.h"
#include "o2/Render/Pipeline/Pipelines.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Properties/Objects/Components/ParticlesEmitterComponentViewer.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<ParticlesEmitterComponent> MakePlayingEmitterActor(const String& name)
    {
        auto actor = MakeActor();
        actor->SetName(name);

        auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
        emitter->SetParticlesPerSecond(100.0f);
        emitter->SetMaxParticles(100);
        emitter->SetParticlesLifetime(5.0f);
        emitter->SetEmissionDuration(5.0f);
        emitter->SetLoop(Loop::Repeat);
        emitter->Play();

        TickScene();
        return emitter;
    }

    void PumpSceneScreen(int frames)
    {
        for (int i = 0; i < frames; i++)
            o2EditorSceneScreen.Update(0.1f);
    }
}

// Emitters must stay frozen in edit time while they are not selected
TEST(ParticlesEditorPreview, EmittersAreNotUpdatedWithoutSelection)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    auto emitter = MakePlayingEmitterActor("unselected emitter");

    PumpSceneScreen(10);

    EXPECT_EQ(emitter->GetParticlesCount(), 0) << "unselected emitter must not emit particles in edit time";
}

// Selecting the emitter enables its component viewer, which drives the particles preview
TEST(ParticlesEditorPreview, SelectedEmitterViewerDrivesParticlesPreview)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    auto emitter = MakePlayingEmitterActor("selected emitter");

    auto viewer = mmake<ParticlesEmitterComponentViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);
    viewer->Refresh({ { dynamic_cast<IObject*>(emitter.Get()), nullptr } });

    Ref<IObjectPropertiesViewer> baseViewer = viewer;
    baseViewer->OnPropertiesEnabled();

    PumpSceneScreen(10);
    EXPECT_GT(emitter->GetParticlesCount(), 0) << "selected emitter viewer must update particles";

    baseViewer->OnPropertiesDisabled();

    int frozenCount = emitter->GetParticlesCount();
    float frozenTime = emitter->GetTime();
    PumpSceneScreen(5);
    EXPECT_EQ(emitter->GetParticlesCount(), frozenCount) << "deselected emitter must stop updating";
    EXPECT_EQ(emitter->GetTime(), frozenTime);
}

// Properties refresh writes values back through setters, it must not inflate looped emitter duration
TEST(ParticlesEditorPreview, PropertiesRefreshDoesNotGrowDuration)
{
    SceneCleanGuard guard;

    auto emitter = MakePlayingEmitterActor("looped emitter");
    emitter->SetEmissionDuration(10.0f);
    emitter->SetParticlesLifetime(1.6f);

    auto viewer = mmake<ParticlesEmitterComponentViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);

    Vector<Pair<IObject*, IObject*>> targets = { { dynamic_cast<IObject*>(emitter.Get()), nullptr } };
    for (int i = 0; i < 5; i++)
        viewer->Refresh(targets);

    EXPECT_FLOAT_EQ(emitter->GetDuration(), 11.6f) << "properties refresh must not change emitter duration";
    EXPECT_FLOAT_EQ(emitter->GetParticlesLifetime(), 1.6f);
}

// 3D particles of the selected emitter must spread and move in the editor preview while properties refresh
TEST(ParticlesEditorPreview, Selected3DEmitterParticlesSpreadInPreview)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    auto actor = MakeActor();
    actor->SetName("3d sparks");
    actor->transform->SetPosition(Vec3F(-30.0f, 40.0f, 130.0f));
    actor->transform->SetScale(Vec3F(40.0f, 40.0f, 40.0f));

    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    emitter->SetIs3D(true);
    emitter->SetEmitParticlesMoveDirection3D(Vec3F(0, 0, 1));
    emitter->SetEmitParticlesMoveDirectionRange(50.0f);
    emitter->SetInitialSpeed(350.0f);
    emitter->SetInitialSpeedRange(150.0f);
    emitter->SetParticlesPerSecond(80.0f);
    emitter->SetMaxParticles(300);
    emitter->SetParticlesLifetime(1.6f);
    emitter->SetEmissionDuration(10.0f);
    emitter->SetLoop(Loop::Repeat);
    emitter->Play();
    TickScene();

    auto viewer = mmake<ParticlesEmitterComponentViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);

    Vector<Pair<IObject*, IObject*>> targets = { { dynamic_cast<IObject*>(emitter.Get()), nullptr } };
    viewer->Refresh(targets);

    Ref<IObjectPropertiesViewer> baseViewer = viewer;
    baseViewer->OnPropertiesEnabled();

    // The editor refreshes properties and re-sends the actor transform every frame
    for (int i = 0; i < 10; i++)
    {
        viewer->Refresh(targets);
        actor->transform->SetPosition(actor->transform->GetPosition());
        TickScene();
        o2EditorSceneScreen.Update(0.1f);
    }

    ASSERT_GT(emitter->GetParticlesCount(), 0);

    o2::AABB bounds;
    ASSERT_TRUE(emitter->GetParticlesBounds(bounds));
    Vec3F size = bounds.max - bounds.min;
    EXPECT_GT(Math::Max(size.x, Math::Max(size.y, size.z)), 50.0f)
        << "3D particles must spread from the emission point, bounds: "
        << size.x << " " << size.y << " " << size.z;

    auto before = emitter->GetParticles();
    viewer->Refresh(targets);
    actor->transform->SetPosition(actor->transform->GetPosition());
    TickScene();
    o2EditorSceneScreen.Update(0.1f);

    float maxShift = 0.0f;
    auto& after = emitter->GetParticles();
    for (int i = 0; i < before.Count() && i < after.Count(); i++)
    {
        if (before[i].alive && after[i].alive)
            maxShift = Math::Max(maxShift, (after[i].position - before[i].position).Length());
    }

    EXPECT_GT(maxShift, 1.0f) << "alive 3D particles must keep moving in the editor preview";

    baseViewer->OnPropertiesDisabled();
}

// The 2D scene view draws through the scene camera render pipeline, like the 3D mode:
// executing the deferred pipeline creates its G-buffer targets. The edit view runs its
// own clone of the pipeline, so the scene camera instance itself must stay untouched
TEST(ParticlesEditorPreview, TwoDModeDrawsSceneThroughPipeline)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();
    screen.SetView3DMode(false);

    auto camera = mmake<CameraActor>();
    auto pipeline = mmake<DeferredPipeline>();
    camera->SetRenderPipeline(pipeline);
    TickScene();

    auto gBufferPass = pipeline->GetPass<GBufferPass>();
    ASSERT_NE(gBufferPass, nullptr);
    EXPECT_FALSE(gBufferPass->GetAlbedoTarget());

    screen.NeedRedraw();
    screen.Draw();

    auto editPipeline = DynamicCast<DeferredPipeline>(screen.GetEditRenderPipeline());
    ASSERT_TRUE(editPipeline) << "the edit view must clone the scene camera pipeline";
    auto editGBufferPass = editPipeline->GetPass<GBufferPass>();
    ASSERT_NE(editGBufferPass, nullptr);
    EXPECT_TRUE(editGBufferPass->GetAlbedoTarget()) << "2D scene view must render through the scene camera pipeline";

    // the scene camera pipeline instance is not executed by the edit view: sharing it with
    // the Game window would clobber the deferred pass state mid-frame
    EXPECT_FALSE(gBufferPass->GetAlbedoTarget());
}
