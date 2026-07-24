#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2/Utils/Threading/Threading.h"

using namespace o2;

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
