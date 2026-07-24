#pragma once
#include <unordered_map>

#include "o2/Utils/Types/Containers/Pair.h"
#include "o2/Utils/System/Time/Timer.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

namespace o2
{    
    class SimpleProfiler
    {
    public:
        struct ScopeSampler
        {
            const char* id;
            float begin, end;

            ScopeSampler(const char* id): 
                id(id), begin(SimpleProfiler::GetProfileTime())
            {}

            ~ScopeSampler()
            {
                end = SimpleProfiler::GetProfileTime();
                SimpleProfiler::Sample(id, begin, end);
            }
        };

        struct ZoneSampler
        {
            const char* id;
            float begin, end;

            ZoneSampler(const char* id) :
                id(id), begin(SimpleProfiler::GetProfileTime())
            {}

            void End()
            {
                end = SimpleProfiler::GetProfileTime();
                SimpleProfiler::Sample(id, begin, end);
            }
        };

        struct SampleInterval
        {
            const char* id;
            float begin, end;

            SampleInterval(const char* id, float begin, float end) :
                id(id), begin(begin), end(end)
            {}
        };

    public:
        static void Reset();
        static void Flush();

        static void DumpLog();

        static void Sample(const char* id, float start, float end);

        static const Vector<SampleInterval>& GetSamples();
        static const std::unordered_map<const char*, Pair<float, int>>& GetAccumulatedSamples();
        static int GetAccumulatedSamplesCount();

        static float GetProfileTime();

    private:
        static Timer mTimer;
        static Vector<SampleInterval> mSamples;
        static std::unordered_map<const char*, Pair<float, int>> mAccumulatedSamples;
        static int mAccumulatedSamplesCount;
    };

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#if defined(TRACY_ENABLE)
#define TRACY_PROFILE_SAMPLE_FUNC() ZoneScoped
#define TRACY_PROFILE_SAMPLE(id) ZoneScopedN(id)
#define TRACY_PROFILE_SAMPLE_COLOR(id, color) ZoneScopedNC(id, color)
#define TRACY_PROFILE_INFO(info) ZoneText(info, info.Length())
#define TRACY_PROFILE_FRAME() FrameMark
#define TRACY_PROFILE_FRAME_NAMED(name) FrameMarkNamed(name)
#define TRACY_PROFILE_THREAD(name) tracy::SetThreadName(name)
#else
#define TRACY_PROFILE_SAMPLE_FUNC()
#define TRACY_PROFILE_SAMPLE(id)
#define TRACY_PROFILE_SAMPLE_COLOR(id, color)
#define TRACY_PROFILE_INFO(info)
#define TRACY_PROFILE_FRAME()
#define TRACY_PROFILE_FRAME_NAMED(name)
#define TRACY_PROFILE_THREAD(name)
#endif

#if defined(O2_PROFILE_STATS)
#define SIMPLE_PROFILE_SAMPLE_FUNC() o2::SimpleProfiler::ScopeSampler __scope_sampler(__PRETTY_FUNCTION__)
#define SIMPLE_PROFILE_SAMPLE(id) o2::SimpleProfiler::ScopeSampler __scope_sampler(id)
#define SIMPLE_PROFILE_INFO(info) 
#else
#define SIMPLE_PROFILE_SAMPLE_FUNC() 
#define SIMPLE_PROFILE_SAMPLE(id) 
#define SIMPLE_PROFILE_INFO(info)
#endif 

#define PROFILE_SAMPLE_FUNC() TRACY_PROFILE_SAMPLE_FUNC(); SIMPLE_PROFILE_SAMPLE_FUNC()
#define PROFILE_SAMPLE(id) TRACY_PROFILE_SAMPLE(id); SIMPLE_PROFILE_SAMPLE(id)

// A named profiling zone with an explicit color (0xRRGGBB), shown as a distinct bar in the profiler
#define PROFILE_SAMPLE_COLOR(id, color) TRACY_PROFILE_SAMPLE_COLOR(id, color)

#define PROFILE_INFO(info) TRACY_PROFILE_INFO(info); SIMPLE_PROFILE_INFO(info)
#define PROFILE_FRAME() TRACY_PROFILE_FRAME()

// Marks a named frame boundary (e.g. a secondary thread's frame in Tracy)
#define PROFILE_FRAME_NAMED(name) TRACY_PROFILE_FRAME_NAMED(name)

// Names the current thread in the profiler (call once at thread start)
#define PROFILE_THREAD(name) TRACY_PROFILE_THREAD(name)

}
