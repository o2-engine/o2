#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/System/Time/Timer.h"

#include <chrono>

#if defined(O2_PROFILER_ENABLED)

using namespace o2;

namespace
{
    // Nanoseconds per iteration of the measured body
    double MeasureNs(int iterations, const std::function<void()>& body)
    {
        // warm up, then take the best of a few runs so a scheduler hiccup doesn't decide the number
        for (int i = 0; i < iterations/10 + 1; i++)
            body();

        double best = 1e18;
        for (int run = 0; run < 5; run++)
        {
            const auto begin = std::chrono::steady_clock::now();
            for (int i = 0; i < iterations; i++)
                body();
            const auto end = std::chrono::steady_clock::now();

            const double ns = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            best = Math::Min(best, ns/iterations);
        }

        return best;
    }

    // An unoptimized build inlines nothing, so every measured path costs a real call; the per-scope
    // budgets below only mean something for a shipping build
#ifdef DEBUG
    constexpr double kBudgetFactor = 4.0;
#else
    constexpr double kBudgetFactor = 1.0;
#endif

    class NanoProfilerBenchmark: public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            NanoProfiler::UnbindThread();
            NanoProfiler::BindThread();
        }

        void TearDown() override
        {
            NanoProfiler::SetEnabled(false);
            NanoProfiler::UnbindThread();
        }
    };
}

// A scope of a profiler that isn't recording must be nearly free, otherwise shipping builds would pay
// for the instrumentation they never read
TEST_F(NanoProfilerBenchmark, DisabledScopeIsAlmostFree)
{
    NanoProfiler::SetEnabled(false);
    NanoProfiler::NextFrame();

    const double ns = MeasureNs(200000, []() { NanoProfiler::SampleScope scope("benchmark"); });
    o2Debug.Log("NanoProfiler: disabled scope %.1f ns", ns);

    EXPECT_LT(ns, 25.0*kBudgetFactor);
}

// On a thread the profiler doesn't record, a scope is a single null check
TEST_F(NanoProfilerBenchmark, UnboundThreadScopeIsAlmostFree)
{
    NanoProfiler::UnbindThread();
    NanoProfiler::SetEnabled(true);

    const double ns = MeasureNs(200000, []() { NanoProfiler::SampleScope scope("benchmark"); });
    o2Debug.Log("NanoProfiler: unbound thread scope %.1f ns", ns);

    EXPECT_LT(ns, 25.0*kBudgetFactor);
}

// Recording a scope is two clock reads plus a store, and must stay in the tens of nanoseconds so a
// few thousand scopes a frame don't distort the frame they measure
TEST_F(NanoProfilerBenchmark, RecordedScopeCostsTwoClockReads)
{
    NanoProfiler::SetEnabled(true);

    const double ns = MeasureNs(1000, []()
    {
        NanoProfiler::NextFrame();
        for (int i = 0; i < 100; i++)
        {
            NanoProfiler::SampleScope scope("benchmark");
        }
    })/100.0;

    o2Debug.Log("NanoProfiler: recorded scope %.1f ns", ns);

    EXPECT_LT(ns, 200.0*kBudgetFactor);
}

// Reducing a full frame to per-name self times is allocation free and runs once per frame
TEST_F(NanoProfilerBenchmark, AggregatingAFullFrameIsCheap)
{
    NanoProfiler::SetEnabled(true);
    NanoProfiler::NextFrame();

    static const char* names[] = { "update", "draw", "physics", "audio", "ui", "scripts", "assets", "jobs" };

    // a frame shaped like a real one: a few hundred nested scopes over a handful of distinct names
    for (int i = 0; i < 400; i++)
    {
        NanoProfiler::SampleScope outer(names[i%8]);
        NanoProfiler::SampleScope inner(names[(i + 3)%8]);
    }

    NanoProfiler::NextFrame();
    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 800);

    static NanoProfiler::AggregatedSample aggregated[NanoProfiler::maxAggregatedSamples];
    const double ns = MeasureNs(200, []()
    {
        NanoProfiler::AggregateFrame(aggregated, NanoProfiler::maxAggregatedSamples);
    });

    o2Debug.Log("NanoProfiler: aggregating 800 samples %.1f us", ns/1000.0);

    EXPECT_LT(ns, 200000.0); // 200 us
}

#endif // O2_PROFILER_ENABLED
