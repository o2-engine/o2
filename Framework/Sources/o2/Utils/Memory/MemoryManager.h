#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <mutex>

#include "o2/EngineSettings.h"
#include "o2/Utils/Types/CommonTypes.h"

// Memory manager access macros
#define o2Memory MemoryManager::Instance()

// Managed new macros
#define mnew new(__FILE__, __LINE__)

// Managed malloc macros
#define mmalloc(SIZE) _mmalloc(SIZE, __FILE__, __LINE__)

// Managed free macros
#define mfree(MEMORY) _mfree(MEMORY)

// Overloaded managed new operator with source and line arguments
void* operator new(size_t size, const char* location, int line);

// Overloaded managed new[] operator with source and line arguments
void* operator new[](size_t size, const char* location, int line);

// Delete operator with source and line arguments
void  operator delete(void* allocMemory, const char* location, int line);

// Delete operator with source and line arguments
void  operator delete[](void* allocMemory, const char* location, int line);

// Overloaded delete operator
void  operator delete(void* allocMemory) noexcept;

// Overloaded delete operator
void  operator delete[](void* allocMemory) noexcept;

// Managed malloc() with source and line arguments
void* _mmalloc(size_t size, const char* location, int line);

// Managed free()
void _mfree(void* allocMemory);

namespace o2
{
    class LogStream;

    // ------------------------------------------------------------------
    // Memory manager, using for collecting garbage, tracing memory leaks
    // ------------------------------------------------------------------
    class MemoryManager
    {
    public:
        // Constructor
        MemoryManager();

        // Destructor
        ~MemoryManager();

        // Returns instance of memory manager
        static MemoryManager& Instance();

        // Initializes memory manager
        static void Initialize();

        // Collects information about allocated memory and prints into console
        void DumpInfo();

    protected:
        // ----------------------
        // Allocation information
        // ----------------------
        struct AllocInfo
        {
            const char* source;     // Allocation source code file
            int         sourceLine; // Allocation source code line
            size_t      size;       // Allocated size in bytes
            void*       memory;     // Pointer to allocated memory

            int markIndex = 0; // Mark index for memory analyzer
        };

        static MemoryManager* mInstance; // Instance pointer

        std::map<void*, AllocInfo> mAllocs;     // Allocations info
        size_t                     mTotalBytes; // Total managed allocated bytes

        // Guards mAllocs/mTotalBytes: allocations may come from worker threads. Recursive because the
        // overridden global operator delete re-enters OnMemoryRelease when the map frees a node.
        std::recursive_mutex mAllocsMutex;

    protected:
        // Called when memory was allocated and registers allocation
        void OnMemoryAllocate(void* memory, size_t size, const char* source, int line);

        // Called when memory releasing, unregisters allocation
        void OnMemoryRelease(void* memory);

        friend void* ::operator new(size_t size, const char* location, int line);
        friend void* ::operator new[](size_t size, const char* location, int line);
        friend void  ::operator delete(void* allocMemory) noexcept;
        friend void* ::_mmalloc(size_t size, const char* location, int line);
        friend void  ::_mfree(void* allocMemory);
        friend class MemoryAnalyzer;
    };
}
