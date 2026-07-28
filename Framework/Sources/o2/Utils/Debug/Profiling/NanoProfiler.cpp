// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#include "o2/stdafx.h"
#include "NanoProfiler.h"

#if defined(O2_PROFILER_ENABLED)

#include "o2/Utils/Debug/Debug.h"

#if defined(PLATFORM_WINDOWS)
#include <profileapi.h>
#else
#include <time.h>
#endif

namespace o2
{
    NanoProfiler::Storage* NanoProfiler::mStorage = nullptr;
    thread_local NanoProfiler::Storage* NanoProfiler::mThreadStorage = nullptr;
    bool NanoProfiler::mEnabled = false;

    const char* const NanoProfiler::otherSampleName = "Other";

#if defined(PLATFORM_WINDOWS)
    namespace
    {
        double QueryPerformanceTicksToNs()
        {
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);

            return 1.0e9/(double)frequency.QuadPart;
        }
    }
#endif

    Int64 NanoProfiler::Now()
    {
#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
        return (Int64)clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#elif defined(PLATFORM_WINDOWS)
        static const double ticksToNs = QueryPerformanceTicksToNs();

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);

        return (Int64)((double)counter.QuadPart*ticksToNs);
#else
        timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);

        return (Int64)time.tv_sec*1000000000ll + (Int64)time.tv_nsec;
#endif
    }

    NanoProfiler::Storage::Storage():
        current(mnew Sample[maxFrameSamples]), completed(mnew Sample[maxFrameSamples]),
        selfTimes(mnew Int64[maxFrameSamples]), aggregationKeys(mnew const char* [aggregationTableSize]),
        aggregationValues(mnew int[aggregationTableSize])
    {
        frameStart = Now();
    }

    NanoProfiler::Storage::~Storage()
    {
        delete[] current;
        delete[] completed;
        delete[] selfTimes;
        delete[] aggregationKeys;
        delete[] aggregationValues;
    }

    NanoProfiler::ExcludeScope::ExcludeScope():
        mWasRecording(mThreadStorage && mThreadStorage->recording)
    {
        if (mWasRecording)
            mThreadStorage->recording = false;
    }

    NanoProfiler::ExcludeScope::~ExcludeScope()
    {
        if (mWasRecording)
            mThreadStorage->recording = true;
    }

    void NanoProfiler::BindThread()
    {
        if (mThreadStorage)
            return;

        if (!mStorage)
            mStorage = mnew Storage();

        mThreadStorage = mStorage;
        mThreadStorage->recording = mEnabled;
    }

    void NanoProfiler::UnbindThread()
    {
        mThreadStorage = nullptr;

        delete mStorage;
        mStorage = nullptr;
    }

    bool NanoProfiler::IsThreadBound()
    {
        return mThreadStorage != nullptr;
    }

    void NanoProfiler::SetEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    bool NanoProfiler::IsEnabled()
    {
        return mEnabled;
    }

    void NanoProfiler::NextFrame()
    {
        Storage* storage = mThreadStorage;
        if (!storage)
            return;

        const Int64 now = Now();
        const UInt32 frameEnd = (UInt32)(now - storage->frameStart);

        // Scopes left open across the frame boundary are closed at it, so their time isn't lost
        for (int open = storage->top; open >= 0; open = storage->current[open].parent)
            storage->current[open].end = frameEnd;

        std::swap(storage->current, storage->completed);
        storage->completedCount = storage->count;
        storage->completedDropped = storage->dropped;
        storage->completedDuration = now - storage->frameStart;

        storage->count = 0;
        storage->dropped = 0;
        storage->top = -1;
        storage->depth = 0;
        storage->frameStart = now;
        storage->recording = mEnabled;
    }

    void NanoProfiler::Reset()
    {
        Storage* storage = mThreadStorage;
        if (!storage)
            return;

        storage->count = 0;
        storage->completedCount = 0;
        storage->dropped = 0;
        storage->completedDropped = 0;
        storage->completedDuration = 0;
        storage->top = -1;
        storage->depth = 0;
        storage->frameStart = Now();
    }

    const NanoProfiler::Sample* NanoProfiler::GetFrameSamples()
    {
        return mThreadStorage ? mThreadStorage->completed : nullptr;
    }

    int NanoProfiler::GetFrameSamplesCount()
    {
        return mThreadStorage ? mThreadStorage->completedCount : 0;
    }

    Int64 NanoProfiler::GetFrameDuration()
    {
        return mThreadStorage ? mThreadStorage->completedDuration : 0;
    }

    int NanoProfiler::GetFrameDroppedSamples()
    {
        return mThreadStorage ? mThreadStorage->completedDropped : 0;
    }

    int NanoProfiler::AggregateFrame(AggregatedSample* buffer, int bufferCapacity)
    {
        Storage* storage = mThreadStorage;
        if (!storage || bufferCapacity < 2)
            return 0;

        const int maxNamed = Math::Min(bufferCapacity - 1, maxAggregatedSamples);

        const Sample* samples = storage->completed;
        const int count = storage->completedCount;
        Int64* selfTimes = storage->selfTimes;

        // A child is always recorded after its parent, so its own entry is initialized before any child
        // subtracts from it
        for (int i = 0; i < count; i++)
        {
            const Int64 duration = (Int64)samples[i].end - (Int64)samples[i].begin;
            selfTimes[i] = duration;

            const int parent = samples[i].parent;
            if (parent >= 0)
                selfTimes[parent] -= duration;
        }

        constexpr int tableMask = Storage::aggregationTableSize - 1;
        const char** keys = storage->aggregationKeys;
        int* values = storage->aggregationValues;
        memset(keys, 0, sizeof(const char*)*Storage::aggregationTableSize);

        int written = 0;
        Int64 otherTime = 0;
        bool hasOther = false;

        for (int i = 0; i < count; i++)
        {
            const char* name = samples[i].name;

            // Names are compile time literals, so the pointer identifies them
            int slot = (int)(((UInt64)(uintptr_t)name >> 4)*2654435761u) & tableMask;
            while (keys[slot] && keys[slot] != name)
                slot = (slot + 1) & tableMask;

            if (keys[slot])
            {
                buffer[values[slot]].time += selfTimes[i];
                continue;
            }

            if (written >= maxNamed)
            {
                otherTime += selfTimes[i];
                hasOther = true;
                continue;
            }

            keys[slot] = name;
            values[slot] = written;

            buffer[written].name = name;
            buffer[written].time = selfTimes[i];
            written++;
        }

        if (hasOther)
        {
            buffer[written].name = otherSampleName;
            buffer[written].time = otherTime;
            written++;
        }

        return written;
    }

    void NanoProfiler::DumpLog()
    {
        static AggregatedSample aggregated[maxAggregatedSamples];
        const int count = AggregateFrame(aggregated, maxAggregatedSamples);

        o2Debug.Log("---- Nano profiler dump, %i samples in %.3f ms:", GetFrameSamplesCount(),
                    (double)GetFrameDuration()*1e-6);

        std::sort(aggregated, aggregated + count,
                  [](const AggregatedSample& a, const AggregatedSample& b) { return a.time > b.time; });

        for (int i = 0; i < count; i++)
            o2Debug.Log("   %.3f ms: %s", (double)aggregated[i].time*1e-6, aggregated[i].name);
    }
}

#endif // O2_PROFILER_ENABLED
