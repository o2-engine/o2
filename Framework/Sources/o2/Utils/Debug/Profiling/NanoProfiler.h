#pragma once

// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Types/CommonTypes.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

namespace o2
{
#if defined(O2_PROFILER_ENABLED)
    // ------------------------------------------------------------------------------------------------------------
    // Hierarchical per-frame profiler. Scopes are recorded into a flat, preallocated array as (name, parent, begin,
    // end) tuples; the completed frame is published by a pointer swap, so a frame costs no allocation and no copy.
    //
    // Only the thread bound with BindThread() records — everything else takes a single null check and returns, so
    // worker threads can keep their PROFILE_SAMPLE macros without racing on the profiler state. Samples taken while
    // inside an editor scope are dropped: in the editor that leaves exactly the game update and the Game window
    // rendering, which is what the profiler widget shows
    // ------------------------------------------------------------------------------------------------------------
    class NanoProfiler
    {
    public:
        static constexpr int maxFrameSamples = 8192;      // Recorded samples per frame, extra ones are dropped
        static constexpr int maxDepth = 256;              // Nesting depth, scopes deeper than this are dropped
        static constexpr int maxAggregatedSamples = 512;  // Distinct names AggregateFrame can return

        static const char* const otherSampleName; // Name AggregateFrame gives to the entries that didn't fit

    public:
        // -------------------------------------------------------------------------------
        // Recorded scope. Times are nanoseconds from the frame start, end == begin when
        // the scope was still open when the frame ended
        // -------------------------------------------------------------------------------
        struct Sample
        {
            const char* name = nullptr; // Scope name, a literal or __PRETTY_FUNCTION__ with static lifetime
            int         parent = -1;    // Index of the enclosing recorded sample, -1 for a root one
            UInt32      begin = 0;      // Scope entering time
            UInt32      end = 0;        // Scope leaving time
        };

        // -----------------------------------------------------------------------
        // Frame scopes aggregated by name: the time spent in a scope itself, with
        // the time of its nested scopes subtracted
        // -----------------------------------------------------------------------
        struct AggregatedSample
        {
            const char* name = nullptr; // Scope name
            Int64       time = 0;       // Self time, nanoseconds
        };

        // ------------------------------------
        // RAII scope, samples its own lifetime
        // ------------------------------------
        struct SampleScope
        {
            explicit SampleScope(const char* name) { BeginSample(name); }
            ~SampleScope() { EndSample(); }
        };

        // ----------------------------------------------------------------------------------
        // Suspends recording inside its scope; used by the profiler widget to keep its own
        // drawing out of the measurements
        // ----------------------------------------------------------------------------------
        struct ExcludeScope
        {
            ExcludeScope();
            ~ExcludeScope();

        private:
            bool mWasRecording;
        };

    public:
        // Binds the calling thread as the recorded one and allocates its sample storage
        static void BindThread();

        // Unbinds the recorded thread and releases the sample storage
        static void UnbindThread();

        // Returns true when the calling thread is the recorded one
        static bool IsThreadBound();

        // Enables or disables recording; takes effect from the next frame
        static void SetEnabled(bool enabled);

        // Returns true when recording is enabled
        static bool IsEnabled();

        // Opens a scope, returns its sample index or -1 when it wasn't recorded
        static int BeginSample(const char* name);

        // Closes the innermost open scope
        static void EndSample();

        // Publishes the recorded scopes as the completed frame and starts a new one. Must be called
        // where no scope is open, otherwise the still-open ones are dropped
        static void NextFrame();

        // Drops the completed frame and the one being recorded
        static void Reset();

        // Returns the completed frame samples, ordered by their opening time
        static const Sample* GetFrameSamples();

        // Returns the completed frame samples count
        static int GetFrameSamplesCount();

        // Returns the completed frame duration in nanoseconds
        static Int64 GetFrameDuration();

        // Returns the samples count dropped from the completed frame by the capacity limits
        static int GetFrameDroppedSamples();

        // Writes the completed frame self times, aggregated by scope name, into the buffer and returns
        // the written entries count. Names that don't fit are summed into a trailing "Other" entry.
        // Allocation free: the working set lives in the bound thread's storage
        static int AggregateFrame(AggregatedSample* buffer, int bufferCapacity);

        // Outputs the completed frame self times, aggregated by scope name, into the log
        static void DumpLog();

    private:
        // ---------------------------------------------------------------------------
        // Sample storage of the recorded thread. Two buffers: one being filled and one
        // holding the completed frame, swapped by NextFrame
        // ---------------------------------------------------------------------------
        struct Storage
        {
            static constexpr int aggregationTableSize = 1024; // Power of two, must exceed maxAggregatedSamples

            Sample* current;   // Buffer of the frame being recorded
            Sample* completed; // Buffer of the last completed frame

            Int64*       selfTimes;         // AggregateFrame scratch: per sample self time
            const char** aggregationKeys;   // AggregateFrame scratch: open addressed name table
            int*         aggregationValues; // AggregateFrame scratch: index into the output buffer

            int count = 0;          // Samples recorded into current
            int completedCount = 0; // Samples in completed
            int dropped = 0;        // Samples the capacity limits dropped from current
            int completedDropped = 0;

            int top = -1;   // Index of the innermost open recorded sample
            int depth = 0;  // Open scopes count, including the not recorded ones

            bool recording = false; // Latched enabled state, changes only on a frame boundary

            Int64 frameStart = 0;        // Frame start time
            Int64 completedDuration = 0; // Completed frame duration

            bool recorded[maxDepth] = {}; // Per open scope: was it recorded, so EndSample knows what to pop

            Storage();
            ~Storage();
        };

    private:
        static Storage*              mStorage;       // Storage of the bound thread, owned
        static thread_local Storage* mThreadStorage; // Non-null only on the bound thread

        static bool mEnabled; // Recording enabled, latched into Storage::recording once per frame

    private:
        // Returns the monotonic clock in nanoseconds. Goes straight to the platform clock: it is read
        // twice per profiled scope, and the std::chrono wrappers around it are not free in debug builds
        static Int64 Now();
    };

    inline int NanoProfiler::BeginSample(const char* name)
    {
        Storage* storage = mThreadStorage;
        if (!storage)
            return -1;

        const int depth = storage->depth++;
        if (depth < maxDepth)
            storage->recorded[depth] = false;

        if (!storage->recording || EditorScope::IsInScope())
            return -1;

        if (depth >= maxDepth || storage->count >= maxFrameSamples)
        {
            storage->dropped++;
            return -1;
        }

        storage->recorded[depth] = true;

        Sample& sample = storage->current[storage->count];
        sample.name = name;
        sample.parent = storage->top;
        sample.begin = (UInt32)(Now() - storage->frameStart);
        sample.end = sample.begin;

        storage->top = storage->count++;

        return storage->top;
    }

    inline void NanoProfiler::EndSample()
    {
        Storage* storage = mThreadStorage;
        if (!storage || storage->depth <= 0)
            return;

        const int depth = --storage->depth;
        if (depth >= maxDepth || !storage->recorded[depth])
            return;

        Sample& sample = storage->current[storage->top];
        sample.end = (UInt32)(Now() - storage->frameStart);
        storage->top = sample.parent;
    }
#endif // O2_PROFILER_ENABLED

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
#if defined(TRACY_FIBERS) // Tracy only defines the fiber macros when built with fiber support
#define TRACY_PROFILE_FIBER_ENTER(name) TracyFiberEnter(name)
#define TRACY_PROFILE_FIBER_LEAVE() TracyFiberLeave
#else
#define TRACY_PROFILE_FIBER_ENTER(name)
#define TRACY_PROFILE_FIBER_LEAVE()
#endif
#else
#define TRACY_PROFILE_SAMPLE_FUNC()
#define TRACY_PROFILE_SAMPLE(id)
#define TRACY_PROFILE_SAMPLE_COLOR(id, color)
#define TRACY_PROFILE_INFO(info)
#define TRACY_PROFILE_FRAME()
#define TRACY_PROFILE_FRAME_NAMED(name)
#define TRACY_PROFILE_THREAD(name)
#define TRACY_PROFILE_FIBER_ENTER(name)
#define TRACY_PROFILE_FIBER_LEAVE()
#endif

#if defined(O2_PROFILER_ENABLED)
#define NANO_PROFILE_SAMPLE_FUNC() o2::NanoProfiler::SampleScope __nano_scope_sampler(__PRETTY_FUNCTION__)
#define NANO_PROFILE_SAMPLE(id) o2::NanoProfiler::SampleScope __nano_scope_sampler(id)
#define NANO_PROFILE_NEW_FRAME() o2::NanoProfiler::NextFrame()
#define NANO_PROFILE_BIND_THREAD() o2::NanoProfiler::BindThread()
#define NANO_PROFILE_UNBIND_THREAD() o2::NanoProfiler::UnbindThread()
#else
#define NANO_PROFILE_SAMPLE_FUNC()
#define NANO_PROFILE_SAMPLE(id)
#define NANO_PROFILE_NEW_FRAME()
#define NANO_PROFILE_BIND_THREAD()
#define NANO_PROFILE_UNBIND_THREAD()
#endif

#define PROFILE_SAMPLE_FUNC() TRACY_PROFILE_SAMPLE_FUNC(); NANO_PROFILE_SAMPLE_FUNC()
#define PROFILE_SAMPLE(id) TRACY_PROFILE_SAMPLE(id); NANO_PROFILE_SAMPLE(id)

// A named profiling zone with an explicit color (0xRRGGBB), shown as a distinct bar in the profiler
#define PROFILE_SAMPLE_COLOR(id, color) TRACY_PROFILE_SAMPLE_COLOR(id, color); NANO_PROFILE_SAMPLE(id)

#define PROFILE_INFO(info) TRACY_PROFILE_INFO(info)
#define PROFILE_FRAME() TRACY_PROFILE_FRAME()

// Closes the profiler frame and opens the next one. Must be placed where no profiling scope is open
#define PROFILE_NEW_FRAME() NANO_PROFILE_NEW_FRAME()

// Makes the calling thread the one the frame profiler records, and releases it back
#define PROFILE_BIND_THREAD() NANO_PROFILE_BIND_THREAD()
#define PROFILE_UNBIND_THREAD() NANO_PROFILE_UNBIND_THREAD()

// Marks a named frame boundary (e.g. a secondary thread's frame in Tracy)
#define PROFILE_FRAME_NAMED(name) TRACY_PROFILE_FRAME_NAMED(name)

// Names the current thread in the profiler (call once at thread start)
#define PROFILE_THREAD(name) TRACY_PROFILE_THREAD(name)

// Attributes the following zones to a named fiber (a logical thread that can migrate between OS threads,
// e.g. a coroutine) until PROFILE_FIBER_LEAVE. The name must be a persistent, unique string
#define PROFILE_FIBER_ENTER(name) TRACY_PROFILE_FIBER_ENTER(name)

// Returns zone attribution from the current fiber back to the OS thread
#define PROFILE_FIBER_LEAVE() TRACY_PROFILE_FIBER_LEAVE()

}
