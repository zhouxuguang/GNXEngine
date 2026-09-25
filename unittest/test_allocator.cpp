//
//  test_allocator.cpp
//  GNXEngine
//
//  可插拔内存分配器（内存池）的功能 / 对齐 / 扩容 / 并发 / 边界回归测试。
//
//  覆盖四个后端：MallocAnsi（系统）、MallocTBB、MallocMimalloc、MallocTLSF（内存池）。
//  这些用例在修复前应当是失败（部分为静默返回 nullptr 或死循环）的，修复后必须全部通过。
//
//  与 doc/BugAuditReport.md 的关系：本期修复的分配器缺陷集中在本文件回归。
//

#include <catch2/catch_test_macros.hpp>

#include "Runtime/Allocator/include/AMalloc.h"
#include "Runtime/Allocator/source/MallocAnsi.h"
#include "Runtime/Allocator/source/MallocTBB.h"
#include "Runtime/Allocator/source/MallocMimalloc.h"
#include "Runtime/Allocator/source/MallocTLSF.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using namespace Allocator;

namespace
{

enum class Backend
{
    Ansi,
    Tbb,
    Mimalloc,
    Tlsf
};

const char* BackendName(Backend backend)
{
    switch (backend)
    {
    case Backend::Ansi:     return "ANSI";
    case Backend::Tbb:      return "TBB";
    case Backend::Mimalloc: return "Mimalloc";
    case Backend::Tlsf:     return "TLSF";
    }
    return "Unknown";
}

Malloc* CreateBackend(Backend backend)
{
    switch (backend)
    {
    case Backend::Ansi:     return new MallocAnsi();
    case Backend::Tbb:      return new MallocTBB();
    case Backend::Mimalloc: return new MallocMimalloc();
    case Backend::Tlsf:     return new MallocTLSF();
    }
    return nullptr;
}

const Backend kAllBackends[] = {
    Backend::Ansi, Backend::Tbb, Backend::Mimalloc, Backend::Tlsf
};

// 每个用例使用独立的后端实例：TLSF 会为每个实例申请自己的池，
// 不复用实例可以避免用例之间互相干扰。
class MallocOwner
{
public:
    explicit MallocOwner(Backend backend) : mAlloc(CreateBackend(backend)) {}
    // 通过基类指针删除：依赖 Malloc 的虚析构函数，否则 MallocTLSF 的池永不释放
    ~MallocOwner() { delete mAlloc; }

    MallocOwner(const MallocOwner&) = delete;
    MallocOwner& operator=(const MallocOwner&) = delete;

    Malloc* Get() const { return mAlloc; }
    Malloc* operator->() const { return mAlloc; }

private:
    Malloc* mAlloc;
};

// 校验整块内存都是 expected byte（用一次 REQUIRE，避免逐字节断言产生上万条断言）
bool FilledWith(const void* ptr, size_t size, uint8_t expected)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        if (bytes[i] != expected)
        {
            return false;
        }
    }
    return true;
}

// 用于验证「基类必须具备虚析构函数」的探针
struct DestructorProbe : public Malloc
{
    explicit DestructorProbe(bool* destroyed) : mDestroyed(destroyed) {}
    ~DestructorProbe() override
    {
        if (mDestroyed)
        {
            *mDestroyed = true;
        }
    }

    void* Alloc(size_t, size_t*) override { return nullptr; }
    void* AlignedAlloc(size_t, size_t, size_t*) override { return nullptr; }
    bool FreeAndGetSize(void*, size_t&) override { return false; }
    bool GetAllocationSize(void*, size_t&) override { return false; }
    bool OwnsPointer(const void*) const override { return false; }
    void Trim(bool) override {}
    bool IsThreadSafe() const override { return true; }
    const char* GetDescriptiveName() const override { return "Probe"; }

private:
    bool* mDestroyed;
};

// Catch2 没有内建超时。分配器历史上最危险的缺陷形态是「扩容死循环」：
// 一旦回归，测试进程会不停 malloc 直到被 OOM 杀掉 —— 那是不可诊断的失败。
// 看门狗把「疑似死循环」变成一次立刻可见、可定位的 abort。
class Watchdog
{
public:
    explicit Watchdog(unsigned seconds)
        : mSeconds(seconds), mDone(false)
    {
        mThread = std::thread([this]()
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(mSeconds);
            std::unique_lock<std::mutex> lock(mMutex);
            if (!mCv.wait_until(lock, deadline, [this]() { return mDone; }))
            {
                std::fprintf(stderr,
                             "[Allocator][watchdog] 用例超过 %u 秒未结束：疑似分配器扩容死循环/无界增长\n",
                             mSeconds);
                std::fflush(stderr);
                std::abort();
            }
        });
    }

    ~Watchdog()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDone = true;
        }
        mCv.notify_all();
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

private:
    unsigned                mSeconds;
    bool                    mDone;
    std::mutex              mMutex;
    std::condition_variable mCv;
    std::thread             mThread;
};

struct LiveBlock
{
    void*   ptr;
    size_t  size;
    uint8_t tag;
};

} // namespace

// ===========================================================================
// Allocator 缺陷-A  Malloc 基类虚析构函数缺失
//
// 位置：Engine/Runtime/Allocator/include/AMalloc.h
// 问题：Malloc 声明了虚函数却没有虚析构函数。MallocTLSF 的析构会释放它申请的所有
//       memory pool（初始 2MB + 每次扩容 100MB），但通过 Malloc* 删除时
//       ~MallocTLSF 永远不会执行 —— 池内存全部泄漏。
// ===========================================================================
TEST_CASE("分配器基类必须具备虚析构函数", "[allocator][bugfix]")
{
    bool destroyed = false;
    Malloc* probe = new DestructorProbe(&destroyed);
    delete probe;
    REQUIRE(destroyed);
}

// ===========================================================================
// Allocator 缺陷-B  MallocTLSF 的析构必须真正释放池
//
// 修复前：同上，池内存泄漏（不可观测）；修复后：至少保证「构造-分配-释放-析构」
// 全流程不崩溃，并且析构路径可达。
// ===========================================================================
TEST_CASE("MallocTLSF 通过基类指针析构不崩溃", "[allocator][tlsf]")
{
    Malloc* alloc = new MallocTLSF();
    void* p = alloc->Alloc(1024);
    REQUIRE(p != nullptr);
    alloc->Free(p);
    delete alloc;
}

// ===========================================================================
// Allocator 缺陷-C  MallocTLSF::AlignedAlloc 池耗尽后不扩容
//
// 位置：Engine/Runtime/Allocator/source/MallocTLSF.cpp
// 问题：只有 Alloc 在 tlsf_memalign 失败后追加池，AlignedAlloc 直接返回 nullptr。
//       而 Memory::Malloc 走的正是 AlignedAlloc —— 初始池只有 2MB，
//       任何超过池容量的请求都会拿到 nullptr，调用方随即空指针解引用崩溃。
// 复现：分配 8MB（> 2MB 初始池）并对整块内存写入。
// ===========================================================================
TEST_CASE("每个后端都能完成超过初始池容量的大块对齐分配", "[allocator]")
{
    const size_t bigSize = 8 * 1024 * 1024;
    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        void* ptr = owner->AlignedAlloc(bigSize, 64);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);

        // 触碰全部页面：既验证内存真实可用，也能暴露「返回的地址不可写」
        memset(ptr, 0x11, bigSize);
        REQUIRE(FilledWith(ptr, bigSize, 0x11));

        size_t allocSize = 0;
        REQUIRE(owner->GetAllocationSize(ptr, allocSize));
        REQUIRE(allocSize >= bigSize);

        owner->Free(ptr);
    }
}

// ===========================================================================
// Allocator 缺陷-D  MallocTLSF 扩容失败时死循环
//
// 位置：Engine/Runtime/Allocator/source/MallocTLSF.cpp
// 问题：`while (!newPtr) { malloc(100MB); tlsf_add_pool(...); ... }` 既没有判断
//       malloc / tlsf_add_pool 是否失败，也没有「请求本身就不可满足」的出口。
//       请求超过 TLSF 单池上限（64 位下 tlsf_block_size_max() == 4GB）时，
//       循环会不停 malloc(100MB) 直到进程被 OOM 杀掉。
// 修复：预判不可满足的请求（size > tlsf_block_size_max()）立即返回 nullptr。
// ===========================================================================
TEST_CASE("MallocTLSF 超大请求必须返回 nullptr 而不是无限扩容", "[allocator][tlsf][bugfix]")
{
    MallocTLSF alloc;

    // 16GB > tlsf_block_size_max()(4GB)：加多少池都不可能满足
    void* ptr = alloc.Alloc(static_cast<size_t>(1) << 34);
    REQUIRE(ptr == nullptr);

    // 对齐路径同理
    ptr = alloc.AlignedAlloc(static_cast<size_t>(1) << 34, 4096);
    REQUIRE(ptr == nullptr);

    // 分配器仍可正常工作（失败不得破坏内部状态）
    void* ok = alloc.Alloc(1024);
    REQUIRE(ok != nullptr);
    alloc.Free(ok);
}

// ===========================================================================
// 基础功能：分配 / 写入 / 查询可用大小 / 释放（四后端）
// ===========================================================================
TEST_CASE("所有后端的基础分配与释放", "[allocator]")
{
    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);
        REQUIRE(owner.Get() != nullptr);
        REQUIRE(owner->GetDescriptiveName() != nullptr);

        const size_t size = 4096;
        void* ptr = owner->Alloc(size);
        REQUIRE(ptr != nullptr);

        memset(ptr, 0x5A, size);
        REQUIRE(FilledWith(ptr, size, 0x5A));

        size_t allocSize = 0;
        REQUIRE(owner->GetAllocationSize(ptr, allocSize));
        // GetAllocationSize 返回的是「可用大小」：>= 请求大小（见 BUG-14）
        REQUIRE(allocSize >= size);

        owner->Free(ptr);
    }
}

// ===========================================================================
// 对齐：AlignedAlloc 必须满足请求的对齐，且内容可读写
// ===========================================================================
TEST_CASE("所有后端的对齐分配", "[allocator]")
{
    const size_t alignments[] = {16, 32, 64, 256, 4096};
    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        for (size_t alignment : alignments)
        {
            INFO("alignment = " << alignment);
            const size_t size = 1024;
            void* ptr = owner->AlignedAlloc(size, alignment);
            REQUIRE(ptr != nullptr);
            REQUIRE(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);

            memset(ptr, 0x3C, size);
            REQUIRE(FilledWith(ptr, size, 0x3C));

            size_t allocSize = 0;
            REQUIRE(owner->GetAllocationSize(ptr, allocSize));
            REQUIRE(allocSize >= size);

            owner->Free(ptr);
        }
    }
}

// ===========================================================================
// 扩容 + 复用：持续分配累计超过初始池容量（TLSF 初始池 2MB）
// ===========================================================================
TEST_CASE("池耗尽后可继续分配并正确复用", "[allocator]")
{
    const size_t blockSize = 64 * 1024;
    const int    blockCount = 64;   // 累计 4MB > 2MB 初始池

    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        std::vector<void*> blocks;
        blocks.reserve(blockCount);
        for (int i = 0; i < blockCount; ++i)
        {
            void* ptr = owner->Alloc(blockSize);
            REQUIRE(ptr != nullptr);
            // 每块用不同内容，避免「同一块被重复分配」被掩盖
            memset(ptr, static_cast<uint8_t>(i), blockSize);
            blocks.push_back(ptr);
        }

        for (int i = 0; i < blockCount; ++i)
        {
            REQUIRE(FilledWith(blocks[static_cast<size_t>(i)], blockSize, static_cast<uint8_t>(i)));
        }

        for (void* ptr : blocks)
        {
            owner->Free(ptr);
        }

        // 释放后应能重新拿到同样规模的内存
        void* again = owner->Alloc(blockSize * blockCount);
        REQUIRE(again != nullptr);
        owner->Free(again);
    }
}

// ===========================================================================
// 伪随机压力：混合大小 / 混合对齐的分配与释放，并校验内容不被破坏
//
// 用于暴露 free list / 位图 / 元数据被破坏（如释放外部指针、扩容后链表错乱）后
// 的「静默数据损坏」——这类缺陷在多线程下会直接变成崩溃。
// ===========================================================================
TEST_CASE("伪随机分配释放压力下数据不损坏", "[allocator]")
{
    std::mt19937 rng(20260913u);

    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        std::vector<LiveBlock> live;
        std::vector<size_t>    verified;

        for (int op = 0; op < 3000; ++op)
        {
            const bool doAlloc = live.empty() || (rng() % 100) < 55;
            if (doAlloc)
            {
                const size_t  size = 1 + (rng() % 4096);
                const uint8_t tag  = static_cast<uint8_t>(rng() & 0xFF);
                const size_t  alignment = (rng() % 4 == 0) ? (size_t(1) << (4 + rng() % 5)) : 0;

                void* ptr = (alignment > 0) ? owner->AlignedAlloc(size, alignment)
                                            : owner->Alloc(size);
                REQUIRE(ptr != nullptr);
                if (alignment > 0)
                {
                    REQUIRE(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
                }

                memset(ptr, tag, size);
                live.push_back(LiveBlock{ ptr, size, tag });
            }
            else
            {
                const size_t index = rng() % live.size();
                const LiveBlock block = live[index];

                // 释放前校验内容
                REQUIRE(FilledWith(block.ptr, block.size, block.tag));
                verified.push_back(index);

                owner->Free(block.ptr);
                live[index] = live.back();
                live.pop_back();
            }
        }

        for (const LiveBlock& block : live)
        {
            REQUIRE(FilledWith(block.ptr, block.size, block.tag));
            owner->Free(block.ptr);
        }
    }
}

// ===========================================================================
// 并发：声明线程安全的后端必须能在多线程下并发分配/释放
//
// MallocTLSF 修复前 IsThreadSafe() 返回 false 且无同步 —— 并发 tlsf_free 会破坏
// free list。修复后由互斥锁保证，故四个后端都应通过本用例。
// ===========================================================================
TEST_CASE("线程安全后端的多线程并发分配", "[allocator][thread]")
{
    constexpr int kThreadCount = 4;
    constexpr int kIterations  = 1500;

    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);
        REQUIRE(owner->IsThreadSafe());

        std::atomic<int> failures{ 0 };
        std::vector<std::thread> threads;
        threads.reserve(kThreadCount);

        for (int t = 0; t < kThreadCount; ++t)
        {
            threads.emplace_back([&owner, &failures, t]()
            {
                std::mt19937 rng(1000u + static_cast<unsigned>(t));
                for (int i = 0; i < kIterations; ++i)
                {
                    const size_t size = 1 + (rng() % 2048);
                    void* ptr = owner->Alloc(size);
                    if (!ptr)
                    {
                        ++failures;
                        continue;
                    }
                    memset(ptr, static_cast<uint8_t>(i), size);
                    if (!FilledWith(ptr, size, static_cast<uint8_t>(i)))
                    {
                        ++failures;
                    }
                    owner->Free(ptr);
                }
            });
        }

        for (std::thread& thread : threads)
        {
            thread.join();
        }

        REQUIRE(failures.load() == 0);
    }
}

// ===========================================================================
// Memory 全局入口：懒初始化 / 边界输入 / 可插拔切换
// ===========================================================================

// Memory::Malloc 的懒初始化必须是线程安全的（doc/BugAuditReport.md 指出过该竞争：
// 修复前用裸指针做 `if (!gMalloc) gMalloc = new ...`，多线程首次调用可能各建一个）。
TEST_CASE("Memory 全局入口可被多线程安全地首次使用", "[allocator][memory]")
{
    Allocator::Memory::SetMalloc(nullptr);   // 恢复默认后端，触发一次懒初始化竞争

    constexpr int kThreadCount = 8;
    constexpr int kIterations  = 400;

    std::atomic<int> failures{ 0 };
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int t = 0; t < kThreadCount; ++t)
    {
        threads.emplace_back([&failures]()
        {
            for (int i = 0; i < kIterations; ++i)
            {
                void* ptr = Allocator::Memory::Malloc(256, 64);
                if (!ptr || (reinterpret_cast<uintptr_t>(ptr) % 64) != 0)
                {
                    ++failures;
                    continue;
                }
                memset(ptr, 0x7F, 256);
                // GetAllocSize 只对**当前后端**的指针有效。
                // SetMalloc(nullptr) 之后的竞争窗口里，本线程这次分配可能由兜底的系统分配器
                // 完成，而查询时后端已经切换成池化后端 —— 此时按契约返回 0（无法判定归属），
                // 这属于设计行为而不是错误。因此只拒绝「非 0 但小于请求大小」的结果。
                const size_t allocSize = Allocator::Memory::GetAllocSize(ptr);
                if (allocSize != 0 && allocSize < 256)
                {
                    ++failures;
                }
                Allocator::Memory::Free(ptr);
            }
        });
    }

    for (std::thread& thread : threads)
    {
        thread.join();
    }

    REQUIRE(failures.load() == 0);
}

// 边界输入：nullptr 与 0 字节。
// 修复前 Memory::Malloc(0) 会把 0 透传给 baselib::AlignedMalloc，命中 `assert(size > 0)`
// 而在 Debug 构建下直接 abort。
TEST_CASE("Memory 的边界输入不崩溃", "[allocator][memory]")
{
    Allocator::Memory::SetMalloc(nullptr);

    Allocator::Memory::Free(nullptr);
    REQUIRE(Allocator::Memory::GetAllocSize(nullptr) == 0);

    void* ptr = Allocator::Memory::Malloc(0);
    REQUIRE(ptr != nullptr);
    size_t size = Allocator::Memory::GetAllocSize(ptr);
    REQUIRE(size >= 1);
    Allocator::Memory::Free(ptr);

    // 低于默认对齐的 alignment 应被抬升到 DEFAULT_ALIGNMENT，而不是断言失败
    void* aligned = Allocator::Memory::Malloc(64, 1);
    REQUIRE(aligned != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(aligned) % DEFAULT_ALIGNMENT == 0);
    Allocator::Memory::Free(aligned);
}

// 可插拔：Memory::Malloc/Free/GetAllocSize 走的是通过 SetMalloc 指定的后端
TEST_CASE("Memory 可通过 SetMalloc 切换后端", "[allocator][memory]")
{
    // 后端实例必须活得比 Memory 的引用更久：Memory 只持有借来的裸指针，
    // 切换走/实例析构后旧指针会悬垂（见 AMalloc.h 的生命周期约定）。
    // 旧写法在 for 循环里析构 owner，中间存在一段「Memory 持有悬垂后端指针」的窗口。
    std::vector<std::unique_ptr<Malloc>> owners;
    owners.reserve(4);
    for (Backend backend : kAllBackends)
    {
        owners.emplace_back(CreateBackend(backend));
    }

    for (const std::unique_ptr<Malloc>& owner : owners)
    {
        INFO("backend = " << owner->GetDescriptiveName());
        Allocator::Memory::SetMalloc(owner.get());

        void* ptr = Allocator::Memory::Malloc(4096, 128);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 128 == 0);

        memset(ptr, 0x6D, 4096);
        REQUIRE(FilledWith(ptr, 4096, 0x6D));
        REQUIRE(Allocator::Memory::GetAllocSize(ptr) >= 4096);

        Allocator::Memory::Free(ptr);
    }

    // 恢复默认后端，避免影响其它用例
    Allocator::Memory::SetMalloc(nullptr);
    REQUIRE(Allocator::Memory::GetMalloc() != nullptr);
}

// ===========================================================================
// 高危回归：单次大块对齐分配（跨 TLSF 二级分段索引的桶边界）
//
// 修复前的行为：MallocTLSF 对 >= 98MiB 的对齐请求会「每轮 malloc 一个池、永远失败」，
// 直到进程被 OOM 杀掉。根因：
//   * tlsf_memalign 的 block_locate_free 会把请求向上取整到桶边界，池里那块空闲内存
//     属于更低的桶从而被跳过 —— 「池里有空闲块」并不等于「这次分配能成功」；
//   * AddPool 只判断了 `size > tlsf_block_size_max()`(4GB)，远高于真实阈值；
//   * 重试循环没有次数上限。
// 98MiB(102760448 = 49*2MiB) 是实测临界值；64MiB-1 则用来覆盖
// 「size 略小于 2 的幂时 aligned_size 越过该幂、桶宽翻倍」这一情形。
// 本用例带看门狗：一旦死循环回归会立刻 abort，而不是把机器吃光。
// ===========================================================================
TEST_CASE("大块对齐分配（跨 TLSF 桶边界）", "[allocator][tlsf][bugfix][large]")
{
    const size_t sizes[] = {
        64u * 1024u * 1024u - 1u,   // 67108863：aligned_size 会越过 2^26，桶宽翻倍
        98u * 1024u * 1024u,        // 102760448：实测的临界值
        128u * 1024u * 1024u,       // 134217728
    };

    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        for (size_t size : sizes)
        {
            INFO("size = " << size);
            Watchdog watchdog(8);

            void* ptr = owner->AlignedAlloc(size, 64);
            REQUIRE(ptr != nullptr);
            REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);

            // 只触碰首尾与采样点：几百 MB 全量 memset 会让用例变慢、变吃内存，
            // 而这里要验证的是「分配到的地址真的可写、可用大小不小于请求」。
            memset(ptr, 0xA5, 4096);
            REQUIRE(FilledWith(ptr, 4096, 0xA5));
            unsigned char* tail = static_cast<unsigned char*>(ptr) + size - 4096;
            memset(tail, 0x5A, 4096);
            REQUIRE(FilledWith(tail, 4096, 0x5A));

            size_t allocSize = 0;
            REQUIRE(owner->GetAllocationSize(ptr, allocSize));
            REQUIRE(allocSize >= size);

            owner->Free(ptr);
        }
    }
}

// ===========================================================================
// 非 2 的幂对齐：Memory 统一归一化，四个后端行为必须一致
//
// 修复前：Memory::Malloc 只把 alignment 抬到 DEFAULT_ALIGNMENT 就直接透传，
// 于是 Memory::Malloc(64, 24) 在 ANSI/TBB/mimalloc 上返回 nullptr
//（baselib::AlignedMalloc 拒绝非 2 的幂 / TBB 断言），在 TLSF 上被静默抬到 32 后成功。
// ===========================================================================
TEST_CASE("非 2 的幂对齐会被统一归一化", "[allocator][memory][bugfix]")
{
    struct AlignCase
    {
        size_t requested;   // 调用方请求的对齐
        size_t expected;    // 归一化后的对齐（>= requested 的最小 2 的幂，且 >= DEFAULT_ALIGNMENT）
    };

    const AlignCase cases[] = {
        { 1,   DEFAULT_ALIGNMENT },
        { 3,   DEFAULT_ALIGNMENT },
        { 16,  16  },
        { 17,  32  },
        { 24,  32  },
        { 48,  64  },
        { 100, 128 },
    };

    std::vector<std::unique_ptr<Malloc>> owners;
    for (Backend backend : kAllBackends)
    {
        owners.emplace_back(CreateBackend(backend));
    }

    for (const std::unique_ptr<Malloc>& owner : owners)
    {
        INFO("backend = " << owner->GetDescriptiveName());
        Allocator::Memory::SetMalloc(owner.get());

        for (const AlignCase& c : cases)
        {
            INFO("requested = " << c.requested << " expected = " << c.expected);
            void* ptr = Allocator::Memory::Malloc(64, c.requested);
            REQUIRE(ptr != nullptr);
            REQUIRE(reinterpret_cast<uintptr_t>(ptr) % c.expected == 0);
            memset(ptr, 0x11, 64);
            REQUIRE(FilledWith(ptr, 64, 0x11));
            Allocator::Memory::Free(ptr);
        }
    }

    Allocator::Memory::SetMalloc(nullptr);
}

// ===========================================================================
// 所有权感知：跨后端 / 外部指针必须被安全交还系统分配器
//
// 修复前：MallocTLSF::Free 对非本池指针 assert（Debug）或静默返回（Release，
// 表现为泄漏）；TBB/mimalloc 则会把外部指针交给自己的 free，直接破坏堆。
// 这是「可插拔后端能安全共存」的前提，也是全局 operator new/delete 能启用的前提。
// ===========================================================================
TEST_CASE("跨后端与外部指针的释放必须安全", "[allocator][memory][bugfix]")
{
    Allocator::Memory::SetMalloc(nullptr);
    Allocator::Memory::ResetStats();

    MallocTLSF tlsf;
    Allocator::Memory::SetMalloc(&tlsf);

    // (1) 完全外部的指针（系统 malloc）通过 Memory::Free 释放
    void* foreign = ::malloc(1024);
    REQUIRE(foreign != nullptr);
    Allocator::Memory::Free(foreign);
    REQUIRE(Allocator::Memory::GetStats().foreignFreeCount >= 1);

    // (2) 本后端的指针：OwnsPointer 为真，正常走 TLSF
    void* owned = Allocator::Memory::Malloc(2048, 64);
    REQUIRE(owned != nullptr);
    REQUIRE(tlsf.OwnsPointer(owned));
    REQUIRE(Allocator::Memory::GetAllocSize(owned) >= 2048);
    Allocator::Memory::Free(owned);

    // (3) 外部指针的 GetAllocationSize 必须返回 0，而不是崩溃
    void* foreign2 = ::malloc(256);
    REQUIRE(Allocator::Memory::GetAllocSize(foreign2) == 0);
    REQUIRE_FALSE(tlsf.OwnsPointer(foreign2));
    Allocator::Memory::Free(foreign2);

    Allocator::Memory::SetMalloc(nullptr);
}

// ANSI 是系统分配器：无法区分归属，恒为 true（见 MallocAnsi::OwnsPointer 的说明）
TEST_CASE("各后端 OwnsPointer 的语义", "[allocator][memory]")
{
    MallocAnsi ansi;
    void* systemPtr = ::malloc(64);
    REQUIRE(systemPtr != nullptr);
    REQUIRE(ansi.OwnsPointer(systemPtr));

    MallocTLSF tlsf;
    void* tlsfPtr = tlsf.Alloc(128);
    REQUIRE(tlsfPtr != nullptr);
    REQUIRE(tlsf.OwnsPointer(tlsfPtr));
    REQUIRE_FALSE(tlsf.OwnsPointer(systemPtr));

    MallocTBB tbb;
    void* tbbPtr = tbb.Alloc(128);
    REQUIRE(tbbPtr != nullptr);
    REQUIRE(tbb.OwnsPointer(tbbPtr));
    REQUIRE_FALSE(tbb.OwnsPointer(systemPtr));

    MallocMimalloc mimalloc;
    void* miPtr = mimalloc.Alloc(128);
    REQUIRE(miPtr != nullptr);
    REQUIRE(mimalloc.OwnsPointer(miPtr));
    REQUIRE_FALSE(mimalloc.OwnsPointer(systemPtr));

    // 统一契约：**每个后端**的 Free 只接受本分配器的指针，对非本后端指针一律拒绝
    // （不做释放、给出诊断），因为把外部指针交给任何一个后端的释放路径都可能破坏堆。
    // 跨后端 / 外部指针的释放统一由 Memory::Free 兜底（它先判归属再分派），
    // 见「跨后端与外部指针的释放必须安全」用例。
    REQUIRE_FALSE(tbb.OwnsPointer(nullptr));
    REQUIRE_FALSE(mimalloc.OwnsPointer(nullptr));
    REQUIRE_FALSE(tlsf.OwnsPointer(nullptr));

    ::free(systemPtr);
    tlsf.Free(tlsfPtr);
    tbb.Free(tbbPtr);
    mimalloc.Free(miPtr);
}

// ===========================================================================
// 跨线程释放：分配线程与释放线程不是同一个
//
// mimalloc 曾在这里崩溃：OwnsPointer 用 mi_check_owned（只认当前线程的默认堆），
// 于是「A 线程分配、B 线程释放」一律被误判为外部指针 → Memory::Free 把 mimalloc 的块
// 交给系统 free → malloc_report 报 "pointer being freed was not allocated" 并 abort。
// 修复为 mi_is_in_heap_region（地址区间判断，与线程无关）。
// 本用例用四个后端跑同样的交叉释放模式做回归。
// ===========================================================================
TEST_CASE("跨线程分配-释放（分配线程 != 释放线程）", "[allocator][thread][bugfix]")
{
    struct SharedBlock
    {
        void*   ptr;
        size_t  size;
        uint8_t tag;
    };

    for (Backend backend : kAllBackends)
    {
        INFO("backend = " << BackendName(backend));
        MallocOwner owner(backend);

        std::mutex mutex;
        std::deque<SharedBlock> queue;
        std::atomic<bool> stop{ false };
        std::atomic<int>  failures{ 0 };

        // 分配线程：只分配、只入队
        std::thread allocator([&]()
        {
            std::mt19937 rng(20260925u);
            while (!stop.load(std::memory_order_acquire))
            {
                const size_t size = 16 + rng() % 8192;
                const uint8_t tag = static_cast<uint8_t>(rng() & 0xFF);
                void* ptr = owner->AlignedAlloc(size, 16);
                if (!ptr)
                {
                    ++failures;
                    continue;
                }
                memset(ptr, tag, size);

                std::lock_guard<std::mutex> lock(mutex);
                if (queue.size() < 512)
                {
                    queue.push_back(SharedBlock{ ptr, size, tag });
                }
                else
                {
                    // 队列满：由分配线程自己释放，避免内存无界增长
                    owner->Free(ptr);
                }
            }
        });

        // 释放线程：只出队、只释放（线程与分配线程不同）
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        std::vector<SharedBlock> drained;
        while (std::chrono::steady_clock::now() < deadline)
        {
            SharedBlock block{ nullptr, 0, 0 };
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!queue.empty())
                {
                    block = queue.front();
                    queue.pop_front();
                }
            }
            if (!block.ptr)
            {
                continue;
            }

            if (!FilledWith(block.ptr, block.size, block.tag))
            {
                ++failures;
            }
            // 关键：在**另一个线程**上释放由分配线程分配的块
            owner->Free(block.ptr);
        }

        stop.store(true, std::memory_order_release);
        allocator.join();

        // 清空残余队列（仍然在测试主线程释放，属于第三种线程）
        while (true)
        {
            SharedBlock block{ nullptr, 0, 0 };
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (queue.empty())
                {
                    break;
                }
                block = queue.front();
                queue.pop_front();
            }
            REQUIRE(FilledWith(block.ptr, block.size, block.tag));
            owner->Free(block.ptr);
        }

        REQUIRE(failures.load() == 0);
    }
}

// ===========================================================================
// 分配统计：用于长时间运行的泄漏观测（引擎启动清零、退出前打印）
// ===========================================================================
TEST_CASE("分配统计能反映存活块与峰值", "[allocator][memory]")
{
    Allocator::Memory::SetMalloc(nullptr);
    Allocator::Memory::ResetStats();

    std::vector<void*> blocks;
    blocks.reserve(64);
    for (int i = 0; i < 64; ++i)
    {
        void* ptr = Allocator::Memory::Malloc(1024, 32);
        REQUIRE(ptr != nullptr);
        blocks.push_back(ptr);
    }

    MemoryStats stats = Allocator::Memory::GetStats();
    REQUIRE(stats.allocationCount == 64);
    REQUIRE(stats.liveBlockCount == 64);
    REQUIRE(stats.liveBytes >= 64 * 1024);
    REQUIRE(stats.peakLiveBytes >= stats.liveBytes);
    REQUIRE(stats.freeCount == 0);

    for (void* ptr : blocks)
    {
        Allocator::Memory::Free(ptr);
    }

    stats = Allocator::Memory::GetStats();
    REQUIRE(stats.freeCount == 64);
    REQUIRE(stats.liveBlockCount == 0);
    REQUIRE(stats.liveBytes == 0);
    REQUIRE(stats.peakLiveBytes >= 64 * 1024);

    // ResetStats 只清计数，不影响分配能力
    Allocator::Memory::ResetStats();
    stats = Allocator::Memory::GetStats();
    REQUIRE(stats.allocationCount == 0);
    REQUIRE(stats.peakLiveBytes == 0);

    void* afterReset = Allocator::Memory::Malloc(16);
    REQUIRE(afterReset != nullptr);
    Allocator::Memory::Free(afterReset);
    REQUIRE(Allocator::Memory::GetStats().liveBlockCount == 0);

    Allocator::Memory::SetMalloc(nullptr);
}

// ===========================================================================
// TLSF 的 Trim 必须真正回收「完全空闲」的追加池（否则池内存只增不减）
// ===========================================================================
TEST_CASE("MallocTLSF 的 Trim 回收空闲追加池", "[allocator][tlsf]")
{
    MallocTLSF tlsf;
    const size_t baseline = tlsf.GetPoolCount();
    REQUIRE(baseline == 1);

    std::vector<void*> blocks;
    for (int i = 0; i < 8; ++i)
    {
        void* ptr = tlsf.Alloc(4 * 1024 * 1024);
        REQUIRE(ptr != nullptr);
        blocks.push_back(ptr);
    }

    // 初始池只有 2MB，8 x 4MB 必然触发扩容
    REQUIRE(tlsf.GetPoolCount() > baseline);

    for (void* ptr : blocks)
    {
        tlsf.Free(ptr);
    }

    tlsf.Trim(true);
    REQUIRE(tlsf.GetPoolCount() == baseline);

    // 回收之后分配器仍然可用（不能把控制结构所在的首个池一起回收掉）
    void* again = tlsf.Alloc(1024);
    REQUIRE(again != nullptr);
    tlsf.Free(again);
}

// 池内有存活块时，Trim 绝不能回收该池
TEST_CASE("MallocTLSF 的 Trim 不会回收仍有存活块的池", "[allocator][tlsf]")
{
    MallocTLSF tlsf;

    std::vector<void*> blocks;
    for (int i = 0; i < 8; ++i)
    {
        void* ptr = tlsf.Alloc(4 * 1024 * 1024);
        REQUIRE(ptr != nullptr);
        blocks.push_back(ptr);
    }

    const size_t before = tlsf.GetPoolCount();
    REQUIRE(before > 1);

    tlsf.Trim(false);
    REQUIRE(tlsf.GetPoolCount() == before);

    // 数据必须仍然完整（Trim 误回收会立刻表现为崩溃/错乱）
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        memset(blocks[i], static_cast<uint8_t>(i + 1), 4096);
    }
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        REQUIRE(FilledWith(blocks[i], 4096, static_cast<uint8_t>(i + 1)));
    }

    for (void* ptr : blocks)
    {
        tlsf.Free(ptr);
    }
}
