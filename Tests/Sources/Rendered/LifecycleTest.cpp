#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include "o2/Application/Application.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Utils/Threading/Threading.h"

using namespace o2;

namespace o2
{
    // Fails the first frame while drawing and the second while updating, then behaves
    class FrameBreakerComponent: public Component
    {
    public:
        int updates = 0;
        int draws = 0;

        FrameBreakerComponent() {}
        FrameBreakerComponent(const FrameBreakerComponent& other): Component(other) {}

        SERIALIZABLE(FrameBreakerComponent);
        CLONEABLE_REF(FrameBreakerComponent);

    protected:
        void OnUpdate(float) override
        {
            updates++;
            if (updates == 2)
                throw std::runtime_error("update failed");
        }

        void OnDraw() override
        {
            draws++;
            if (draws == 1)
                throw std::runtime_error("draw failed");
        }
    };
}

// The application lifecycle is a coroutine: each ProcessFrame advances it one frame, running the
// frame body (which updates o2Time) and pumping main-thread jobs. These run in the rendered tier
// because ProcessFrame drives the render, which is absent in headless mode.

TEST(Lifecycle, PumpingFramesAdvancesFrameCounterThroughCoroutine)
{
    int before = o2Time.GetCurrentFrame();
    AppTestDriver::PumpFrames(5);
    int after = o2Time.GetCurrentFrame();
    EXPECT_EQ(after - before, 5);
}

TEST(Lifecycle, MainThreadJobRunsWithinFrame)
{
    Atomic<int> ran(0);
    o2Jobs.Schedule([&] { ran.FetchAdd(1); }, JobPriority::Normal, JobThread::Main);

    // The lifecycle-driving ProcessFrame pumps main-thread jobs every frame
    AppTestDriver::PumpFrames(1);
    EXPECT_EQ(ran.Load(), 1);
}

TEST(Lifecycle, CoroutineAdvancesOnePerFrameInRealLoop)
{
    Atomic<int> stage(0);
    auto coroutine = [](Atomic<int>* s) -> Coroutine<void> {
        s->Store(1);
        co_await WaitNextFrame();
        s->Store(2);
        co_await WaitNextFrame();
        s->Store(3);
    }(&stage);
    coroutine.Start(JobThread::Main);

    AppTestDriver::PumpFrames(1);
    EXPECT_GE(stage.Load(), 1);

    for (int i = 0; i < 10 && !coroutine.IsDone(); i++)
        AppTestDriver::PumpFrames(1);

    EXPECT_TRUE(coroutine.IsDone());
    EXPECT_EQ(stage.Load(), 3);
}

// An exception escaping the frame is logged and the frame is abandoned; the render is closed and the next frame runs as usual
TEST(Lifecycle, FrameSurvivesAnEscapedException)
{
    SceneCleanGuard guard;
    auto camera = mmake<CameraActor>();
    auto actor = mmake<Actor>();
    auto breaker = actor->AddComponent<FrameBreakerComponent>();

    int before = o2Time.GetCurrentFrame();
    AppTestDriver::PumpFrames(4);
    EXPECT_EQ(o2Time.GetCurrentFrame() - before, 4);
    EXPECT_EQ(breaker->updates, 4);
    EXPECT_GE(breaker->draws, 2);
}
// --- META ---

CLASS_BASES_META(o2::FrameBreakerComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::FrameBreakerComponent)
{
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(updates);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(draws);
}
END_META;
CLASS_METHODS_META(o2::FrameBreakerComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const FrameBreakerComponent&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;
// --- END META ---
