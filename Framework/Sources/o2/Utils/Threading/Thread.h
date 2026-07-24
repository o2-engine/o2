#pragma once

#include <thread>
#include <chrono>
#include <utility>

// Whether the platform can create OS threads. It is 0 on Emscripten builds without pthread support
// (single-threaded WebAssembly, no Web Workers): there the job / coroutine / render systems fall back
// to cooperative execution on the main thread instead of spawning threads
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
#   define O2_HAS_THREADS 0
#else
#   define O2_HAS_THREADS 1
#endif

namespace o2
{
    // ----------------------------------------------------------------------------------
    // Thread wrapper over std::thread with CamelCase o2 API. Auto-joins on destruction to
    // avoid std::terminate (jthread-like). Non-copyable, movable
    // ----------------------------------------------------------------------------------
    class Thread
    {
    public:
        typedef std::thread::id Id;

    public:
        // Default constructor, doesn't start any thread
        Thread() = default;

        // Constructor that immediately starts a thread running function with arguments
        template<typename _fn_type, typename ... _args>
        explicit Thread(_fn_type&& function, _args&& ... args);

        // Move constructor
        Thread(Thread&& other) noexcept;

        // Move operator
        Thread& operator=(Thread&& other) noexcept;

        Thread(const Thread& other) = delete;
        Thread& operator=(const Thread& other) = delete;

        // Destructor, joins the thread if it is still joinable
        ~Thread();

        // Returns true if the thread is running and wasn't joined or detached
        bool IsJoinable() const;

        // Waits for the thread to finish its execution
        void Join();

        // Detaches the thread, letting it run independently
        void Detach();

        // Returns thread identifier
        Id GetId() const;

        // Sleeps current thread for the given time in seconds
        static void SleepFor(float seconds);

        // Sleeps current thread for the given time in milliseconds
        static void SleepForMilliseconds(long long milliseconds);

        // Yields execution of the current thread to another ready thread
        static void Yield();

        // Returns number of hardware threads (cores) available, or 0 if unknown
        static unsigned int HardwareConcurrency();

        // Returns identifier of the current thread
        static Id GetCurrentThreadId();

        // Sets the debug name of the current thread (visible in profilers/debuggers)
        static void SetCurrentThreadName(const char* name);

    protected:
        std::thread mThread; // Wrapped standard thread
    };

    template<typename _fn_type, typename ... _args>
    Thread::Thread(_fn_type&& function, _args&& ... args):
        mThread(std::forward<_fn_type>(function), std::forward<_args>(args)...)
    {}
}
