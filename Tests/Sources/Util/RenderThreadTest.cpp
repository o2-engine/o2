#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/RenderThread.h"
#include "o2/Utils/Threading/Threading.h"

using namespace o2;

TEST(RenderThread, DispatchesAndWaits)
{
    RenderThread renderThread;
    renderThread.Start();
    EXPECT_TRUE(renderThread.IsRunning());

    Atomic<int> ran(0);
    renderThread.DispatchFrame([&] { ran.FetchAdd(1); });
    renderThread.WaitFrameDone();
    EXPECT_EQ(ran.Load(), 1);

    renderThread.Stop();
    EXPECT_FALSE(renderThread.IsRunning());
}

TEST(RenderThread, RunsFramesInOrderWithRendezvous)
{
    RenderThread renderThread;
    renderThread.Start();

    const int frames = 200;
    Atomic<int> counter(0);
    Atomic<int> outOfOrder(0);

    for (int i = 0; i < frames; i++)
    {
        // DispatchFrame blocks until the previous frame finished, so each frame observes the counter
        // exactly equal to its index — proving the per-frame rendezvous
        renderThread.DispatchFrame([&, i] {
            if (counter.Load() != i)
                outOfOrder.FetchAdd(1);
            counter.FetchAdd(1);
        });
    }
    renderThread.WaitFrameDone();

    EXPECT_EQ(counter.Load(), frames);
    EXPECT_EQ(outOfOrder.Load(), 0);

    renderThread.Stop();
}

TEST(RenderThread, WorkRunsOnRenderThreadNotMain)
{
    RenderThread renderThread;
    renderThread.Start();

    Thread::Id mainId = Thread::GetCurrentThreadId();
    Atomic<bool> onOtherThread(false);
    renderThread.DispatchFrame([&] { onOtherThread.Store(Thread::GetCurrentThreadId() != mainId); });
    renderThread.WaitFrameDone();

    EXPECT_TRUE(onOtherThread.Load());
    renderThread.Stop();
}

TEST(RenderThread, StopWithoutDispatchIsSafe)
{
    RenderThread renderThread;
    renderThread.Start();
    renderThread.Stop();
    SUCCEED();
}
