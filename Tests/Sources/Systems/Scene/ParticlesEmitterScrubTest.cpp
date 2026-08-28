#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Render/Particles/ParticlesContainer.h"
#include "o2/Render/Particles/ParticlesEmitter.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

#if IS_EDITOR

namespace
{
    // Container that records what the emitter handed it for drawing
    class CountingContainer : public ParticlesContainer
    {
    public:
        int lastAlive = -1;
        int updates = 0;

        void Update(Vector<Particle>& particles, int maxParticles) override
        {
            updates++;
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

// Editor scrubbing of a sub-track sets the emitter time without Update; the drawn container
// must still follow the restored baked frame
TEST(ParticlesEmitterScrub, SubControlledSetTimeSyncsContainer)
{
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();

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
    clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto player = mmake<AnimationPlayer>(actor.Get(), clip);
    ASSERT_TRUE(emitter->IsSubControlled());

    player->SetTime(0.5f);
    int aliveMid = emitter->GetParticlesCount();
    EXPECT_GT(aliveMid, 0);
    EXPECT_EQ(source->container->lastAlive, aliveMid);

    // scrub back to an already baked frame: no simulation runs, the container must be resynced
    player->SetTime(0.0f);
    EXPECT_EQ(emitter->GetParticlesCount(), 0);
    EXPECT_EQ(source->container->lastAlive, 0);
}

#endif

#if IS_EDITOR
// Mixed clip: a value track next to a particles sub-track must still evaluate at the scrub time
TEST(ParticlesEmitterScrub, ValueTrackNextToSubTrackEvaluatesAtScrubTime)
{
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    emitter->SetParticlesSource(mmake<CountingSource>());
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(0.5f);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto clip = mmake<AnimationClip>();
    auto angleTrack = clip->AddTrack<float>("transform/angleDegrees");
    *angleTrack = AnimationTrack<float>::Linear(0.0f, 100.0f, 1.0f);
    clip->AddTrack("component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto player = mmake<AnimationPlayer>(actor.Get(), clip);
    player->SetTime(0.5f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), angleTrack->GetValue(0.5f), 0.5f);
    EXPECT_GT(actor->transform->GetAngleDegrees(), 1.0f);
    EXPECT_LT(actor->transform->GetAngleDegrees(), 99.0f);
}

namespace
{
    float MeanAliveX(const Ref<ParticlesEmitter>& emitter, int& alive)
    {
        alive = 0;
        float sum = 0.0f;
        for (auto& particle : emitter->GetParticles())
        {
            if (!particle.alive)
                continue;

            alive++;
            sum += particle.position.x;
        }
        return alive > 0 ? sum/(float)alive : 0.0f;
    }
}

// After the emitter moved, the frames baked at the old place are re-simulated where it is now
TEST(ParticlesEmitterScrub, MovedEmitterRebakesFramesInPlace)
{
    for (bool relative : { false, true })
    {
        SceneCleanGuard guard;

        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->transform->SetSize2D(Vec2F(10, 10));
        auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
        emitter->SetParticlesSource(mmake<CountingSource>());
        emitter->SetShape(mmake<CircleParticlesEmitterShape>());
        emitter->SetEmissionDuration(1.0f);
        emitter->SetParticlesLifetime(5.0f);
        emitter->SetParticlesPerSecond(100.0f);
        emitter->SetMaxParticles(200);
        emitter->SetInitialSpeed(0.0f);
        emitter->SetInitialSpeedRange(0.0f);
        emitter->SetParticlesRelativity(relative);
        emitter->SetLoop(Loop::None);
        emitter->SetSubControlled(true);
        emitter->Stop();
        o2Scene.UpdateAddedEntities();

        actor->transform->SetPosition2D(Vec2F(0, 0));
        o2Scene.UpdateTransforms();
        emitter->SetTime(0.5f);

        int alive = 0;
        EXPECT_NEAR(MeanAliveX(emitter, alive), 0.0f, 30.0f) << "relative " << relative;
        ASSERT_GT(alive, 0);

        actor->transform->SetPosition2D(Vec2F(300, 0));
        o2Scene.UpdateTransforms();
        emitter->SetTime(0.5f);
        EXPECT_NEAR(MeanAliveX(emitter, alive), 300.0f, 30.0f) << "relative " << relative;
    }
}

// Re-simulation keeps the particle pattern: per-frame seeds make it independent of the bake order
TEST(ParticlesEmitterScrub, RebakeKeepsParticlePattern)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
    emitter->SetParticlesSource(mmake<CountingSource>());
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(5.0f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(200);
    emitter->SetInitialSpeed(50.0f);
    emitter->SetInitialSpeedRange(20.0f);
    emitter->SetInitialAngleRange(360.0f);
    emitter->SetEmitParticlesMoveDirectionRange(360.0f);
    emitter->SetParticlesRelativity(false);
    emitter->SetLoop(Loop::None);
    emitter->SetSubControlled(true);
    emitter->Stop();
    o2Scene.UpdateAddedEntities();

    auto pattern = [&](const Vec2F& origin)
    {
        Vector<Vec2F> result;
        for (auto& particle : emitter->GetParticles())
        {
            if (particle.alive)
                result.Add(Vec2F(particle.position.x, particle.position.y) - origin);
        }
        return result;
    };

    // baked in one go
    actor->transform->SetPosition2D(Vec2F(0, 0));
    o2Scene.UpdateTransforms();
    emitter->SetTime(0.5f);
    auto first = pattern(Vec2F(0, 0));
    ASSERT_FALSE(first.IsEmpty());

    // unrelated random draws in between, then baked frame by frame at another place
    for (int i = 0; i < 17; i++)
        rand();

    actor->transform->SetPosition2D(Vec2F(300, 0));
    o2Scene.UpdateTransforms();
    for (float t = 0.0f; t < 0.5f; t += 1.0f/60.0f)
        emitter->SetTime(t);
    emitter->SetTime(0.5f);

    auto second = pattern(Vec2F(300, 0));
    ASSERT_EQ(second.Count(), first.Count());
    for (int i = 0; i < first.Count(); i++)
    {
        EXPECT_NEAR(first[i].x, second[i].x, 0.01f) << "particle " << i;
        EXPECT_NEAR(first[i].y, second[i].y, 0.01f) << "particle " << i;
    }
}


namespace
{
    struct FlightRig
    {
        Ref<Actor> actor;
        Ref<Actor> sparks;
        Ref<ParticlesEmitterComponent> emitter;
        Ref<AnimationClip> clip;
        Ref<AnimationPlayer> player;
    };

    // Actor flying 0 -> 400 over one second with a sub-track emitter under it
    FlightRig MakeFlightRig(bool relative)
    {
        FlightRig rig;
        rig.actor = mmake<Actor>(ActorCreateMode::InScene);
        rig.sparks = mmake<Actor>(ActorCreateMode::InScene);
        rig.sparks->SetName("Sparks");
        rig.sparks->transform->SetSize2D(Vec2F(10, 10));
        rig.actor->AddChild(rig.sparks);

        rig.emitter = rig.sparks->AddComponent<ParticlesEmitterComponent>();
        rig.emitter->SetParticlesSource(mmake<CountingSource>());
        rig.emitter->SetShape(mmake<CircleParticlesEmitterShape>());
        rig.emitter->SetEmissionDuration(1.0f);
        rig.emitter->SetParticlesLifetime(5.0f);
        rig.emitter->SetParticlesPerSecond(100.0f);
        rig.emitter->SetMaxParticles(200);
        rig.emitter->SetInitialSpeed(0.0f);
        rig.emitter->SetInitialSpeedRange(0.0f);
        rig.emitter->SetParticlesRelativity(relative);
        rig.emitter->SetLoop(Loop::None);
        rig.emitter->Stop();
        o2Scene.UpdateAddedEntities();

        rig.clip = mmake<AnimationClip>();
        auto position = rig.clip->AddTrack<Vec2F>("transform/position2D");
        *position = AnimationTrack<Vec2F>::Linear(Vec2F(0, 0), Vec2F(400, 0), 1.0f);
        rig.clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

        rig.player = mmake<AnimationPlayer>(rig.actor.Get(), rig.clip);
        return rig;
    }

    void AliveRange(const Ref<ParticlesEmitter>& emitter, float& minX, float& maxX, int& alive)
    {
        alive = 0;
        minX = FLT_MAX;
        maxX = -FLT_MAX;
        for (auto& particle : emitter->GetParticles())
        {
            if (!particle.alive)
                continue;

            alive++;
            minX = Math::Min(minX, particle.position.x);
            maxX = Math::Max(maxX, particle.position.x);
        }
    }
}

// A scrub jump bakes every frame with the actor where the clip has it then: world-space particles
// stay along the path, relative ones travel with the actor
TEST(ParticlesEmitterScrub, JumpScrubBakesFramesAlongThePath)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);

    rig.player->SetTime(1.0f);
    EXPECT_NEAR(rig.actor->transform->GetPosition2D().x, 400.0f, 1.0f);

    float minX, maxX;
    int alive;
    AliveRange(rig.emitter, minX, maxX, alive);
    ASSERT_GT(alive, 0);
    EXPECT_LT(minX, 100.0f) << "first particles were emitted at the start of the path";
    EXPECT_GT(maxX, 340.0f) << "last particles were emitted near the end of the path";

    // scrubbing back shows only what was emitted by then, ending where the actor is at that time
    rig.player->SetTime(0.5f);
    float actorMidX = rig.actor->transform->GetPosition2D().x;
    EXPECT_GT(actorMidX, 100.0f);
    EXPECT_LT(actorMidX, 390.0f);

    AliveRange(rig.emitter, minX, maxX, alive);
    ASSERT_GT(alive, 0);
    EXPECT_LT(minX, 100.0f);
    EXPECT_NEAR(maxX, actorMidX, 40.0f);
}

TEST(ParticlesEmitterScrub, RelativeParticlesTravelWithTheActorOnScrub)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(true);

    rig.player->SetTime(1.0f);

    float minX, maxX;
    int alive;
    AliveRange(rig.emitter, minX, maxX, alive);
    ASSERT_GT(alive, 0);
    EXPECT_GT(minX, 340.0f) << "relative particles sit at the actor";
    EXPECT_LT(maxX, 460.0f);

    rig.player->SetTime(0.5f);
    float actorMidX = rig.actor->transform->GetPosition2D().x;
    AliveRange(rig.emitter, minX, maxX, alive);
    ASSERT_GT(alive, 0);
    EXPECT_GT(minX, actorMidX - 60.0f);
    EXPECT_LT(maxX, actorMidX + 60.0f);
}

// Editing the emitter duration must resize its sub-track in the clip right away
TEST(ParticlesEmitterScrub, EmitterDurationChangeResizesSubTrack)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);

    auto subTrack = rig.clip->GetTracks().FindOrDefault([](auto& t) { return DynamicCast<AnimationSubTrack>(t) != nullptr; });
    ASSERT_TRUE(subTrack);
    EXPECT_NEAR(subTrack->GetDuration(), rig.emitter->GetDuration(), 0.001f);

    rig.emitter->SetEmissionDuration(3.0f);
    EXPECT_NEAR(subTrack->GetDuration(), rig.emitter->GetDuration(), 0.001f);
    EXPECT_NEAR(rig.clip->GetDuration(), rig.emitter->GetDuration(), 0.001f);
}


// The sub-track player holds its target raw, and either side may die first (scene reload keeps
// the editor's player alive): neither may touch the other afterwards
TEST(ParticlesEmitterScrub, SubTrackPlayerAndEmitterOutliveEachOtherSafely)
{
    auto track = mmake<AnimationSubTrack>();

    // emitter dies first: evaluation and unsubscription must be skipped
    {
        auto emitter = mmake<ParticlesEmitter>();
        emitter->SetEmissionDuration(1.0f);

        auto player = mmake<AnimationSubTrack::Player>();
        player->SetTrack(track);
        player->SetTarget(emitter.Get());
        EXPECT_TRUE(emitter->IsSubControlled());
        EXPECT_NEAR(track->GetDuration(), emitter->GetDuration(), 0.001f);

        emitter = nullptr;

        player->ForceSetTime(0.5f, 1.0f);
        player = nullptr;
    }

    // player dies first: the emitter must not keep a dangling duration subscription
    {
        auto emitter = mmake<ParticlesEmitter>();
        emitter->SetEmissionDuration(1.0f);

        auto player = mmake<AnimationSubTrack::Player>();
        player->SetTrack(track);
        player->SetTarget(emitter.Get());
        EXPECT_FALSE(emitter->GetSubControlEvaluator().IsEmpty());
        player = nullptr;
        EXPECT_TRUE(emitter->GetSubControlEvaluator().IsEmpty());

        emitter->SetEmissionDuration(2.0f);
        EXPECT_NEAR(emitter->GetDuration(), 2.0f + emitter->GetParticlesLifetime(), 0.001f);
    }
}

#endif

// A sub-track starting later in the clip (the burst after the flight) must apply parameter
// edits at the current scrub position just like one starting at zero
TEST(ParticlesEmitterScrub, LateSubTrackEmitterAppliesEditsWithoutScrub)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);
    rig.emitter->SetMaxParticles(2000);

    auto subTrack = DynamicCast<AnimationSubTrack>(
        rig.clip->GetTracks().FindOrDefault([](auto& t) { return DynamicCast<AnimationSubTrack>(t) != nullptr; }));
    ASSERT_TRUE(subTrack);
    subTrack->SetBeginTime(0.5f);

    rig.player->SetTime(0.7f);
    EXPECT_NEAR(rig.emitter->GetTime(), 0.2f, 0.001f);

    int alive = 0;
    float minX = 0, maxX = 0;
    AliveRange(rig.emitter, minX, maxX, alive);
    ASSERT_GT(alive, 0);
    int aliveBefore = alive;

    rig.emitter->SetParticlesPerSecond(400.0f);
    AliveRange(rig.emitter, minX, maxX, alive);
    EXPECT_GT(alive, aliveBefore*3);
}

// The component viewer's play toggle may leave a sub-controlled emitter "playing": it never
// advances on its own, so edits must still rebake the frame at the scrubbed time
TEST(ParticlesEmitterScrub, PlayingFlagDoesNotBlockEditsOfSubControlledEmitter)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(2.0f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(2000);
    emitter->SetLoop(Loop::None);
    emitter->SetSubControlled(true);
    emitter->Play();
    ASSERT_TRUE(emitter->IsPlaying());

    emitter->SetTime(0.5f);
    int aliveBefore = 0;
    for (auto& particle : emitter->GetParticles())
        if (particle.alive)
            aliveBefore++;
    ASSERT_GT(aliveBefore, 0);

    emitter->SetParticlesPerSecond(400.0f);
    int aliveAfter = 0;
    for (auto& particle : emitter->GetParticles())
        if (particle.alive)
            aliveAfter++;
    EXPECT_GT(aliveAfter, aliveBefore*3);
    EXPECT_NEAR(emitter->GetTime(), 0.5f, 0.001f);
}

#if IS_EDITOR
// A value track on an emitter property fires OnChanged for every baked frame: baking must not be
// restarted from inside itself and the cache must stay consistent
TEST(ParticlesEmitterScrub, ValueTrackOnEmitterPropertyDoesNotCorruptBaking)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);

    auto coefTrack = rig.clip->AddTrack<float>("child/Sparks/component/o2::ParticlesEmitterComponent/emittingCoefficient");
    *coefTrack = AnimationTrack<float>::Linear(0.5f, 1.0f, 1.0f);
    rig.player = mmake<AnimationPlayer>(rig.actor.Get(), rig.clip);

    rig.player->SetTime(0.5f);
    EXPECT_GE(rig.emitter->GetBakedFramesCount(), 31);
    EXPECT_GT(rig.emitter->GetParticlesCount(), 0);
    EXPECT_NEAR(rig.emitter->GetEmittingCoef(), coefTrack->GetValue(0.5f), 0.01f);

    rig.player->SetTime(0.8f);
    EXPECT_GE(rig.emitter->GetBakedFramesCount(), 49);
    EXPECT_GT(rig.emitter->GetParticlesCount(), 0);

    rig.player->SetTime(0.3f);
    EXPECT_GE(rig.emitter->GetBakedFramesCount(), 19);
    EXPECT_GT(rig.emitter->GetParticlesCount(), 0);
    EXPECT_NEAR(rig.emitter->GetEmittingCoef(), coefTrack->GetValue(0.3f), 0.01f);
}

// Scrub positions rarely land on the 1/60 grid: the cached frame is compared with the emitter placed
// at the frame's own time, so a moving actor keeps its cache instead of rebaking on every scrub
TEST(ParticlesEmitterScrub, OffGridScrubKeepsBakedFrames)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);
    auto container = DynamicCast<CountingSource>(rig.emitter->GetParticlesSource())->container;

    rig.player->SetTime(0.5f);
    int frames = rig.emitter->GetBakedFramesCount();
    ASSERT_GE(frames, 31);
    int updates = container->updates;

    rig.player->SetTime(0.505f);
    EXPECT_EQ(rig.emitter->GetBakedFramesCount(), frames);
    EXPECT_LE(container->updates - updates, 2) << "cache hit must not re-simulate frames";

    updates = container->updates;
    rig.player->SetTime(0.41f);
    EXPECT_EQ(rig.emitter->GetBakedFramesCount(), frames);
    EXPECT_LE(container->updates - updates, 2);
}

// Writing a value back through its setter (properties refresh) is not a change and must not rebake
TEST(ParticlesEmitterScrub, UnchangedSetterDoesNotRebake)
{
    SceneCleanGuard guard;
    auto rig = MakeFlightRig(false);
    auto container = DynamicCast<CountingSource>(rig.emitter->GetParticlesSource())->container;

    rig.player->SetTime(0.5f);
    int updates = container->updates;

    rig.emitter->SetParticlesPerSecond(rig.emitter->GetParticlesPerSecond());
    rig.emitter->SetInitialSize(rig.emitter->GetInitialSize());
    rig.emitter->SetDuration(rig.emitter->GetDuration());
    rig.emitter->SetParticlesSource(rig.emitter->GetParticlesSource());
    EXPECT_EQ(container->updates, updates);

    rig.emitter->SetParticlesPerSecond(rig.emitter->GetParticlesPerSecond()*2.0f);
    EXPECT_GT(container->updates, updates) << "a real change rebakes the current frame";
}
#endif
