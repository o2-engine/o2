#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Render/Particles/ParticlesContainer.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/FlightTrajectoryComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/DockableWindow.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct AnimationWindowProbe: AnimationWindow
    {
        AnimationWindowProbe(RefCounter* refCounter): AnimationWindow(refCounter) {}

        const Ref<DockableWindow>& Window() const { return mWindow; }
        const Ref<AnimationTimeline>& Timeline() const { return mTimeline; }

        void SetClip(const Ref<AnimationClip>& clip, const Ref<AnimationPlayer>& player)
        {
            mAnimation = clip;
            mPreviewPlayer = player;
            mHandlesSheet->SetAnimation(clip);
            mTimeline->SetAnimation(clip, player);
            mTree->SetAnimation(clip);
        }
    };

    class CountingContainer : public ParticlesContainer
    {
    public:
        int lastAlive = -1;

        void Update(Vector<Particle>& particles, int maxParticles) override
        {
            lastAlive = particles.Count([](const Particle& p) { return p.alive; });
        }

        void Draw() override {}
    };

    class CountingSource : public ParticleSource
    {
    public:
        Ref<CountingContainer> container = mmake<CountingContainer>();

        Ref<ParticlesContainer> CreateContainer() override { return container; }
    };
}

// Scrubbing the timeline in edit mode (no scene update) must move a trajectory-driven actor and
// keep the drawn particles of a sub-track emitter in sync with the cursor
TEST(AnimationWindowScrubUI, TimelineCursorDrivesTrajectoryAndParticles)
{
    auto uiRoot = mmake<UIRoot>();

    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    auto trajectory = actor->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(200, 130), 0.0f);
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto sparks = mmake<Actor>(ActorCreateMode::NotInScene);
    sparks->SetName("Sparks");
    actor->AddChild(sparks);

    auto emitter = sparks->AddComponent<ParticlesEmitterComponent>();
    auto source = mmake<CountingSource>();
    emitter->SetParticlesSource(source);
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(0.5f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(100);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto clip = mmake<AnimationClip>();
    auto positionTrack = clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position");
    *positionTrack = AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);
    clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto player = mmake<AnimationPlayer>(actor.Get(), clip);
    ASSERT_TRUE(emitter->IsSubControlled());

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->SetClip(clip, player);
        window->Window()->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.5f);
        float expectedPosition = positionTrack->GetValue(0.5f);
        EXPECT_GT(expectedPosition, 0.05f);
        EXPECT_LT(expectedPosition, 0.95f);
        EXPECT_NEAR(trajectory->GetPosition(), expectedPosition, 0.01f);

        Vec2F expectedPoint = trajectory->EvaluatePoint(expectedPosition);
        EXPECT_NEAR(actor->transform->GetPosition2D().x, expectedPoint.x, 2.0f);
        EXPECT_NEAR(actor->transform->GetPosition2D().y, expectedPoint.y, 2.0f);
        EXPECT_GT(actor->transform->GetPosition2D().y, 10.0f);

        int aliveMid = emitter->GetParticlesCount();
        EXPECT_GT(aliveMid, 0);
        EXPECT_EQ(source->container->lastAlive, aliveMid);

        window->Timeline()->SetTimeCursor(0.0f);
        EXPECT_NEAR(actor->transform->GetPosition2D().x, 0.0f, 2.0f);
        EXPECT_EQ(emitter->GetParticlesCount(), 0);
        EXPECT_EQ(source->container->lastAlive, 0);
    }
}
