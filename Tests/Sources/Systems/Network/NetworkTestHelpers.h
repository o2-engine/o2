#pragma once

#include <chrono>

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/Threading/Thread.h"

namespace o2
{
    // Drives the network pump, main-thread jobs and frame wake-ups until the predicate holds or
    // the timeout expires. Stands in for the real frame loop that would pump these each frame
    template<typename _predicate>
    bool NetPumpUntil(const _predicate& predicate, float timeoutSeconds = 5.0f)
    {
        auto start = std::chrono::steady_clock::now();
        auto previous = start;
        while (!predicate())
        {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - previous).count();
            previous = now;

            o2Network.Update(dt);
            o2Jobs.ExecuteMainThreadJobs(-1.0f);
            o2Coroutines.OnNewFrame();

            float elapsed = std::chrono::duration<float>(now - start).count();
            if (elapsed > timeoutSeconds)
                return false;

            Thread::SleepForMilliseconds(1);
        }

        o2Network.Update(0.001f);
        o2Jobs.ExecuteMainThreadJobs(-1.0f);
        return true;
    }
}
