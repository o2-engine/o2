#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/FlightTrajectoryComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Properties/Objects/Components/FlightTrajectoryViewer.h"
#include "o2Editor/Properties/Objects/Components/ParticlesEmitterComponentViewer.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Windows/AnimationWindow/Timeline.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/KeyFramesTrackControl.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct AnimationWindowProbe: AnimationWindow
    {
        AnimationWindowProbe(RefCounter* refCounter): AnimationWindow(refCounter) {}

        const Ref<DockableWindow>& Window() const { return mWindow; }
        const Ref<AnimationTimeline>& Timeline() const { return mTimeline; }
        const Ref<AnimationTree>& Tree() const { return mTree; }
        bool OwnsPlayer() const { return mOwnPreviewPlayer; }
        Ref<AnimationPlayer> Player() const { return mPreviewPlayer; }

        // Mirrors the editor: the state's own player of a scene actor is previewed
        void SetClip(const Ref<AnimationClip>& clip, const Ref<AnimationPlayer>& player)
        {
            mAnimation = clip;
            mPreviewPlayer = player;
            mOwnPreviewPlayer = false;
            mHandlesSheet->SetAnimation(clip);
            mTimeline->SetAnimation(clip, player);
            mTree->SetAnimation(clip);
        }
    };

    // Draws the widget alone on black and returns the color at its center (default camera at origin)
    Color4 DrawAndSampleCenter(const Ref<Widget>& widget)
    {
        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.SetCamera(Camera());
        o2Render.Clear(Color4::Black());
        widget->Draw();
        o2Render.End();

        if (!captured)
            return Color4(0, 0, 0, 0);

        Vec2I size = captured->GetSize();
        Vec2I resolution = o2Render.GetResolution();
        Vec2F center = widget->layout->GetWorldRect().Center();
        int x = size.x/2 + (int)(center.x*(float)size.x/(float)resolution.x);
        int y = size.y/2 - (int)(center.y*(float)size.y/(float)resolution.y);

        const UInt8* data = captured->GetData();
        int idx = (y*size.x + x)*4;
        return Color4((int)data[idx], (int)data[idx + 1], (int)data[idx + 2], (int)data[idx + 3]);
    }
}

// Editor flow for a prototype instance: the AnimationComponent state player is scrubbed from the
// timeline while the scene is not updating; widget layers and the trajectory must follow
TEST(AnimationWindowStateScrubUI, StatePlayerScrubUpdatesWidgetLayers)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    widget->name = "FxFlyingLetter";
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));

    auto back = widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto star = widget->AddLayer("star", mmake<Sprite>(Color4::Blue()), Layout::BothStretch());
    star->SetTransparency(0.0f);

    auto animation = widget->AddComponent<AnimationComponent>();
    auto trajectory = widget->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(200, 130), 0.0f);
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto clip = mmake<AnimationClip>();
    auto positionTrack = clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position");
    *positionTrack = AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);

    auto backTrack = clip->AddTrack<float>("layer/back/transparency");
    backTrack->AddKey(0.0f, 1.0f);
    backTrack->AddKey(1.0f, 0.0f);

    auto starTrack = clip->AddTrack<float>("layer/star/transparency");
    starTrack->AddKey(0.0f, 0.0f);
    starTrack->AddKey(1.0f, 1.0f);

    auto state = animation->AddState("flight", clip, AnimationMask(), 1.0f);
    state->autoPlay = false;

    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->SetClip(clip, DynamicCast<AnimationState>(state)->player);
        window->Window()->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.5f);
        window->Window()->Update(0.016f);

        EXPECT_NEAR(trajectory->GetPosition(), positionTrack->GetValue(0.5f), 0.01f);
        EXPECT_NEAR(back->GetTransparency(), backTrack->GetValue(0.5f), 0.01f);
        EXPECT_NEAR(star->GetTransparency(), starTrack->GetValue(0.5f), 0.01f);
        EXPECT_GT(star->GetTransparency(), 0.05f);
        EXPECT_LT(back->GetTransparency(), 0.95f);

        window->Timeline()->SetTimeCursor(1.0f);
        EXPECT_NEAR(back->GetTransparency(), 0.0f, 0.01f);
        EXPECT_NEAR(star->GetTransparency(), 1.0f, 0.01f);
        EXPECT_NEAR(back->GetDrawable()->GetTransparency(), 0.0f, 0.01f);
        EXPECT_NEAR(star->GetDrawable()->GetTransparency(), 1.0f, 0.01f);
    }
}

// Same flow through the real entry point: the state viewer's "edit" opens the animation asset with
// the state as editable preview, the window makes its own player over the scene widget
TEST(AnimationWindowStateScrubUI, EditAssetFromStateScrubUpdatesWidgetLayers)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    widget->name = "FxFlyingLetter";
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));

    auto back = widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto star = widget->AddLayer("star", mmake<Sprite>(Color4::Blue()), Layout::BothStretch());
    star->SetTransparency(0.0f);

    auto animation = widget->AddComponent<AnimationComponent>();
    auto trajectory = widget->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(200, 130), 0.0f);
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto clip = mmake<AnimationClip>();
    auto positionTrack = clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position");
    *positionTrack = AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);

    auto backTrack = clip->AddTrack<float>("layer/back/transparency");
    backTrack->AddKey(0.0f, 1.0f);
    backTrack->AddKey(1.0f, 0.0f);

    auto starTrack = clip->AddTrack<float>("layer/star/transparency");
    starTrack->AddKey(0.0f, 0.0f);
    starTrack->AddKey(1.0f, 1.0f);

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;

    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;

    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        ASSERT_TRUE(window->Player());
        EXPECT_TRUE(window->OwnsPlayer());

        window->Window()->Update(0.016f);
        window->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.5f);
        window->Update(0.016f);

        EXPECT_NEAR(trajectory->GetPosition(), positionTrack->GetValue(0.5f), 0.01f);
        EXPECT_NEAR(back->GetTransparency(), backTrack->GetValue(0.5f), 0.01f);
        EXPECT_NEAR(star->GetTransparency(), starTrack->GetValue(0.5f), 0.01f);
        EXPECT_NEAR(back->GetDrawable()->GetTransparency(), backTrack->GetValue(0.5f), 0.01f);

        window->Timeline()->SetTimeCursor(1.0f);
        window->Update(0.016f);
        EXPECT_NEAR(back->GetTransparency(), 0.0f, 0.01f);
        EXPECT_NEAR(star->GetTransparency(), 1.0f, 0.01f);

        // rendered result must follow: white tile at start, blue star at the end
        Color4 atEnd = DrawAndSampleCenter(widget);
        EXPECT_GT(atEnd.b, 200) << "star layer must be drawn at the end";
        EXPECT_LT(atEnd.r, 60) << "tile layer must be faded out at the end";

        window->Timeline()->SetTimeCursor(0.0f);
        window->Update(0.016f);
        Color4 atStart = DrawAndSampleCenter(widget);
        EXPECT_GT(atStart.r, 200) << "tile layer must be drawn at the start";
        EXPECT_GT(atStart.g, 200);
    }
}

namespace
{
    template<typename WidgetType>
    void CollectWidgets(const Ref<Widget>& root, Vector<Ref<WidgetType>>& result)
    {
        for (auto& child : root->GetChildWidgets())
        {
            if (auto typed = DynamicCast<WidgetType>(child))
                result.Add(typed);

            CollectWidgets(child, result);
        }
    }
}

// Layer and component tracks of a widget must get their rows and key handles in the window
TEST(AnimationWindowStateScrubUI, WidgetLayerTracksShowKeys)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    auto animation = widget->AddComponent<AnimationComponent>();
    widget->AddComponent<FlightTrajectoryComponent>();

    auto clip = mmake<AnimationClip>();
    auto positionTrack = clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position");
    positionTrack->AddKey(0.0f, 0.0f);
    positionTrack->AddKey(1.0f, 1.0f);
    auto backTrack = clip->AddTrack<float>("layer/back/transparency");
    backTrack->AddKey(0.0f, 1.0f);
    backTrack->AddKey(1.0f, 0.0f);

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        window->Window()->Update(0.016f);
        root->UpdateChildrenTransforms();

        Vector<Ref<AnimationTreeNode>> nodes;
        CollectWidgets(DynamicCast<Widget>(window->Tree()), nodes);
        ASSERT_FALSE(nodes.IsEmpty());

        int leafControls = 0;
        for (auto& node : nodes)
        {
            Vector<Ref<KeyFramesTrackControl<AnimationTrack<float>>>> controls;
            CollectWidgets<KeyFramesTrackControl<AnimationTrack<float>>>(node, controls);
            for (auto& control : controls)
            {
                leafControls++;
                EXPECT_EQ(control->GetKeyHandles().Count(), 2) << "track row must show both keys";
            }
        }
        EXPECT_EQ(leafControls, 2) << "both tracks must build key rows";
    }
}

// Scrubbing back and forth with the properties panel refreshing must not move the actor off its
// trajectory: same cursor time gives the same point, the random corridor offset stays fixed
TEST(AnimationWindowStateScrubUI, ScrubbingIsStableWithPropertiesRefresh)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));
    widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());

    auto animation = widget->AddComponent<AnimationComponent>();
    auto trajectory = widget->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(200, 130), 300.0f); // wide corridor: a reroll is visible
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto sparks = mmake<Actor>(ActorCreateMode::InScene);
    sparks->SetName("Sparks");
    widget->AddChild(sparks);
    auto emitter = sparks->AddComponent<ParticlesEmitterComponent>();
    emitter->SetEmissionDuration(0.45f);
    emitter->SetParticlesLifetime(0.3f);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto clip = mmake<AnimationClip>();
    auto positionTrack = clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position");
    *positionTrack = AnimationTrack<float>::EaseInOut(0.0f, 1.0f, 0.45f);
    clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));

        auto viewer = mmake<FlightTrajectoryViewer>();
        auto parent = o2UI.CreateWidget<VerticalLayout>();
        viewer->CheckCreateSpoiler(parent);
        viewer->SetHeaderEnabled(false);
        viewer->Refresh({ { dynamic_cast<IObject*>(trajectory.Get()), nullptr } });

        // the editor frame: the moved widget is reported as changed and the scene screen runs
        // CheckChangedObjects, which updates the actor (and its AnimationComponent) with dt = 0
        auto frame = [&](float time)
        {
            window->Timeline()->SetTimeCursor(time);
            window->Update(0.016f);
            widget->UpdateTransform();
            o2Scene.OnObjectChanged(widget);
            o2Scene.CheckChangedObjects();
            viewer->Refresh({ { dynamic_cast<IObject*>(trajectory.Get()), nullptr } });
            window->Update(0.016f);
            return widget->layout->GetWorldRect().Center();
        };

        Vec2F mid = trajectory->EvaluatePoint(0.5f);

        Vec2F first = frame(0.2f);
        EXPECT_NEAR(trajectory->GetPosition(), positionTrack->GetValue(0.2f), 0.001f)
            << "the component mixers must not overwrite the previewed value";
        Vec2F firstAgain = frame(0.2f);
        EXPECT_NEAR(first.x, firstAgain.x, 0.01f);
        EXPECT_NEAR(first.y, firstAgain.y, 0.01f);

        // forward and back within the flight: the same time must give the same point
        for (float t : { 0.3f, 0.4f, 0.45f, 0.6f, 0.3f, 0.1f, 0.05f, 0.1f, 0.2f })
            frame(t);

        Vec2F afterRoundTrip = frame(0.2f);
        EXPECT_NEAR(first.x, afterRoundTrip.x, 0.01f);
        EXPECT_NEAR(first.y, afterRoundTrip.y, 0.01f);

        Vec2F midAfter = trajectory->EvaluatePoint(0.5f);
        EXPECT_NEAR(mid.x, midAfter.x, 0.01f) << "corridor offset must not reroll while scrubbing mid-flight";
        EXPECT_NEAR(mid.y, midAfter.y, 0.01f);

        // back to the very start is a new flight: the corridor offset rerolls once and then holds
        float offset = trajectory->GetRandomOffset();
        frame(0.0f);
        float rerolled = trajectory->GetRandomOffset();
        EXPECT_NE(rerolled, offset) << "entering zero starts a new flight";
        frame(0.0f);
        frame(0.3f);
        frame(0.2f);
        EXPECT_EQ(trajectory->GetRandomOffset(), rerolled) << "no further rerolls until the next start";
    }
}

// A sub-track emitter under an actor moved by the same clip: frames baked on scrub must use the
// actor position of that time, not the stale transform from before the value tracks were applied
TEST(AnimationWindowStateScrubUI, SubTrackParticlesFollowMovedActor)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));

    auto animation = widget->AddComponent<AnimationComponent>();
    auto trajectory = widget->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto sparks = mmake<Actor>(ActorCreateMode::InScene);
    sparks->SetName("Sparks");
    widget->AddChild(sparks);
    auto emitter = sparks->AddComponent<ParticlesEmitterComponent>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(2.0f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(100);
    emitter->SetInitialSpeed(0.0f);
    emitter->SetInitialSpeedRange(0.0f);
    emitter->SetParticlesRelativity(false);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::Linear(0.0f, 1.0f, 1.0f);
    clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        window->Window()->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.5f);
        window->Update(0.016f);

        float actorX = widget->layout->GetWorldRect().Center().x;
        EXPECT_GT(actorX, 100.0f);

        auto aliveRange = [&](float& minX, float& maxX)
        {
            int alive = 0;
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
            return alive;
        };

        // world-space particles: each frame was baked with the actor where the clip has it then
        float minX, maxX;
        ASSERT_GT(aliveRange(minX, maxX), 0);
        EXPECT_LT(minX, 60.0f) << "first particles were emitted at the start";
        EXPECT_NEAR(maxX, actorX, 40.0f) << "newest particles were emitted where the actor is now";

        // scrub back: only what was emitted by then, ending where the actor is at that time
        window->Timeline()->SetTimeCursor(0.25f);
        window->Update(0.016f);

        float actorXBack = widget->layout->GetWorldRect().Center().x;
        EXPECT_LT(actorXBack, actorX - 50.0f);

        ASSERT_GT(aliveRange(minX, maxX), 0);
        EXPECT_NEAR(maxX, actorXBack, 40.0f) << "particles emitted later than the cursor must not show";
    }
}

// Editing the emitter duration while its animation is open: the clip change re-evaluates the
// window's player (nested from the duration event) and must not break it
TEST(AnimationWindowStateScrubUI, EmitterDurationChangeWhileEditingIsSafe)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));
    auto back = widget->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());

    auto animation = widget->AddComponent<AnimationComponent>();
    auto trajectory = widget->AddComponent<FlightTrajectoryComponent>();
    trajectory->spline = mmake<Spline>();
    trajectory->spline->AppendKey(Vec2F(0, 0), 0.0f);
    trajectory->spline->AppendKey(Vec2F(400, 0), 0.0f);
    trajectory->SetPoints(0, 0, 400, 0);

    auto sparks = mmake<Actor>(ActorCreateMode::InScene);
    sparks->SetName("Sparks");
    sparks->transform->SetSize2D(Vec2F(10, 10));
    widget->AddChild(sparks);
    auto emitter = sparks->AddComponent<ParticlesEmitterComponent>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(0.45f);
    emitter->SetParticlesLifetime(0.3f);
    emitter->SetLoop(Loop::None);
    emitter->Stop();

    auto clip = mmake<AnimationClip>();
    *clip->AddTrack<float>("component/o2::FlightTrajectoryComponent/position") =
        AnimationTrack<float>::Linear(0.0f, 1.0f, 0.45f);
    auto backTrack = clip->AddTrack<float>("layer/back/transparency");
    backTrack->AddKey(0.0f, 1.0f);
    backTrack->AddKey(0.45f, 0.0f);
    clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        window->Window()->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.3f);
        window->Update(0.016f);

        // properties panel edits the emitter: sub-track resizes, clip changes, window re-evaluates
        emitter->SetEmissionDuration(2.0f);
        EXPECT_NEAR(clip->GetDuration(), 2.3f, 0.001f);
        window->Update(0.016f);

        emitter->SetEmissionDuration(0.2f);
        window->Update(0.016f);

        window->Timeline()->SetTimeCursor(0.4f);
        window->Update(0.016f);
        EXPECT_NEAR(trajectory->GetPosition(), 1.0f, 0.05f);

        // opening the same asset again rebinds the window player while the old one dies
        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));
        emitter->SetEmissionDuration(1.0f);
        window->Timeline()->SetTimeCursor(0.2f);
        window->Update(0.016f);
    }
}

// Editing the selected emitter whose sub-track starts later in the clip (the burst after the
// flight): the change must show at the current scrub position without scrubbing or play mode
TEST(AnimationWindowStateScrubUI, LateSubTrackEmitterEditAppliesWhileSelected)
{
    SceneCleanGuard guard;
    auto uiRoot = mmake<UIRoot>();

    auto& screen = o2EditorSceneScreen;
    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(64, 64));
    auto animation = widget->AddComponent<AnimationComponent>();

    auto makeEmitter = [&](const String& name, float emission, float lifetime)
    {
        auto actor = mmake<Actor>(ActorCreateMode::InScene);
        actor->SetName(name);
        widget->AddChild(actor);
        auto emitter = actor->AddComponent<ParticlesEmitterComponent>();
        emitter->SetShape(mmake<CircleParticlesEmitterShape>());
        emitter->SetEmissionDuration(emission);
        emitter->SetParticlesLifetime(lifetime);
        emitter->SetParticlesPerSecond(100.0f);
        emitter->SetMaxParticles(2000);
        emitter->SetLoop(Loop::None);
        return emitter;
    };

    auto sparks = makeEmitter("Sparks", 0.45f, 0.3f);
    sparks->Stop();
    auto burst = makeEmitter("Burst", 0.1f, 0.35f);

    auto clip = mmake<AnimationClip>();
    clip->AddTrack("child/Sparks/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent));
    auto burstTrack = DynamicCast<AnimationSubTrack>(
        clip->AddTrack("child/Burst/component/o2::ParticlesEmitterComponent", TypeOf(ParticlesEmitterComponent)));
    burstTrack->SetBeginTime(0.45f);

    auto animAsset = mmake<AnimationAsset>();
    animAsset->animation = clip;
    auto state = mmake<AnimationState>("flight");
    state->SetAnimation(animAsset);
    animation->AddState(state);
    state->autoPlay = false;
    TickScene();

    auto alive = [](const Ref<ParticlesEmitterComponent>& emitter)
    {
        int count = 0;
        for (auto& particle : emitter->GetParticles())
            if (particle.alive)
                count++;
        return count;
    };

    {
        auto window = mmake<AnimationWindowProbe>();
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));
        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->EditAsset(AssetRef<Asset>(animAsset), animation, DynamicCast<IAssetEditablePreview>(state));

        // the burst is selected: its component viewer drives the particles preview and refreshes
        auto viewer = mmake<ParticlesEmitterComponentViewer>();
        auto parent = o2UI.CreateWidget<VerticalLayout>();
        viewer->CheckCreateSpoiler(parent);
        viewer->SetHeaderEnabled(false);
        viewer->Refresh({ { dynamic_cast<IObject*>(burst.Get()), nullptr } });
        Ref<IObjectPropertiesViewer> baseViewer = viewer;
        baseViewer->OnPropertiesEnabled();

        auto frame = [&]()
        {
            window->Update(0.016f);
            screen.Update(0.016f);
            viewer->Refresh({ { dynamic_cast<IObject*>(burst.Get()), nullptr } });
            o2Scene.CheckChangedObjects();
            window->Update(0.016f);
        };

        window->Timeline()->SetTimeCursor(0.65f);
        for (int i = 0; i < 3; i++)
            frame();

        EXPECT_NEAR(burst->GetTime(), 0.2f, 0.001f);
        int burstBefore = alive(burst);
        int sparksBefore = alive(sparks);
        ASSERT_GT(burstBefore, 0);
        ASSERT_GT(sparksBefore, 0);

        // the properties window sets the value through the setter and reports the actor changed
        burst->SetParticlesPerSecond(400.0f);
        o2Scene.OnObjectChanged(burst->GetActor());
        sparks->SetParticlesPerSecond(400.0f);
        o2Scene.OnObjectChanged(sparks->GetActor());
        for (int i = 0; i < 3; i++)
            frame();

        EXPECT_NEAR(burst->GetTime(), 0.2f, 0.001f);
        EXPECT_GT(alive(sparks), sparksBefore*3) << "sparks (sub-track from zero) must apply the edit";
        EXPECT_GT(alive(burst), burstBefore*3) << "burst (late sub-track) must apply the edit";

        baseViewer->OnPropertiesDisabled();
    }

    o2Scene.CheckChangedObjects();
}
