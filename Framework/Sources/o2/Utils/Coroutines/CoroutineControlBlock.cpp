#include "o2/stdafx.h"
#include "CoroutineControlBlock.h"

#include "o2/Utils/Jobs/JobSystem.h"

namespace o2
{
    void ScheduleCoroutineResume(const SharedRef<CoroutineControlBlock>& controlBlock)
    {
        JobThread   thread = (JobThread)controlBlock->resumeThread.Load();
        JobPriority priority = (JobPriority)controlBlock->resumePriority.Load();

        // The job captures a shared reference, keeping the coroutine alive across the async gap
        o2Jobs.Schedule([controlBlock] {
            if (controlBlock->handle && !controlBlock->handle.done())
            {
                // A named coroutine runs as its own Tracy fiber, so its zones follow it across worker
                // threads on a single track instead of scattering over whichever OS thread resumed it
                const char* fiber = controlBlock->fiberName;
                if (fiber)
                    PROFILE_FIBER_ENTER(fiber);

                {
                    PROFILE_SAMPLE("o2 Coroutine Resume");
                    controlBlock->handle.resume();
                }

                if (fiber)
                    PROFILE_FIBER_LEAVE();
            }
        }, priority, thread);
    }

    void StartCoroutine(const SharedRef<CoroutineControlBlock>& controlBlock, JobThread thread, JobPriority priority,
                        const char* fiberName)
    {
        if (!controlBlock)
            return;

        int expected = 0;
        if (controlBlock->started.CompareExchange(expected, 1))
        {
            controlBlock->resumeThread.Store((int)thread);
            controlBlock->resumePriority.Store((int)priority);
            controlBlock->fiberName = fiberName;
            ScheduleCoroutineResume(controlBlock);
        }
    }
}
