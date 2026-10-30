#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Debug/Profiling/NanoProfiler.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace o2;

namespace
{
    void SetWaitVariable(const char* value)
    {
#if defined(_WIN32)
        _putenv_s("O2_TRACY_WAIT", value ? value : "");
#else
        if (value)
            setenv("O2_TRACY_WAIT", value, 1);
        else
            unsetenv("O2_TRACY_WAIT");
#endif
    }

    // Sets O2_TRACY_WAIT for a test and puts the previous value back
    class WaitVariableGuard
    {
    public:
        explicit WaitVariableGuard(const char* value)
        {
            if (const char* previous = getenv("O2_TRACY_WAIT"))
                mPrevious = previous;

            SetWaitVariable(value);
        }

        ~WaitVariableGuard()
        {
            SetWaitVariable(mPrevious.empty() ? nullptr : mPrevious.c_str());
        }

    private:
        std::string mPrevious;
    };

    double MeasureWaitSeconds()
    {
        auto start = std::chrono::steady_clock::now();
        WaitForProfilerConnection();
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }
}

TEST(ProfilerConnection, WithoutTheVariableTheStartIsNotHeld)
{
    WaitVariableGuard guard(nullptr);
    EXPECT_LT(MeasureWaitSeconds(), 0.05);
}

#if defined(TRACY_ENABLE)
TEST(ProfilerConnection, GivesUpWhenNoProfilerConnectsInTime)
{
    if (TracyIsConnected)
        GTEST_SKIP() << "a profiler is connected to the tests";

    WaitVariableGuard guard("0.1");
    double waited = MeasureWaitSeconds();
    EXPECT_GE(waited, 0.09);
    EXPECT_LT(waited, 2.0);
}

TEST(ProfileZones, EndClosesTheLastOpenedZone)
{
    ASSERT_EQ(GetOpenProfileZonesCount(), 0);

    BeginProfileZone("outer", strlen("outer"));
    BeginProfileZone("inner", strlen("inner"));
    EXPECT_EQ(GetOpenProfileZonesCount(), 2);

    EndProfileZone();
    EXPECT_EQ(GetOpenProfileZonesCount(), 1);

    EndProfileZone();
    EndProfileZone();
    EXPECT_EQ(GetOpenProfileZonesCount(), 0);
}
#endif

#if defined(O2_PROFILER_ENABLED)
TEST(ProfileZones, NamedZoneIsRecordedUnderItsIdInTheFrameProfiler)
{
    NanoProfiler::UnbindThread();
    NanoProfiler::BindThread();
    NanoProfiler::SetEnabled(true);
    NanoProfiler::NextFrame();

    {
        PROFILE_SAMPLE_NAMED("named zone id", String("named ") + "zone");
    }

    NanoProfiler::NextFrame();

    bool found = false;
    for (int i = 0; i < NanoProfiler::GetFrameSamplesCount(); i++)
        found = found || strcmp(NanoProfiler::GetFrameSamples()[i].name, "named zone id") == 0;

    EXPECT_TRUE(found);

    NanoProfiler::SetEnabled(false);
    NanoProfiler::UnbindThread();
}
#endif
