#include "o2Editor/stdafx.h"
#include "PipelineImageLoader.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/Types/WeakRef.h"

#include <chrono>
#include <map>
#include <thread>

namespace Editor
{
    static double NowMs()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    Ref<Bitmap> PipelineImageLoader::Request(const PipelineValue& value, const String& waiter)
    {
        if (!value.IsImage())
            return nullptr;

        value.ShareDecoded();
        auto& slot = value.decoded;
        if (slot->bitmap || slot->failed)
            return slot->bitmap;

        for (auto& j : mJobs)
        {
            if (j.slot != slot)
                continue;

            if (!j.waiters.Contains(waiter))
            {
                j.waiters.Add(waiter);
                mWaiting[waiter]++;
            }
            return nullptr;
        }

        Job newJob;
        newJob.work = std::make_shared<Work>();
        newJob.work->bytes = value.data;
        newJob.slot = slot;
        newJob.waiters.Add(waiter);
        mWaiting[waiter]++;
        slot->pending = true;
        mJobs.Add(newJob);

        std::shared_ptr<Work> work = newJob.work;
        o2Jobs.Schedule([work]()
        {
            auto bitmap = DecodeImageBytes(work->bytes);
            Ref<Bitmap> display;
            if (bitmap)
            {
                auto copy = PipelineDisplayCache::MakeDisplayCopy(*bitmap);
                if (copy != bitmap)
                    display = std::move(copy);
            }

            work->bitmap = std::move(bitmap);
            work->display = std::move(display);
            work->done.store(true, std::memory_order_release);
        }, JobPriority::Low, JobThread::Any);

        return nullptr;
    }

    bool PipelineImageLoader::IsPending(const PipelineValue& value) const
    {
        return value.decoded && value.decoded->pending;
    }

    bool PipelineImageLoader::IsWaiting(const String& waiter) const
    {
        return mWaiting.ContainsKey(waiter);
    }

    void PipelineImageLoader::Apply(Job& job, Vector<String>& ready)
    {
        job.slot->bitmap = std::move(job.work->bitmap);
        job.slot->failed = !job.slot->bitmap;
        job.slot->pending = false;
        if (job.slot->bitmap && job.work->display)
            PipelineDisplayCache::SetDisplayCopy(job.slot->bitmap, job.work->display);
        job.work->display = nullptr;

        for (auto& waiter : job.waiters)
        {
            if (!ready.Contains(waiter))
                ready.Add(waiter);

            auto it = mWaiting.find(waiter);
            if (it != mWaiting.end() && --it->second <= 0)
                mWaiting.erase(it);
        }
        mFinished++;
    }

    Vector<String> PipelineImageLoader::TakeReady(float budgetMs)
    {
        Vector<String> ready;
        double start = NowMs();
        for (int i = 0; i < mJobs.Count();)
        {
            if (!mJobs[i].work->done.load(std::memory_order_acquire))
            {
                i++;
                continue;
            }

            Apply(mJobs[i], ready);
            mJobs.RemoveAt(i);
            if (NowMs() - start > budgetMs)
                break;
        }

        if (mJobs.IsEmpty())
            mFinished = 0;

        return ready;
    }

    Vector<String> PipelineImageLoader::WaitAll()
    {
        Vector<String> ready;
        while (!mJobs.IsEmpty())
        {
            // A single-threaded build runs the decodes from the main-thread queue
            o2Jobs.ExecuteMainThreadJobs(-1.0f);
            for (auto& waiter : TakeReady(1000.0f))
            {
                if (!ready.Contains(waiter))
                    ready.Add(waiter);
            }
            if (!mJobs.IsEmpty())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ready;
    }

    namespace PipelineDisplayCache
    {
        struct Entry
        {
            WeakRef<Bitmap> source;  // The full picture, so a new bitmap at a freed address is not taken for it
            Ref<Bitmap>     display; // Copy made in the background, until the texture is made from it
            TextureRef      texture; // Texture of the card-sized copy
            Vec2I           size;    // Size of the card-sized copy
        };

        static std::map<const Bitmap*, Entry>& Entries()
        {
            static auto* entries = new std::map<const Bitmap*, Entry>();
            return *entries;
        }

        static Entry* Find(const Ref<Bitmap>& bitmap)
        {
            auto& entries = Entries();
            auto it = entries.find(bitmap.Get());
            if (it == entries.end())
                return nullptr;

            if (it->second.source.Lock() != bitmap)
            {
                entries.erase(it);
                return nullptr;
            }
            return &it->second;
        }

        static void Prune()
        {
            auto& entries = Entries();
            for (auto it = entries.begin(); it != entries.end();)
            {
                if (!it->second.source.IsValid())
                    it = entries.erase(it);
                else
                    ++it;
            }
        }

        Ref<Bitmap> MakeDisplayCopy(const Bitmap& bitmap)
        {
            Vec2I size = bitmap.GetSize();
            if (size.x <= maxSide && size.y <= maxSide)
                return Ref<Bitmap>(const_cast<Bitmap*>(&bitmap));

            float k = (float)maxSide/Math::Max(size.x, size.y);
            return bitmap.Resized(Vec2I(Math::Max(1, (int)(size.x*k)), Math::Max(1, (int)(size.y*k))));
        }

        TextureRef GetTexture(const Ref<Bitmap>& bitmap, Vec2I* size)
        {
            if (!bitmap)
                return TextureRef();

            auto entry = Find(bitmap);
            if (!entry)
            {
                static int inserts = 0;
                if (++inserts % 16 == 0)
                    Prune();

                Entry fresh;
                fresh.source = WeakRef<Bitmap>(bitmap);
                entry = &(Entries()[bitmap.Get()] = fresh);
            }

            if (!entry->texture)
            {
                Ref<Bitmap> display = entry->display ? entry->display : MakeDisplayCopy(*bitmap);
                entry->texture = TextureRef(*display);
                entry->size = display->GetSize();
                entry->display = nullptr;
            }

            if (size)
                *size = entry->size;

            return entry->texture;
        }

        void SetDisplayCopy(const Ref<Bitmap>& bitmap, const Ref<Bitmap>& display)
        {
            if (!bitmap || !display)
                return;

            auto entry = Find(bitmap);
            if (!entry)
            {
                Entry fresh;
                fresh.source = WeakRef<Bitmap>(bitmap);
                entry = &(Entries()[bitmap.Get()] = fresh);
            }

            if (!entry->texture)
                entry->display = display;
        }

        int GetCount()
        {
            Prune();
            return (int)Entries().size();
        }
    }
}
