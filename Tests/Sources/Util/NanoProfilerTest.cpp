#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Editor/EditorScope.h"

#include <thread>

#if defined(O2_PROFILER_ENABLED)

using namespace o2;

namespace
{
    // Binds the profiler to the test thread and leaves nothing behind: the suites share a process
    class NanoProfilerGuard
    {
    public:
        NanoProfilerGuard()
        {
            NanoProfiler::UnbindThread();
            NanoProfiler::BindThread();
            NanoProfiler::SetEnabled(true);
            NanoProfiler::NextFrame();
        }

        ~NanoProfilerGuard()
        {
            NanoProfiler::SetEnabled(false);
            NanoProfiler::UnbindThread();
        }
    };

    const NanoProfiler::Sample* FindSample(const char* name)
    {
        const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
        for (int i = 0; i < NanoProfiler::GetFrameSamplesCount(); i++)
        {
            if (samples[i].name == name)
                return samples + i;
        }

        return nullptr;
    }

    Int64 AggregatedTime(const char* name, const NanoProfiler::AggregatedSample* samples, int count)
    {
        for (int i = 0; i < count; i++)
        {
            if (samples[i].name == name)
                return samples[i].time;
        }

        return -1;
    }

    void Spin(int iterations)
    {
        volatile int sink = 0;
        for (int i = 0; i < iterations; i++)
            sink += i;
    }
}

TEST(NanoProfiler, UnboundThreadRecordsNothing)
{
    NanoProfiler::UnbindThread();
    NanoProfiler::SetEnabled(true);

    EXPECT_FALSE(NanoProfiler::IsThreadBound());
    EXPECT_EQ(NanoProfiler::BeginSample("unbound"), -1);
    NanoProfiler::EndSample();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);
    EXPECT_EQ(NanoProfiler::GetFrameSamples(), nullptr);

    NanoProfiler::SetEnabled(false);
}

TEST(NanoProfiler, SamplesArePublishedOnTheNextFrame)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope scope("frame work");
        Spin(1000);
    }

    // still being recorded, the completed frame is the previous, empty one
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);

    const NanoProfiler::Sample* sample = FindSample("frame work");
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(sample->parent, -1);
    EXPECT_GT(sample->end, sample->begin);

    // and the next frame starts empty again
    NanoProfiler::NextFrame();
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);
}

TEST(NanoProfiler, NestedScopesKeepTheirParents)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope outer("outer");
        {
            NanoProfiler::SampleScope inner("inner");
            Spin(1000);
        }
        {
            NanoProfiler::SampleScope sibling("sibling");
            Spin(1000);
        }
    }

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 3);

    const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
    EXPECT_EQ(samples[0].parent, -1);
    EXPECT_EQ(samples[1].parent, 0);
    EXPECT_EQ(samples[2].parent, 0);
}

TEST(NanoProfiler, DisabledProfilerRecordsNothingButStaysBalanced)
{
    NanoProfilerGuard guard;

    NanoProfiler::SetEnabled(false);
    NanoProfiler::NextFrame();

    {
        NanoProfiler::SampleScope outer("disabled outer");
        NanoProfiler::SampleScope inner("disabled inner");
    }

    NanoProfiler::NextFrame();
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);

    // recording resumes cleanly, no leftover open scopes from the disabled run
    NanoProfiler::SetEnabled(true);
    NanoProfiler::NextFrame();

    {
        NanoProfiler::SampleScope scope("after enable");
    }

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);
    EXPECT_EQ(NanoProfiler::GetFrameSamples()[0].parent, -1);
}

TEST(NanoProfiler, ScopesTakenInEditorScopeAreDropped)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope game("game work");

        {
            PushEditorScopeOnStack editor;
            NanoProfiler::SampleScope editorWork("editor work");
        }

        NanoProfiler::SampleScope moreGame("more game work");
    }

    NanoProfiler::NextFrame();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 2);
    EXPECT_NE(FindSample("game work"), nullptr);
    EXPECT_NE(FindSample("more game work"), nullptr);
    EXPECT_EQ(FindSample("editor work"), nullptr);
}

// A scope can open inside an editor scope and close outside of it, and the other way round; both
// must leave the recorded stack where it was
TEST(NanoProfiler, EditorScopeLeavingMidScopeKeepsTheStackBalanced)
{
    NanoProfilerGuard guard;

    {
        PushEditorScopeOnStack editor;
        NanoProfiler::SampleScope skipped("skipped outer");

        {
            ForcePopEditorScopeOnStack game;
            NanoProfiler::SampleScope recorded("recorded inner");
        }
    }

    {
        NanoProfiler::SampleScope root("root");
    }

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 2);
    EXPECT_EQ(FindSample("skipped outer"), nullptr);

    const NanoProfiler::Sample* inner = FindSample("recorded inner");
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->parent, -1);

    const NanoProfiler::Sample* root = FindSample("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->parent, -1);
}

TEST(NanoProfiler, ExcludeScopeSuspendsRecording)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope before("before");
    }

    {
        NanoProfiler::ExcludeScope exclude;
        NanoProfiler::SampleScope excluded("excluded");
    }

    {
        NanoProfiler::SampleScope after("after");
    }

    NanoProfiler::NextFrame();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 2);
    EXPECT_EQ(FindSample("excluded"), nullptr);
}

TEST(NanoProfiler, ScopeOpenAcrossTheFrameBoundaryIsClosedAtIt)
{
    NanoProfilerGuard guard;

    NanoProfiler::BeginSample("spans the boundary");
    Spin(1000);

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);
    const NanoProfiler::Sample* sample = NanoProfiler::GetFrameSamples();
    EXPECT_GT(sample->end, sample->begin);

    // the dangling End lands in the new frame and must not corrupt it
    NanoProfiler::EndSample();

    {
        NanoProfiler::SampleScope scope("next frame");
    }

    NanoProfiler::NextFrame();

    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);
    EXPECT_EQ(NanoProfiler::GetFrameSamples()[0].parent, -1);
}

TEST(NanoProfiler, ResetDropsBothFrames)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope scope("work");
    }
    NanoProfiler::NextFrame();
    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);

    NanoProfiler::Reset();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);
    EXPECT_EQ(NanoProfiler::GetFrameDuration(), 0);
}

TEST(NanoProfiler, AggregationSubtractsChildrenFromTheirParent)
{
    NanoProfilerGuard guard;

    const int outer = NanoProfiler::BeginSample("outer");
    const int inner = NanoProfiler::BeginSample("inner");
    NanoProfiler::EndSample();
    NanoProfiler::EndSample();

    ASSERT_EQ(outer, 0);
    ASSERT_EQ(inner, 1);

    NanoProfiler::NextFrame();

    NanoProfiler::AggregatedSample aggregated[8];
    const int count = NanoProfiler::AggregateFrame(aggregated, 8);

    ASSERT_EQ(count, 2);

    const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
    const Int64 outerDuration = (Int64)samples[0].end - (Int64)samples[0].begin;
    const Int64 innerDuration = (Int64)samples[1].end - (Int64)samples[1].begin;

    EXPECT_EQ(AggregatedTime("outer", aggregated, count), outerDuration - innerDuration);
    EXPECT_EQ(AggregatedTime("inner", aggregated, count), innerDuration);
}

TEST(NanoProfiler, AggregationSumsScopesWithTheSameName)
{
    NanoProfilerGuard guard;

    for (int i = 0; i < 3; i++)
    {
        NanoProfiler::SampleScope scope("repeated");
        Spin(1000);
    }

    NanoProfiler::NextFrame();

    const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 3);

    Int64 total = 0;
    for (int i = 0; i < 3; i++)
        total += (Int64)samples[i].end - (Int64)samples[i].begin;

    NanoProfiler::AggregatedSample aggregated[8];
    const int count = NanoProfiler::AggregateFrame(aggregated, 8);

    ASSERT_EQ(count, 1);
    EXPECT_EQ(aggregated[0].time, total);
}

TEST(NanoProfiler, AggregationOverflowGoesIntoOther)
{
    NanoProfilerGuard guard;

    static const char* names[] = { "a", "b", "c", "d", "e" };
    for (auto name : names)
    {
        NanoProfiler::SampleScope scope(name);
        Spin(500);
    }

    NanoProfiler::NextFrame();

    // room for two named entries plus the trailing "Other"
    NanoProfiler::AggregatedSample aggregated[3];
    const int count = NanoProfiler::AggregateFrame(aggregated, 3);

    ASSERT_EQ(count, 3);
    EXPECT_STREQ(aggregated[0].name, "a");
    EXPECT_STREQ(aggregated[1].name, "b");
    EXPECT_EQ(aggregated[2].name, NanoProfiler::otherSampleName);
    EXPECT_GT(aggregated[2].time, 0);
}

TEST(NanoProfiler, AggregationNeedsRoomForOther)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope scope("only");
    }
    NanoProfiler::NextFrame();

    NanoProfiler::AggregatedSample aggregated[2];
    EXPECT_EQ(NanoProfiler::AggregateFrame(aggregated, 1), 0);
    EXPECT_EQ(NanoProfiler::AggregateFrame(aggregated, 2), 1);
}

TEST(NanoProfiler, SamplesOverTheCapacityAreCountedAsDropped)
{
    NanoProfilerGuard guard;

    const int extra = 16;
    for (int i = 0; i < NanoProfiler::maxFrameSamples + extra; i++)
    {
        NanoProfiler::SampleScope scope("flood");
    }

    NanoProfiler::NextFrame();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), NanoProfiler::maxFrameSamples);
    EXPECT_EQ(NanoProfiler::GetFrameDroppedSamples(), extra);
}

TEST(NanoProfiler, ScopesDeeperThanTheLimitAreDroppedAndUnwindCleanly)
{
    NanoProfilerGuard guard;

    const int depth = NanoProfiler::maxDepth + 4;
    for (int i = 0; i < depth; i++)
        NanoProfiler::BeginSample("deep");

    for (int i = 0; i < depth; i++)
        NanoProfiler::EndSample();

    // everything unwound, so the following scope is a root one again
    {
        NanoProfiler::SampleScope scope("after deep");
    }

    NanoProfiler::NextFrame();

    const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
    const int count = NanoProfiler::GetFrameSamplesCount();

    ASSERT_EQ(count, NanoProfiler::maxDepth + 1);
    EXPECT_STREQ(samples[count - 1].name, "after deep");
    EXPECT_EQ(samples[count - 1].parent, -1);
}

TEST(NanoProfiler, OtherThreadsDoNotDisturbTheBoundOne)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope scope("main thread work");
    }

    std::thread worker([]()
    {
        EXPECT_FALSE(NanoProfiler::IsThreadBound());

        for (int i = 0; i < 100; i++)
        {
            NanoProfiler::SampleScope scope("worker work");
        }
    });
    worker.join();

    NanoProfiler::NextFrame();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);
    EXPECT_NE(FindSample("main thread work"), nullptr);
}

TEST(NanoProfiler, FrameDurationCoversTheWholeFrame)
{
    NanoProfilerGuard guard;

    {
        NanoProfiler::SampleScope scope("work");
        Spin(100000);
    }

    NanoProfiler::NextFrame();

    const NanoProfiler::Sample* sample = NanoProfiler::GetFrameSamples();
    EXPECT_GE(NanoProfiler::GetFrameDuration(), (Int64)sample->end - (Int64)sample->begin);
}

#endif // O2_PROFILER_ENABLED
