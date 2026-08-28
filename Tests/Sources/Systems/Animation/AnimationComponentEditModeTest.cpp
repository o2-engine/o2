#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// The editor previews a state with its own player and updates changed actors with dt = 0:
// the component's mixers must not overwrite the previewed values while the state is in edit mode
TEST(AnimationComponentEditMode, MixersDoNotOverwritePreviewedState)
{
    SceneCleanGuard guard;

    auto actor = mmake<Actor>(ActorCreateMode::InScene);
    auto animation = actor->AddComponent<AnimationComponent>();

    auto clip = mmake<AnimationClip>();
    auto track = clip->AddTrack<float>("transform/angleDegrees");
    track->AddKey(0.0f, 0.0f);
    track->AddKey(1.0f, 100.0f);

    auto state = DynamicCast<AnimationState>(animation->AddState("turn", clip, AnimationMask(), 1.0f));
    ASSERT_TRUE(state);
    state->autoPlay = false;
    TickFrame();

    Ref<IAssetEditablePreview> preview = state;
    preview->BeginPreview();
    EXPECT_TRUE(state->IsInEditMode());

    auto editorPlayer = mmake<AnimationPlayer>(actor.Get(), clip);
    editorPlayer->SetTime(0.5f);
    float previewed = actor->transform->GetAngleDegrees();
    EXPECT_GT(previewed, 1.0f);

    actor->Update(0.0f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), previewed, 0.001f);

    TickFrame();
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), previewed, 0.001f);

    // out of preview the state drives the value again (its player is at 0)
    preview->EndPreview();
    EXPECT_FALSE(state->IsInEditMode());
    actor->Update(0.0f);
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 0.0f, 0.001f);
}
