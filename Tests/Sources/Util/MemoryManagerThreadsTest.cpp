#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "o2/Utils/Memory/MemoryManager.h"

using namespace o2;

// Every managed allocation/release goes through MemoryManager from whatever thread made it:
// concurrent churn from several threads must keep it consistent
TEST(MemoryManagerThreads, ConcurrentAllocateAndReleaseStaysConsistent)
{
    const int threads = 8;
    const int iterations = 20000;

    std::atomic<int> started(0);
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; t++)
    {
        workers.emplace_back([&, t]()
        {
            started++;
            while (started < threads)
                std::this_thread::yield();

            std::vector<void*> live;
            live.reserve(64);

            unsigned int seed = 1234u + (unsigned int)t;
            for (int i = 0; i < iterations; i++)
            {
                seed = seed*1103515245u + 12345u;
                size_t size = 1 + (seed >> 8) % 256;

                live.push_back(mmalloc(size));

                if (live.size() > 32 || (seed & 1))
                {
                    size_t idx = (seed >> 4) % live.size();
                    mfree(live[idx]);
                    live[idx] = live.back();
                    live.pop_back();
                }

                // objects created with the managed new and freed with the global delete
                auto* object = mnew std::vector<int>(4, i);
                delete object;
            }

            for (auto ptr : live)
                mfree(ptr);
        });
    }

    for (auto& worker : workers)
        worker.join();

    // the manager itself must still be usable and report nothing from this test
    void* probe = mmalloc(16);
    mfree(probe);
    SUCCEED();
}
