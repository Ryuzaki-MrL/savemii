#include <coreinit/dynload.h>
#include <coreinit/mutex.h>
#include <coreinit/memheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memory.h>
#include <coreinit/debug.h>

#define USE_DL_PREFIX
#include "malloc.h"

// 846 MB (used in SSB)
#define MAX_HEAP_SIZE 0x34e00000

static OSMutex malloc_lock;
static MEMHeapHandle mem2_handle;
static uint32_t mem2_size;
static void* custom_heap_base;
static uint32_t custom_heap_size;

static void *CustomAllocFromDefaultHeap(uint32_t size)
{
    OSLockMutex(&malloc_lock);
    void* ptr = dlmalloc(size);
    OSUnlockMutex(&malloc_lock);
    return ptr;
}

static void *CustomAllocFromDefaultHeapEx(uint32_t size, int32_t alignment)
{
    OSLockMutex(&malloc_lock);
    void* ptr = dlmemalign(alignment, size);
    OSUnlockMutex(&malloc_lock);
    return ptr;
}

static void CustomFreeToDefaultHeap(void* ptr)
{
    OSLockMutex(&malloc_lock);
    dlfree(ptr);
    OSUnlockMutex(&malloc_lock);
}

static OSDynLoad_Error CustomDynLoadAlloc(int32_t size, int32_t align, void **outAddr)
{
    if (!outAddr) {
        return OS_DYNLOAD_INVALID_ALLOCATOR_PTR;
    }

    if (align >= 0 && align < 4) {
        align = 4;
    } else if (align < 0 && align > -4) {
        align = -4;
    }

    if (!(*outAddr = MEMAllocFromDefaultHeapEx(size, align))) {
        return OS_DYNLOAD_OUT_OF_MEMORY;
    }

    return OS_DYNLOAD_OK;
}

static void CustomDynLoadFree(void *addr)
{
    MEMFreeToDefaultHeap(addr);
}

void *wiiu_morecore(long size)
{
    uint32_t old_size = custom_heap_size;
    uint32_t new_size = old_size + size;
    if (new_size > MAX_HEAP_SIZE) {
        return (void*) -1;
    }

    custom_heap_size = new_size;
    return ((uint8_t*) custom_heap_base) + old_size;
}

void __preinit_user(MEMHeapHandle *mem1, MEMHeapHandle *foreground, MEMHeapHandle *mem2)
{
    MEMAllocFromDefaultHeap = CustomAllocFromDefaultHeap;
    MEMAllocFromDefaultHeapEx = CustomAllocFromDefaultHeapEx;
    MEMFreeToDefaultHeap = CustomFreeToDefaultHeap;

    OSInitMutex(&malloc_lock);

    mem2_handle = *mem2;
    mem2_size = MEMGetAllocatableSizeForExpHeapEx(mem2_handle, 4);

    if (mem2_size < MAX_HEAP_SIZE) {
        OSFatal("Out of memory");
    }

    custom_heap_base = MEMAllocFromExpHeapEx(mem2_handle, MAX_HEAP_SIZE, 4);
    if (!custom_heap_base) {
        OSFatal("Cannot allocate custom heap");
    }

    custom_heap_size = 0;

    OSDynLoad_SetAllocator(CustomDynLoadAlloc, CustomDynLoadFree);
    OSDynLoad_SetTLSAllocator(CustomDynLoadAlloc, CustomDynLoadFree);
}