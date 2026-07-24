#include "o2/stdafx.h"
#include "Thread.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace o2
{
    Thread::Thread(Thread&& other) noexcept:
        mThread(std::move(other.mThread))
    {}

    Thread& Thread::operator=(Thread&& other) noexcept
    {
        if (this != &other)
        {
            if (mThread.joinable())
                mThread.join();

            mThread = std::move(other.mThread);
        }

        return *this;
    }

    Thread::~Thread()
    {
        if (mThread.joinable())
            mThread.join();
    }

    bool Thread::IsJoinable() const
    {
        return mThread.joinable();
    }

    void Thread::Join()
    {
        if (mThread.joinable())
            mThread.join();
    }

    void Thread::Detach()
    {
        if (mThread.joinable())
            mThread.detach();
    }

    Thread::Id Thread::GetId() const
    {
        return mThread.get_id();
    }

    void Thread::SleepFor(float seconds)
    {
        std::this_thread::sleep_for(std::chrono::duration<float>(seconds));
    }

    void Thread::SleepForMilliseconds(long long milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void Thread::Yield()
    {
        std::this_thread::yield();
    }

    unsigned int Thread::HardwareConcurrency()
    {
        return std::thread::hardware_concurrency();
    }

    Thread::Id Thread::GetCurrentThreadId()
    {
        return std::this_thread::get_id();
    }

    void Thread::SetCurrentThreadName(const char* name)
    {
#if defined(_WIN32)
        // Convert to wide string for SetThreadDescription
        wchar_t wideName[256] = { 0 };
        int i = 0;
        for (; name[i] != '\0' && i < 255; i++)
            wideName[i] = (wchar_t)name[i];
        wideName[i] = L'\0';
        SetThreadDescription(GetCurrentThread(), wideName);
#elif defined(__APPLE__)
        pthread_setname_np(name);
#elif defined(__linux__) && !defined(__ANDROID__)
        pthread_setname_np(pthread_self(), name);
#else
        (void)name;
#endif
    }
}
