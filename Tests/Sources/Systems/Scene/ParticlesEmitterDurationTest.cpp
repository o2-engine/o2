#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Particles/ParticlesEmitter.h"

using namespace o2;

// The editor properties refresh writes values back through setters, so get->set must be stable
TEST(ParticlesEmitterDuration, SetGetRoundTripIsStableWhenLooped)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetEmissionDuration(10.0f);
    emitter->SetParticlesLifetime(1.6f);
    emitter->SetLoop(Loop::Repeat);

    float duration = emitter->GetDuration();
    emitter->SetDuration(duration);

    EXPECT_FLOAT_EQ(emitter->GetDuration(), duration);
    EXPECT_FLOAT_EQ(emitter->GetParticlesLifetime(), 1.6f);
}

TEST(ParticlesEmitterDuration, SetGetRoundTripIsStableWithoutLoop)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetEmissionDuration(10.0f);
    emitter->SetParticlesLifetime(1.6f);
    emitter->SetLoop(Loop::None);

    float duration = emitter->GetDuration();
    emitter->SetDuration(duration);

    EXPECT_FLOAT_EQ(emitter->GetDuration(), duration);
    EXPECT_FLOAT_EQ(emitter->GetParticlesLifetime(), 1.6f);
}
