#pragma once

#if defined PLATFORM_WINDOWS
#include <Windows.h>
#elif defined PLATFORM_ANDROID || defined PLATFORM_MAC || defined PLATFORM_IOS || defined PLATFORM_LINUX || defined PLATFORM_WASM
#include <sys/time.h>
#endif

#include <stdio.h>

namespace o2
{
    // ------------------------------------
    // Timer class, using for counting time
    // ------------------------------------
    class Timer
    {
    public:
        // Default constructor
        Timer();

        // Destructor
        ~Timer();

        // Resets time
        void Reset();

        // Returns time in seconds from last Reset() call
        float GetTime();

        // Returns time in seconds from last Reset() or GetElapsedTime() call
        float GetDeltaTime();

    protected:
#if defined PLATFORM_WINDOWS
        LONGLONG      mLastElapsedTime;
        LARGE_INTEGER mFrequency;
        LARGE_INTEGER mStartTime;
#elif defined PLATFORM_ANDROID || defined PLATFORM_MAC|| defined PLATFORM_IOS || defined PLATFORM_LINUX || defined PLATFORM_WASM
        struct timeval mLastElapsedTime;
        struct timeval mStartTime;
#endif
    };

    class ScopeTimer
    {
    public:
        const char* id = "";
        Timer timer;
        
        ScopeTimer() {}
        ScopeTimer(const char* id):id(id) {}
        ~ScopeTimer() { Check(); }

        void Check() { printf("%s - %.4f sec\n", id, timer.GetDeltaTime()); }
    };
}
