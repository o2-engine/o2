#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/ConditionVariable.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/Thread.h"

namespace o2
{
    // ---------------------------------------------------------------------------------------------
    // Dedicated rendering thread. The main thread records a frame's draw commands and then hands a
    // "submit" callback to this thread, which runs the actual GPU submission (encode / draw calls /
    // present). The two threads rendezvous every frame: the main thread dispatches a frame and waits
    // for it to finish before starting the next one, so they wait for each other each frame
    // ---------------------------------------------------------------------------------------------
    class RenderThread
    {
    public:
        // Constructor
        RenderThread() = default;

        // Destructor, stops the thread
        ~RenderThread();

        // Starts the render thread
        void Start();

        // Stops and joins the render thread
        void Stop();

        // Returns true if the render thread is running
        bool IsRunning() const;

        // Dispatches a frame submit callback to the render thread. Blocks until the previous frame has
        // finished (rendezvous), then hands off this frame's work
        void DispatchFrame(const Function<void()>& submit);

        // Blocks until the currently dispatched frame has finished on the render thread
        void WaitFrameDone();

    protected:
        Thread            mThread;               // The render thread
        mutable Mutex     mMutex;                // Guards the handoff state
        ConditionVariable mWorkAvailable;        // Wakes the render thread when a frame is dispatched
        ConditionVariable mFrameDone;            // Signals the main thread when a frame finishes
        Function<void()>  mSubmit;               // Pending submit callback
        bool              mHasWork = false;      // True when a frame is waiting to be submitted
        bool              mIdle = true;          // True when no frame is in flight
        Atomic<bool>      mStopping{ false };    // True while shutting down
        bool              mStarted = false;      // True once the thread is running

    protected:
        // Render thread main loop
        void Loop();
    };
}
