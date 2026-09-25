#include "AMalloc.h"
#include "MallocTBB.h"
#include "MallocMimalloc.h"
#include "MallocTLSF.h"
#include "MallocAnsi.h"

#include "Runtime/BaseLib/include/LogService.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

NS_ALLOCATOR_BEGIN

void* UseSystemMallocForNew::operator new(size_t size)
{
	return ::malloc(size ? size : 1);
}

void* UseSystemMallocForNew::operator new[](size_t size)
{
	return ::malloc(size ? size : 1);
}

void* UseSystemMallocForNew::operator new(size_t size, const std::nothrow_t&) noexcept
{
	return ::malloc(size ? size : 1);
}

void* UseSystemMallocForNew::operator new[](size_t size, const std::nothrow_t&) noexcept
{
	return ::malloc(size ? size : 1);
}

void* UseSystemMallocForNew::operator new(size_t size, std::align_val_t alignment)
{
	return baselib::AlignedMalloc(size ? size : 1, static_cast<size_t>(alignment));
}

void* UseSystemMallocForNew::operator new[](size_t size, std::align_val_t alignment)
{
	return baselib::AlignedMalloc(size ? size : 1, static_cast<size_t>(alignment));
}

void* UseSystemMallocForNew::operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return baselib::AlignedMalloc(size ? size : 1, static_cast<size_t>(alignment));
}

void* UseSystemMallocForNew::operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return baselib::AlignedMalloc(size ? size : 1, static_cast<size_t>(alignment));
}

void UseSystemMallocForNew::operator delete(void* ptr) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, const std::nothrow_t&) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, size_t) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, size_t) noexcept
{
	::free(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, std::align_val_t) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, std::align_val_t) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, size_t, std::align_val_t) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, size_t, std::align_val_t) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete(void* ptr, size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
	baselib::AlignedFree(ptr);
}

void UseSystemMallocForNew::operator delete[](void* ptr, size_t, std::align_val_t, const std::nothrow_t&) noexcept
{
	baselib::AlignedFree(ptr);
}

namespace
{

constinit std::atomic<Malloc*> gBackend{ nullptr };

constinit std::atomic<bool> gSelectingBackend{ false };

Malloc* GetSystemMalloc()
{
    static Malloc* sAnsi = new MallocAnsi();
    return sAnsi;
}

Malloc* CreateBackendFromEnvironment()
{
    const char* name = ::getenv("GNX_ALLOCATOR");
    if (!name || !*name || ::strcmp(name, "tbb") == 0)
    {
        return new MallocTBB();
    }
    if (::strcmp(name, "ansi") == 0)
    {
        return GetSystemMalloc();
    }
    if (::strcmp(name, "mimalloc") == 0)
    {
        return new MallocMimalloc();
    }
    if (::strcmp(name, "tlsf") == 0)
    {
        return new MallocTLSF();
    }
    return nullptr;
}

struct AtomicStats
{
    std::atomic<uint64_t> allocationCount{ 0 };
    std::atomic<uint64_t> freeCount{ 0 };
    std::atomic<uint64_t> liveBytes{ 0 };
    std::atomic<uint64_t> peakLiveBytes{ 0 };
    std::atomic<uint64_t> foreignFreeCount{ 0 };
};

constinit AtomicStats gStats;

void RecordAllocation(size_t usableBytes)
{
    gStats.allocationCount.fetch_add(1, std::memory_order_relaxed);
    const uint64_t live = gStats.liveBytes.fetch_add(usableBytes, std::memory_order_relaxed) + usableBytes;

    uint64_t peak = gStats.peakLiveBytes.load(std::memory_order_relaxed);
    while (live > peak &&
           !gStats.peakLiveBytes.compare_exchange_weak(peak, live,
                                                       std::memory_order_relaxed,
                                                       std::memory_order_relaxed))
    {
    }
}

void RecordFree(size_t usableBytes)
{
    gStats.freeCount.fetch_add(1, std::memory_order_relaxed);

    uint64_t current = gStats.liveBytes.load(std::memory_order_relaxed);
    while (true)
    {
        const uint64_t next = (current > usableBytes) ? (current - usableBytes) : 0;
        if (gStats.liveBytes.compare_exchange_weak(current, next,
                                                   std::memory_order_relaxed,
                                                   std::memory_order_relaxed))
        {
            break;
        }
    }
}

size_t NormalizeAlignment(size_t alignment)
{
    if (alignment < DEFAULT_ALIGNMENT)
    {
        alignment = DEFAULT_ALIGNMENT;
    }

    size_t power = DEFAULT_ALIGNMENT;
    while (power < alignment)
    {
        const size_t next = power << 1;
        if (next <= power)
        {
            return alignment;
        }
        power = next;
    }
    return power;
}

}

MallocPtr Memory::GetMalloc()
{
    MallocPtr alloc = gBackend.load(std::memory_order_acquire);
    if (alloc)
    {
        return alloc;
    }

    bool expected = false;
    if (gSelectingBackend.compare_exchange_strong(expected, true,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire))
    {
        MallocPtr chosen = CreateBackendFromEnvironment();
        if (!chosen)
        {
            chosen = GetSystemMalloc();
        }
        gBackend.store(chosen, std::memory_order_release);
        gSelectingBackend.store(false, std::memory_order_release);
        return chosen;
    }

    return GetSystemMalloc();
}

void Memory::SetMalloc(MallocPtr allocator)
{
    gBackend.store(allocator, std::memory_order_release);
}

void Malloc::Free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    size_t usable = 0;
    if (!FreeAndGetSize(ptr, usable))
    {
        std::fprintf(stderr,
                     "[Allocator] %s::Free: 拒绝释放非本分配器的指针 %p（该块会被泄漏）\n",
                     GetDescriptiveName(), ptr);
        std::fflush(stderr);
        assert(false && "Malloc::Free: pointer is not owned by this allocator");
    }
}

void* Memory::Malloc(size_t size, size_t alignment)
{
    if (size == 0)
    {
        size = 1;
    }
    alignment = NormalizeAlignment(alignment);

    MallocPtr alloc = GetMalloc();

    size_t usable = 0;
    void* ptr = alloc->AlignedAlloc(size, alignment, &usable);
    if (ptr)
    {
        RecordAllocation(usable);
    }
    return ptr;
}

void Memory::Free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    MallocPtr alloc = GetMalloc();

    size_t usable = 0;
    if (alloc->FreeAndGetSize(ptr, usable))
    {
        RecordFree(usable);
        return;
    }

    gStats.foreignFreeCount.fetch_add(1, std::memory_order_relaxed);
    RecordFree(0);
    GetSystemMalloc()->Free(ptr);
}

size_t Memory::GetAllocSize(void* ptr)
{
    if (!ptr)
    {
        return 0;
    }

    MallocPtr alloc = GetMalloc();
    if (!alloc->OwnsPointer(ptr))
    {
        return 0;
    }

    size_t sizeOut = 0;
    if (!alloc->GetAllocationSize(ptr, sizeOut))
    {
        return 0;
    }
    return sizeOut;
}

const char* Memory::GetBackendName()
{
    MallocPtr alloc = GetMalloc();
    const char* name = alloc ? alloc->GetDescriptiveName() : nullptr;
    return name ? name : "Unknown";
}

void Memory::Trim(bool bTrimThreadCaches)
{
    MallocPtr alloc = GetMalloc();
    if (alloc)
    {
        alloc->Trim(bTrimThreadCaches);
    }
}

MemoryStats Memory::GetStats()
{
    MemoryStats snapshot;
    snapshot.allocationCount = gStats.allocationCount.load(std::memory_order_relaxed);
    snapshot.freeCount = gStats.freeCount.load(std::memory_order_relaxed);
    snapshot.liveBytes = gStats.liveBytes.load(std::memory_order_relaxed);
    snapshot.peakLiveBytes = gStats.peakLiveBytes.load(std::memory_order_relaxed);
    snapshot.foreignFreeCount = gStats.foreignFreeCount.load(std::memory_order_relaxed);

    snapshot.liveBlockCount = (snapshot.allocationCount > snapshot.freeCount)
                            ? (snapshot.allocationCount - snapshot.freeCount)
                            : 0;
    return snapshot;
}

void Memory::ResetStats()
{
    gStats.allocationCount.store(0, std::memory_order_relaxed);
    gStats.freeCount.store(0, std::memory_order_relaxed);
    gStats.liveBytes.store(0, std::memory_order_relaxed);
    gStats.peakLiveBytes.store(0, std::memory_order_relaxed);
    gStats.foreignFreeCount.store(0, std::memory_order_relaxed);
}

void Memory::LogBackendInfo()
{
    MallocPtr alloc = GetMalloc();
    LOG_INFO("[Allocator] backend = %s, threadSafe = %s",
             GetBackendName(),
             (alloc && alloc->IsThreadSafe()) ? "true" : "false");
}

void Memory::LogStats(const char* tag)
{
    const MemoryStats stats = GetStats();
    LOG_INFO("[Allocator] %s: backend=%s alloc=%llu free=%llu liveBlocks=%llu liveBytes=%llu peakBytes=%llu foreignFree=%llu",
             tag ? tag : "stats",
             GetBackendName(),
             (unsigned long long)stats.allocationCount,
             (unsigned long long)stats.freeCount,
             (unsigned long long)stats.liveBlockCount,
             (unsigned long long)stats.liveBytes,
             (unsigned long long)stats.peakLiveBytes,
             (unsigned long long)stats.foreignFreeCount);
}

NS_ALLOCATOR_END
