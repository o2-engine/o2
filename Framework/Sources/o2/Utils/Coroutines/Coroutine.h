#pragma once

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

#include "o2/Utils/Coroutines/CoroutineControlBlock.h"
#include "o2/Utils/Threading/SharedRef.h"

namespace o2
{
    namespace Detail
    {
        // Result storage part of a coroutine promise. Non-void keeps the returned value; void keeps nothing
        template<typename _type>
        struct CoroutineResultStorage: public CoroutinePromiseBase
        {
            _type value{};
            void return_value(_type result) { value = std::move(result); }
            _type& Result() { return value; }
        };

        template<>
        struct CoroutineResultStorage<void>: public CoroutinePromiseBase
        {
            void return_void() {}
            void Result() {}
        };
    }

    // ---------------------------------------------------------------------------------------------
    // A coroutine handle and return type. Running through the job system, it can be started on the
    // main thread or the parallel workers, awaited from another coroutine (co_await), and holds an
    // atomically counted control block so the handle is safe to keep and drop on any thread.
    //
    // Coroutines start suspended: create one, then Start() it (or co_await / WaitAll it, which starts
    // it for you). The result type _type must be default-constructible (or void)
    // ---------------------------------------------------------------------------------------------
    template<typename _type = void>
    class Coroutine
    {
    public:
        // Promise type required by the C++ coroutine machinery
        struct promise_type: public Detail::CoroutineResultStorage<_type>
        {
            using Handle = std::coroutine_handle<promise_type>;

            // Creates the control block and the handle, returns the coroutine object
            Coroutine get_return_object()
            {
                auto controlBlock = MakeShared<CoroutineControlBlock>();
                controlBlock->handle = Handle::from_promise(*this);
                this->mControlBlock = controlBlock.Get();
                return Coroutine(controlBlock);
            }

            // Start suspended: the scheduler resumes the coroutine on the chosen thread
            std::suspend_always initial_suspend() noexcept { return {}; }

            // Awaiter used at the end: keeps the frame alive and fires the continuations
            struct FinalAwaiter
            {
                bool await_ready() const noexcept { return false; }
                void await_suspend(Handle handle) const noexcept { handle.promise().GetControlBlockPtr()->Complete(); }
                void await_resume() const noexcept {}
            };

            FinalAwaiter final_suspend() noexcept { return {}; }

            void unhandled_exception() { std::terminate(); }
        };

        using Handle = std::coroutine_handle<promise_type>;

        // Awaiter for co_await on a sub-coroutine
        struct SubCoroutineAwaiter
        {
            Coroutine child;

            bool await_ready() const { return child.IsDone(); }

            template<typename _promise>
            void await_suspend(std::coroutine_handle<_promise> parent) const
            {
                auto parentControlBlock = parent.promise().GetControlBlock();
                child.StartIfNeeded();
                child.GetControlBlock()->AddContinuation([parentControlBlock] {
                    ScheduleCoroutineResume(parentControlBlock);
                });
            }

            _type await_resume() const { return child.GetResult(); }
        };

    public:
        // Default constructor, holds no coroutine
        Coroutine() = default;

        // Constructor from a control block
        explicit Coroutine(const SharedRef<CoroutineControlBlock>& controlBlock): mControlBlock(controlBlock) {}

        // Returns true if it holds a coroutine
        bool IsValid() const { return mControlBlock.IsValid(); }

        // Returns true if the coroutine has finished
        bool IsDone() const { return mControlBlock && mControlBlock->IsDone(); }

        // Returns the shared control block
        SharedRef<CoroutineControlBlock> GetControlBlock() const { return mControlBlock; }

        // Starts the coroutine on the given thread and resume priority. No effect if already started
        Coroutine& Start(JobThread thread = JobThread::Any, JobPriority priority = JobPriority::Normal)
        {
            StartCoroutine(mControlBlock, thread, priority);
            return *this;
        }

        // Starts the coroutine if it wasn't started yet
        void StartIfNeeded(JobThread thread = JobThread::Any, JobPriority priority = JobPriority::Normal) const
        {
            StartCoroutine(mControlBlock, thread, priority);
        }

        // Blocks the calling thread until the coroutine finishes. Don't call it on a main-thread
        // coroutine from the main thread — its resumptions need the main thread to run
        void Wait() const
        {
            if (!mControlBlock)
                return;

            int state;
            while ((state = mControlBlock->done.Load()) == 0)
                mControlBlock->done.WaitWhileEquals(state);
        }

        // Returns the coroutine result. Valid only after it finished
        _type GetResult() const
        {
            auto handle = Handle::from_address(mControlBlock->handle.address());
            if constexpr (!std::is_void_v<_type>)
                return handle.promise().Result();
        }

        // co_await support: awaiting a coroutine starts it (if needed) and suspends the awaiter until
        // the coroutine finishes, then resumes on the awaiter's thread
        SubCoroutineAwaiter operator co_await() const { return SubCoroutineAwaiter{ *this }; }

    protected:
        SharedRef<CoroutineControlBlock> mControlBlock; // Shared coroutine state
    };
}
