#include "o2/stdafx.h"
#include "o2/Scripts/JerryScript/ScriptValueContainerAllocator.h"

#include <algorithm>
#include <array>
#include <limits>

namespace o2
{
    namespace
    {
        static constexpr UInt16 kDirectContainerAllocationClass = std::numeric_limits<UInt16>::max();
        static constexpr size_t kContainerChunkTargetSize = 16*1024;
        static constexpr size_t kContainerChunkMinSlots = 32;
        static constexpr std::array<size_t, ScriptContainerAllocator::SizeClassesCount> kContainerPayloadSizeClasses =
            { 32, 48, 64, 96, 128, 192, 256, 384, 512 };

        static size_t AlignUp(size_t value, size_t alignment)
        {
            size_t remainder = value % alignment;
            return remainder == 0 ? value : value + alignment - remainder;
        }
    }

    void ScriptContainerSizeClass::Initialize(size_t payloadSize)
    {
        mPayloadSize = payloadSize;
        mSlotSize = AlignUp(sizeof(ScriptContainerAllocationHeader) + mPayloadSize, alignof(std::max_align_t));
    }

    void ScriptContainerSizeClass::WarmUp(size_t targetChunkSize, size_t minSlots)
    {
        if (!mFreeList)
            AddChunk(targetChunkSize, minSlots);
    }

    void* ScriptContainerSizeClass::Allocate(size_t targetChunkSize, size_t minSlots)
    {
        if (!mFreeList)
            AddChunk(targetChunkSize, minSlots);

        auto* slot = mFreeList;
        mFreeList = slot->next;
        return slot;
    }

    void ScriptContainerSizeClass::Free(void* slotMemory)
    {
        auto* slot = reinterpret_cast<ScriptContainerFreeSlot*>(slotMemory);
        slot->next = mFreeList;
        mFreeList = slot;
    }

    void ScriptContainerSizeClass::ReleaseAllChunks()
    {
        for (auto* chunkMemory : mChunks)
            mfree(chunkMemory);

        mChunks.clear();
        mFreeList = nullptr;
    }

    void ScriptContainerSizeClass::AddChunk(size_t targetChunkSize, size_t minSlots)
    {
        size_t slotsPerChunk = std::max(minSlots, targetChunkSize/mSlotSize);
        void* chunkMemory = mmalloc(mSlotSize*slotsPerChunk);
        mChunks.push_back(chunkMemory);

        auto* chunkBytes = reinterpret_cast<std::byte*>(chunkMemory);
        for (size_t i = 0; i < slotsPerChunk; i++)
            Free(chunkBytes + i*mSlotSize);
    }

    ScriptContainerAllocator::ScriptContainerAllocator()
    {
        InitializeSizeClasses();
    }

    ScriptContainerAllocator::~ScriptContainerAllocator()
    {
        for (auto& sizeClass : mSizeClasses)
            sizeClass.ReleaseAllChunks();
    }

    ScriptContainerAllocator& ScriptContainerAllocator::GetInstance()
    {
        static auto* instance = mnew ScriptContainerAllocator();
        return *instance;
    }

    void* ScriptContainerAllocator::Allocate(size_t size, size_t alignment)
    {
        if (alignment <= alignof(std::max_align_t))
        {
            int sizeClassIdx = FindSizeClass(size);
            if (sizeClassIdx >= 0)
            {
                auto* header = reinterpret_cast<ScriptContainerAllocationHeader*>(
                    mSizeClasses[(size_t)sizeClassIdx].Allocate(kContainerChunkTargetSize, kContainerChunkMinSlots));
                header->sizeClass = (UInt16)sizeClassIdx;
                return header + 1;
            }
        }

        size_t allocationSize = AlignUp(sizeof(ScriptContainerAllocationHeader) + size, alignof(std::max_align_t));
        auto* header = reinterpret_cast<ScriptContainerAllocationHeader*>(mmalloc(allocationSize));
        header->sizeClass = kDirectContainerAllocationClass;
        return header + 1;
    }

    void ScriptContainerAllocator::Free(void* ptr)
    {
        if (!ptr)
            return;

        auto* header = reinterpret_cast<ScriptContainerAllocationHeader*>(ptr) - 1;
        if (header->sizeClass == kDirectContainerAllocationClass)
        {
            mfree(header);
            return;
        }

        mSizeClasses[header->sizeClass].Free(header);
    }

    void ScriptContainerAllocator::InitializeSizeClasses()
    {
        for (size_t i = 0; i < mSizeClasses.size(); i++)
        {
            mSizeClasses[i].Initialize(kContainerPayloadSizeClasses[i]);

            if (kContainerPayloadSizeClasses[i] <= 128)
                mSizeClasses[i].WarmUp(kContainerChunkTargetSize, kContainerChunkMinSlots);
        }
    }

    int ScriptContainerAllocator::FindSizeClass(size_t size)
    {
        for (size_t i = 0; i < kContainerPayloadSizeClasses.size(); i++)
        {
            if (size <= kContainerPayloadSizeClasses[i])
                return (int)i;
        }

        return -1;
    }
}
