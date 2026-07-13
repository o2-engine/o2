#pragma once

#include "o2/Utils/Types/CommonTypes.h"

#include <array>
#include <cstddef>
#include <vector>

namespace o2
{
    // -----------------------------------------------------------
    // Allocation header stored before each script container object
    // -----------------------------------------------------------
    struct alignas(std::max_align_t) ScriptContainerAllocationHeader
    {
        // Allocator size class index or direct-allocation marker.
        UInt16 sizeClass = 0;

        // Reserved for future allocator metadata.
        UInt16 reserved = 0;
    };

    // ------------------------------------
    // Free-list node for pooled allocation
    // ------------------------------------
    struct ScriptContainerFreeSlot
    {
        // Next free slot in the current size class.
        ScriptContainerFreeSlot* next = nullptr;
    };

    // ---------------------------------------
    // Single allocator bucket for one slot size
    // ---------------------------------------
    class ScriptContainerSizeClass
    {
    public:
        // Initializes size class for specified payload size.
        void Initialize(size_t payloadSize);

        // Preallocates first chunk for frequently used bucket.
        void WarmUp(size_t targetChunkSize, size_t minSlots);

        // Allocates memory slot from current size class.
        void* Allocate(size_t targetChunkSize, size_t minSlots);

        // Returns slot back into free list.
        void Free(void* slotMemory);

        // Releases all chunks owned by current size class.
        void ReleaseAllChunks();

    private:
        // Allocates one more chunk and appends its slots to the free list.
        void AddChunk(size_t targetChunkSize, size_t minSlots);

    private:
        // Maximum payload size supported by current bucket.
        size_t mPayloadSize = 0;

        // Full slot size including allocation header.
        size_t mSlotSize = 0;

        // Head of the free-list for this bucket.
        ScriptContainerFreeSlot* mFreeList = nullptr;

        // Owned raw chunk allocations.
        std::vector<void*> mChunks;
    };

    // ----------------------------------------------------
    // Small-object allocator for ScriptValue data containers
    // ----------------------------------------------------
    class ScriptContainerAllocator
    {
    public:
        static constexpr size_t SizeClassesCount = 9;

    public:
        // Initializes pooled size classes.
        ScriptContainerAllocator();

        // Releases all pooled chunks.
        ~ScriptContainerAllocator();

        // Returns shared allocator instance.
        static ScriptContainerAllocator& GetInstance();

        // Allocates container memory with requested size and alignment.
        void* Allocate(size_t size, size_t alignment);

        // Frees memory previously returned by Allocate().
        void Free(void* ptr);

    private:
        // Initializes all predefined size classes.
        void InitializeSizeClasses();

        // Returns index of first matching size class.
        static int FindSizeClass(size_t size);

    private:
        // Predefined pooled size classes.
        std::array<ScriptContainerSizeClass, SizeClassesCount> mSizeClasses;
    };
}
