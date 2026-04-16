#include "o2/stdafx.h"
#include "Timer.h"

namespace o2
{
    Timer::Timer()
    {
        Reset();
    }

    Timer::~Timer()
    {}

#ifdef PLATFORM_WINDOWS
    void Timer::Reset()
    {
        QueryPerformanceFrequency(&mFrequency);
        QueryPerformanceCounter(&mStartTime);
        mLastElapsedTime = mStartTime.QuadPart;
    }

    float Timer::GetTime()
    {
        LARGE_INTEGER curTime;
        QueryPerformanceCounter(&curTime);

        float res = (float)((double)(curTime.QuadPart - (double)mStartTime.QuadPart)/(double)mFrequency.QuadPart);
        mLastElapsedTime = curTime.QuadPart;

        return res;
    }

    float Timer::GetDeltaTime()
    {
        LARGE_INTEGER curTime;
        QueryPerformanceCounter(&curTime);

        float res = (float)((double)(curTime.QuadPart - (double)mLastElapsedTime)/(double)mFrequency.QuadPart);
        mLastElapsedTime = curTime.QuadPart;

        return res;
    }
#endif

#if defined PLATFORM_ANDROID || defined PLATFORM_MAC || defined PLATFORM_IOS || defined PLATFORM_LINUX || defined PLATFORM_WASM
    void Timer::Reset()
    {
        gettimeofday(&mStartTime, NULL);
        gettimeofday(&mLastElapsedTime, NULL);
    }

    float Timer::GetTime()
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        float deltaTime = (now.tv_sec - mStartTime.tv_sec)*1000 + (now.tv_usec - mStartTime.tv_usec)/1000.0f;
        deltaTime /= 1000.0f;
        mLastElapsedTime = now;

        return deltaTime;
    }

    float Timer::GetDeltaTime()
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        float deltaTime = (now.tv_sec - mLastElapsedTime.tv_sec)*1000 + (now.tv_usec - mLastElapsedTime.tv_usec)/1000.0f;
        deltaTime /= 1000.0f;
        mLastElapsedTime = now;

        return deltaTime;
    }
#endif
}
