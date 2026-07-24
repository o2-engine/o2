#include "o2/stdafx.h"
#include "RenderThread.h"

namespace o2
{
    RenderThread::~RenderThread()
    {
        Stop();
    }

    void RenderThread::Start()
    {
        if (mStarted)
            return;

        // No OS threads on this platform (single-threaded WebAssembly): stay unstarted so the caller keeps
        // rendering synchronously. Multithreaded render is also gated off there by IsMultithreadedRenderSupported
        if constexpr (!O2_HAS_THREADS)
            return;

        mStopping.Store(false);
        mHasWork = false;
        mIdle = true;
        mStarted = true;
        mThread = Thread([this] { Loop(); });
    }

    void RenderThread::Stop()
    {
        if (!mStarted)
            return;

        // Finish any in-flight frame first, then signal the loop to exit
        WaitFrameDone();

        {
            UniqueLock lock(mMutex);
            mStopping.Store(true);
        }
        mWorkAvailable.NotifyAll();

        mThread.Join();
        mStarted = false;
    }

    bool RenderThread::IsRunning() const
    {
        return mStarted;
    }

    void RenderThread::DispatchFrame(const Function<void()>& submit)
    {
        UniqueLock lock(mMutex);

        // Rendezvous: wait for the previous frame to finish before handing off a new one
        mFrameDone.Wait(lock, [this] { return mIdle; });

        mSubmit = submit;
        mHasWork = true;
        mIdle = false;
        mWorkAvailable.NotifyOne();
    }

    void RenderThread::WaitFrameDone()
    {
        UniqueLock lock(mMutex);
        mFrameDone.Wait(lock, [this] { return mIdle; });
    }

    void RenderThread::Loop()
    {
        Thread::SetCurrentThreadName("o2RenderThread");
        PROFILE_THREAD("o2 Render Thread");

        for (;;)
        {
            Function<void()> submit;
            {
                UniqueLock lock(mMutex);

                // The render thread has nothing to do between frames: the main thread only hands it a new
                // frame once per display refresh (vsync-paced), so this idle is the render pipeline waiting
                // on vsync. Shown as a distinct blue bar on the render thread's timeline
                {
                    PROFILE_SAMPLE_COLOR("Wait vsync", 0x2277BB);
                    mWorkAvailable.Wait(lock, [this] { return mHasWork || mStopping.Load(); });
                }

                if (mStopping.Load() && !mHasWork)
                    return;

                submit = mSubmit;
                mHasWork = false;
            }

            if (submit)
            {
                PROFILE_SAMPLE("o2 Render Submit");
                submit();
            }

            {
                UniqueLock lock(mMutex);
                mSubmit = Function<void()>();
                mIdle = true;
            }
            mFrameDone.NotifyAll();
        }
    }
}
