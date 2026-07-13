#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Animation/Tracks/AnimationVec3FTrack.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
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
        const Ref<AnimationTree>& Tree() const { return mTree; }

        void SetClip(const Ref<AnimationClip>& clip)
        {
            mAnimation = clip;
            mHandlesSheet->SetAnimation(clip);
            mTimeline->SetAnimation(clip);
            mTree->SetAnimation(clip);
        }

        // Mirrors InitializeOwnAnimationPlayer: preview player bound to the target actor
        void SetClipWithTarget(const Ref<AnimationClip>& clip, const Ref<Actor>& target)
        {
            mAnimation = clip;
            mTargetActor = target;

            mOwnPreviewPlayer = true;
            mPreviewPlayer = mmake<AnimationPlayer>(target.Get(), clip);
            mPreviewPlayer->SetTime(0.0f);

            mHandlesSheet->SetAnimation(clip);
            mTimeline->SetAnimation(clip, mPreviewPlayer);
            mTree->SetAnimation(clip);
        }

        const Ref<AnimationPlayer>& Player() const { return mPreviewPlayer; }
    };

    // Clip shaped like SkinnedModelAnimation::ConvertClip output: a bone chain with
    // position/eulerAngles/scale Vec3F tracks, keys sampled per frame
    Ref<AnimationClip> BuildCharacterClip(int bonesCount, int keysCount, float duration)
    {
        auto clip = mmake<AnimationClip>();
        clip->SetLoop(Loop::Repeat);

        String bonePath;
        for (int bone = 0; bone < bonesCount; bone++)
        {
            bonePath += bonePath.IsEmpty() ? String("child/bone") + (String)bone
                                           : String("/child/bone") + (String)bone;

            const char* channels[] = { "position", "eulerAngles", "scale" };
            for (auto channel : channels)
            {
                auto track = clip->AddTrack<Vec3F>(bonePath + "/transform/" + channel);
                for (int i = 0; i < keysCount; i++)
                {
                    float time = duration*(float)i/(float)(keysCount - 1);
                    track->AddKey(time, Vec3F(Math::Sin(time + (float)bone), (float)bone, time));
                }
            }
        }

        return clip;
    }
}

namespace
{
    // Bone actors chain matching BuildCharacterClip paths, for the preview player binding
    Ref<Actor> BuildCharacterTarget(int bonesCount)
    {
        auto target = mmake<Actor>();
        auto parent = target;
        for (int i = 0; i < bonesCount; i++)
        {
            auto bone = mmake<Actor>();
            bone->SetName(String("bone") + (String)i);
            parent->AddChild(bone);
            parent = bone;
        }

        return target;
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
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const UInt8* pa = GetPixel(a, x, y);
                const UInt8* pb = GetPixel(b, x, y);
                if (pa[0] != pb[0] || pa[1] != pb[1] || pa[2] != pb[2])
                    count++;
            }
        }

        return count;
    }
}

// Realistic scenario: preview player bound to a bones hierarchy, window is drawn
// for real. Frames must be fast and pixel-stable when nothing changes
TEST(AnimationWindowCharacterClipUI, PreviewFramesAreFastAndStable)
{
    auto uiRoot = mmake<UIRoot>();

    auto clip = BuildCharacterClip(20, 60, 2.0f);
    auto target = BuildCharacterTarget(20);

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->SetEnabledForcible(true);

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->SetClipWithTarget(clip, target);

        int drawCalls = 0;
        auto tickFrame = [&](bool capture = false) -> Ref<Bitmap>
        {
            Ref<Bitmap> captured;
            if (capture)
                o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            window->Window()->Update(0.016f);
            if (auto targetActor = target)
                targetActor->UpdateTransform();

            root->UpdateChildrenTransforms();

            o2Render.Begin();
            root->Draw();
            drawCalls = o2Render.GetDrawCallsCount();
            o2Render.End();

            return captured;
        };

        for (int i = 0; i < 3; i++)
            tickFrame();

        printf("root enabled: %i, window enabled: %i, window visible: %i\n",
               (int)root->IsEnabledInHierarchy(), (int)window->Window()->IsEnabledInHierarchy(),
               (int)window->Window()->IsEnabled());

        ASSERT_GT(drawCalls, 10) << "window must actually be drawn for a realistic measure";

        const int framesCount = 20;
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < framesCount; i++)
            tickFrame();

        double frameMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count()/framesCount;

        printf("realistic preview frame time: %.2f ms, %i draw calls\n", frameMs, drawCalls);
        EXPECT_LT(frameMs, 50.0) << "frame took " << frameMs << " ms";

        // Static scene: consecutive frames must be pixel-identical, differences mean flickering.
        // The first captured frame is skipped: switching to a capture render target
        // takes a frame to settle
        tickFrame(true);

        auto frameA = tickFrame(true);
        auto frameB = tickFrame(true);
        auto frameC = tickFrame(true);

        ASSERT_TRUE(frameA && frameB && frameC);

        if (const char* dumpDir = getenv("ANIM_TEST_DUMP_DIR"))
        {
            frameA->Save(String(dumpDir) + "/frameA.png", Bitmap::ImageType::Png);
            frameB->Save(String(dumpDir) + "/frameB.png", Bitmap::ImageType::Png);
            frameC->Save(String(dumpDir) + "/frameC.png", Bitmap::ImageType::Png);
        }

        EXPECT_EQ(CountDifferentPixels(frameA, frameB), 0) << "drawing flickers between frames";
        EXPECT_EQ(CountDifferentPixels(frameB, frameC), 0) << "drawing flickers between frames";
    }
}

// A character animation stays interactive after opening: updating and drawing
// a frame must not lag with thousands of keys shown
TEST(AnimationWindowCharacterClipUI, UpdatesAndDrawsFast)
{
    auto uiRoot = mmake<UIRoot>();

    auto clip = BuildCharacterClip(20, 60, 2.0f);

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        window->SetClip(clip);

        auto tickFrame = [&]()
        {
            window->Window()->Update(0.016f);
            root->UpdateChildrenTransforms();

            o2Render.Begin();
            root->Draw();
            o2Render.End();
        };

        for (int i = 0; i < 3; i++)
            tickFrame();

        const int framesCount = 30;
        double updateMs = 0.0, transformMs = 0.0, drawMs = 0.0;
        auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < framesCount; i++)
        {
            auto t0 = std::chrono::steady_clock::now();
            window->Window()->Update(0.016f);
            auto t1 = std::chrono::steady_clock::now();
            root->UpdateChildrenTransforms();
            auto t2 = std::chrono::steady_clock::now();

            o2Render.Begin();
            root->Draw();
            o2Render.End();
            auto t3 = std::chrono::steady_clock::now();

            updateMs += std::chrono::duration<double, std::milli>(t1 - t0).count();
            transformMs += std::chrono::duration<double, std::milli>(t2 - t1).count();
            drawMs += std::chrono::duration<double, std::milli>(t3 - t2).count();
        }

        auto elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        double frameMs = elapsedMs/framesCount;
        printf("character clip frame time: %.2f ms (update %.2f, transforms %.2f, draw %.2f)\n",
               frameMs, updateMs/framesCount, transformMs/framesCount, drawMs/framesCount);

        EXPECT_LT(frameMs, 50.0) << "frame update+draw took " << frameMs << " ms";
    }
}

// Opening a character animation (dozens of deep bone tracks with per-frame keys)
// must not hang the editor
TEST(AnimationWindowCharacterClipUI, OpensInReasonableTime)
{
    auto uiRoot = mmake<UIRoot>();

    auto clip = BuildCharacterClip(20, 60, 2.0f);

    {
        auto window = mmake<AnimationWindowProbe>();

        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(1000.0f, 800.0f));
        *window->Window()->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(900.0f, 700.0f));

        root->UpdateSelfTransform();
        root->UpdateChildrenTransforms();

        auto start = std::chrono::steady_clock::now();

        window->SetClip(clip);

        window->Window()->Update(0.016f);
        root->UpdateChildrenTransforms();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        EXPECT_LT(elapsed, 10000) << "opening a character clip took " << elapsed << " ms";
    }
}
