//
//  AllocatorEngineStress.cpp
//  GNXEngine
//
//  引擎级内存分配器压力 / 长跑工具（无窗口，可脚本化）。
//
//  目的：把「可插拔分配器」放进真实引擎负载里长时间运行，验证：
//    1) 不崩溃（各后端接管全局 new/delete 之后仍然稳定）；
//    2) 不泄漏（存活字节在预热后不再持续增长）；
//    3) 不错配（Memory 统计里的 foreignFree 恒为 0）。
//
//  相比单元测试，这里跑的是引擎真实子系统：
//    * ImageCodec      —— RGBA 像素缓冲 → PNG 编码 → 解码往返（大块分配 + 像素校验）
//    * AssetProcess    —— ISPC 的 ASTC 压缩（单线程 / 多线程两条路径）
//    * AssetManager    —— 资源目录初始化与路径解析（字符串/容器分配）
//    * BaseLib         —— SHA256 / GUID
//    * 多线程交叉释放  —— 分配线程 push、释放线程 pop 并释放（分配器最容易出错的场景）
//
//  用法：
//    GNX_ALLOCATOR=ansi|tbb|mimalloc|tlsf  AllocatorEngineStress [秒数]
//    可选 GNX_ALLOCATOR_STATS_INTERVAL（默认 5 秒打印一次统计）
//
//  退出码：0 = 通过；1 = 检测到内存持续增长 / 像素校验失败 / 出现 foreignFree。
//

#include "Runtime/Allocator/include/AMalloc.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/GuidGenerator.h"
#include "Runtime/BaseLib/include/SHA256.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/ImageEncoder.h"
#include "Runtime/AssetProcess/include/ASTCCompressor.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace
{

using Clock = std::chrono::steady_clock;

double NowSeconds()
{
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

size_t ReadSizeArg(int argc, char** argv, const char* envName, size_t defaultValue)
{
    if (argc > 1)
    {
        const long long value = std::atoll(argv[1]);
        if (value > 0)
        {
            return static_cast<size_t>(value);
        }
    }
    if (const char* env = std::getenv(envName))
    {
        const long long value = std::atoll(env);
        if (value > 0)
        {
            return static_cast<size_t>(value);
        }
    }
    return defaultValue;
}

// 可选的工作负载开关，用于把「增长来自哪一部分」定位出来：
//   GNX_STRESS_WORKLOAD=all|churn|image|astc|handoff（缺省 all）
struct Workloads
{
    bool churn   = true;
    bool image   = true;
    bool astc    = true;
    bool handoff = true;
    bool probe   = false;   // 只做「单次编码/解码」的统计差值诊断，然后退出
    bool bench   = false;   // 只做分配器微基准，然后退出

    static Workloads FromEnvironment()
    {
        Workloads w;
        const char* value = std::getenv("GNX_STRESS_WORKLOAD");
        if (!value || !*value || std::strcmp(value, "all") == 0)
        {
            return w;
        }

        w.churn = w.image = w.astc = w.handoff = false;
        if (std::strcmp(value, "churn") == 0)   { w.churn = true; }
        else if (std::strcmp(value, "image") == 0)   { w.image = true; }
        else if (std::strcmp(value, "astc") == 0)    { w.astc = true; }
        else if (std::strcmp(value, "handoff") == 0) { w.handoff = true; }
        else if (std::strcmp(value, "probe") == 0)   { w.probe = true; }
        else if (std::strcmp(value, "bench") == 0)   { w.bench = true; }
        else { w.churn = w.image = w.astc = w.handoff = true; }
        return w;
    }
};

// ---------------------------------------------------------------------------
// 分配器直接压力：混合大小 / 混合对齐，并在释放前校验内容
// ---------------------------------------------------------------------------
struct LiveBlock
{
    void*   ptr;
    size_t  size;
    uint8_t tag;
};

void AllocatorChurn(std::mt19937& rng, std::vector<LiveBlock>& live, int operations)
{
    for (int i = 0; i < operations; ++i)
    {
        const bool wantAlloc = live.empty() || (rng() % 100) < 55;
        if (wantAlloc)
        {
            // 覆盖小对象、页级、以及偶尔的大块
            const size_t roll = rng() % 1000;
            const size_t size = (roll < 960) ? (1 + rng() % 8192)
                              : (roll < 995) ? (1 + rng() % (256 * 1024))
                              : (roll < 998) ? (1 + rng() % (2 * 1024 * 1024))
                                             : (4 * 1024 * 1024);   // 触发池扩容路径
            const size_t alignment = (rng() % 4 == 0) ? (size_t(1) << (4 + rng() % 6)) : 0;
            const uint8_t tag = static_cast<uint8_t>(rng() & 0xFF);

            void* ptr = (alignment > 0) ? Allocator::Memory::Malloc(size, alignment)
                                        : Allocator::Memory::Malloc(size, 16);
            if (!ptr)
            {
                LOG_ERROR("[stress] 分配失败 size=%zu alignment=%zu", size, alignment);
                continue;
            }

            // 只写头部若干字节：目的是探测「同一块内存被重复分配」与元数据损坏，
            // 不需要把整块都踩一遍（大块全写会让工具变慢）
            const size_t touch = (size < 64) ? size : 64;
            std::memset(ptr, tag, touch);

            live.push_back(LiveBlock{ ptr, touch, tag });
        }
        else
        {
            const size_t index = rng() % live.size();
            const LiveBlock block = live[index];

            const uint8_t* bytes = static_cast<const uint8_t*>(block.ptr);
            bool ok = true;
            for (size_t b = 0; b < block.size; ++b)
            {
                if (bytes[b] != block.tag)
                {
                    ok = false;
                    break;
                }
            }
            if (!ok)
            {
                LOG_ERROR("[stress] 内存内容被破坏（分配器元数据损坏或被重复分配）");
            }

            Allocator::Memory::Free(block.ptr);
            live[index] = live.back();
            live.pop_back();
        }
    }
}

// ---------------------------------------------------------------------------
// 引擎负载 1：图像编解码往返（像素缓冲 + PNG 编码器/解码器内部分配）
// ---------------------------------------------------------------------------
std::vector<uint8_t> MakePixels(uint32_t width, uint32_t height, uint32_t seed)
{
    const size_t bytesPerPixel = 4;
    const size_t pixelBytes = static_cast<size_t>(width) * height * bytesPerPixel;

    std::vector<uint8_t> pixels(pixelBytes);
    // 不透明像素（alpha=255），PNG 往返应当逐字节一致
    for (size_t i = 0; i < pixelBytes; i += bytesPerPixel)
    {
        const uint32_t v = static_cast<uint32_t>((i / bytesPerPixel) * 2654435761u + seed);
        pixels[i + 0] = static_cast<uint8_t>(v & 0xFF);
        pixels[i + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        pixels[i + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        pixels[i + 3] = 255;
    }
    return pixels;
}

bool EncodeImage(const std::vector<uint8_t>& pixels, uint32_t width, uint32_t height,
                 std::vector<unsigned char>& encoded)
{
    imagecodec::VImage source(imagecodec::FORMAT_RGBA8, width, height,
                              const_cast<uint8_t*>(pixels.data()));
    if (!imagecodec::ImageEncoder::EncodeMemory(encoded, source, imagecodec::kPNG_Format, 95))
    {
        LOG_ERROR("[stress] EncodeMemory 失败 (%ux%u)", width, height);
        return false;
    }
    return true;
}

bool DecodeImage(const std::vector<unsigned char>& encoded, uint32_t width, uint32_t height,
                 const std::vector<uint8_t>* expectedPixels = nullptr)
{
    imagecodec::VImage decoded;
    if (!imagecodec::ImageDecoder::DecodeMemory(encoded.data(), encoded.size(), &decoded))
    {
        LOG_ERROR("[stress] DecodeMemory 失败 (%ux%u)", width, height);
        return false;
    }

    if (decoded.GetWidth() != width || decoded.GetHeight() != height ||
        decoded.GetImageData() == nullptr)
    {
        LOG_ERROR("[stress] 解码结果尺寸不符 (%ux%u -> %ux%u)",
                  width, height, decoded.GetWidth(), decoded.GetHeight());
        return false;
    }

    // 逐字节比对：分配器若破坏内存（元数据错乱 / 块被重复分配），这里会立刻暴露
    if (expectedPixels)
    {
        const size_t pixelBytes = expectedPixels->size();
        const int decodedSize = decoded.GetImageSize();
        const size_t compareSize = (decodedSize > 0 && static_cast<size_t>(decodedSize) <= pixelBytes)
                                 ? static_cast<size_t>(decodedSize)
                                 : pixelBytes;
        if (std::memcmp(decoded.GetImageData(), expectedPixels->data(), compareSize) != 0)
        {
            LOG_ERROR("[stress] PNG 往返后像素不一致（%ux%u）", width, height);
            return false;
        }
    }
    return true;
}

bool ImageRoundTrip(uint32_t width, uint32_t height, uint32_t seed)
{
    const std::vector<uint8_t> pixels = MakePixels(width, height, seed);

    std::vector<unsigned char> encoded;
    if (!EncodeImage(pixels, width, height, encoded))
    {
        return false;
    }

    return DecodeImage(encoded, width, height, &pixels);
}

// 诊断用：分别测量「单次编码」「单次解码」前后的分配统计差值，
// 用于把存活量增长归属到具体调用（分配器本身不应有增长）。
void ProbeImageLeak()
{
    const uint32_t width = 256;
    const uint32_t height = 256;
    const std::vector<uint8_t> pixels = MakePixels(width, height, 7u);

    LOG_INFO("[stress][probe] 预热 3 轮 ...");
    for (int i = 0; i < 3; ++i)
    {
        std::vector<unsigned char> encoded;
        EncodeImage(pixels, width, height, encoded);
        DecodeImage(encoded, width, height);
    }

    for (int i = 0; i < 5; ++i)
    {
        std::vector<unsigned char> encoded;

        const Allocator::MemoryStats beforeEncode = Allocator::Memory::GetStats();
        EncodeImage(pixels, width, height, encoded);
        const Allocator::MemoryStats afterEncode = Allocator::Memory::GetStats();

        DecodeImage(encoded, width, height);
        const Allocator::MemoryStats afterDecode = Allocator::Memory::GetStats();

        LOG_INFO("[stress][probe] 第 %d 轮: encode +%llu blocks/+%llu bytes, decode +%llu blocks/+%llu bytes",
                 i,
                 (unsigned long long)(afterEncode.liveBlockCount - beforeEncode.liveBlockCount),
                 (unsigned long long)(afterEncode.liveBytes - beforeEncode.liveBytes),
                 (unsigned long long)(afterDecode.liveBlockCount - afterEncode.liveBlockCount),
                 (unsigned long long)(afterDecode.liveBytes - afterEncode.liveBytes));
    }

    // 进一步分段：把「解码」拆成「空 VImage 构造/析构」与「未知格式的失败路径」
    //（失败路径会走完工厂里 7 个 IsFormat），以判断泄漏位于工厂/格式识别还是平台解码。
    {
        const Allocator::MemoryStats before = Allocator::Memory::GetStats();
        for (int i = 0; i < 5; ++i)
        {
            imagecodec::VImage empty;
            (void)empty;
        }
        const Allocator::MemoryStats afterEmpty = Allocator::Memory::GetStats();
        LOG_INFO("[stress][probe] 空 VImage x5: +%llu blocks/+%llu bytes",
                 (unsigned long long)(afterEmpty.liveBlockCount - before.liveBlockCount),
                 (unsigned long long)(afterEmpty.liveBytes - before.liveBytes));

        std::vector<uint8_t> junk(4096, 0x11);   // 非任何已知格式，工厂会逐个 IsFormat 后返回失败
        for (int i = 0; i < 5; ++i)
        {
            imagecodec::VImage bitmap;
            (void)imagecodec::ImageDecoder::DecodeMemory(junk.data(), junk.size(), &bitmap);
        }
        const Allocator::MemoryStats afterJunk = Allocator::Memory::GetStats();
        LOG_INFO("[stress][probe] 未知格式解码失败 x5: +%llu blocks/+%llu bytes",
                 (unsigned long long)(afterJunk.liveBlockCount - afterEmpty.liveBlockCount),
                 (unsigned long long)(afterJunk.liveBytes - afterEmpty.liveBytes));
    }
}

// ---------------------------------------------------------------------------
// 分配器微基准：直接测量「分配 + 释放一次往返」的耗时，用于横向比较四个后端。
//
// 为什么单独做：demo 的 FPS 会被 GPU/垂直同步掩盖分配器差异，
// 而这里测的是纯分配路径（含 Memory 的归属判断与统计开销），
// 差异可以直接归因到后端实现本身。
// ---------------------------------------------------------------------------
void BenchmarkAllocator()
{
    const char* backend = Allocator::Memory::GetBackendName();
    const auto now = []() { return Clock::now(); };
    const auto nsBetween = [](Clock::time_point a, Clock::time_point b) {
        return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count());
    };

    static volatile void* sSink = nullptr;

    LOG_INFO("[bench] ===== 分配器微基准（后端 %s）=====", backend);

    // (1) 单线程：各尺寸 alloc+free 往返
    const size_t sizes[]  = { 8, 64, 512, 4096, 65536 };
    const char*  labels[] = { "8B", "64B", "512B", "4KB", "64KB" };
    const size_t kOps = 200000;

    double sumNs = 0.0;
    size_t sumOps = 0;
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        for (size_t warm = 0; warm < 2000; ++warm)
        {
            void* p = Allocator::Memory::Malloc(sizes[i]);
            Allocator::Memory::Free(p);
        }

        const auto t0 = now();
        for (size_t k = 0; k < kOps; ++k)
        {
            void* p = Allocator::Memory::Malloc(sizes[i]);
            sSink = p;
            Allocator::Memory::Free(p);
        }
        const auto t1 = now();
        const double ns = nsBetween(t0, t1);
        sumNs += ns;
        sumOps += kOps;
        LOG_INFO("[bench] 单线程 %-5s: %7.1f ns/op  (%6.2f Mops/s)",
                 labels[i], ns / (double)kOps, (double)kOps / (ns * 1e-9) / 1e6);
    }
    LOG_INFO("[bench] 单线程 混合平均: %7.1f ns/op", sumNs / (double)sumOps);

    // (2) 单线程：对齐分配（32/64/256 字节对齐）
    const size_t alignments[] = { 32, 64, 256 };
    for (size_t a : alignments)
    {
        for (size_t warm = 0; warm < 2000; ++warm)
        {
            void* p = Allocator::Memory::Malloc(1024, a);
            Allocator::Memory::Free(p);
        }

        const auto t0 = now();
        for (size_t k = 0; k < kOps; ++k)
        {
            void* p = Allocator::Memory::Malloc(1024, a);
            sSink = p;
            Allocator::Memory::Free(p);
        }
        const auto t1 = now();
        const double ns = nsBetween(t0, t1);
        LOG_INFO("[bench] 单线程 1KB/%3zuB 对齐: %7.1f ns/op  (%6.2f Mops/s)",
                 a, ns / (double)kOps, (double)kOps / (ns * 1e-9) / 1e6);
    }

    // (3) 多线程吞吐：4 线程同时做混合尺寸往返（衡量锁竞争 / 线程缓存）
    constexpr int kThreads = 4;
    const size_t kOpsPerThread = 100000;
    std::atomic<bool> start{ false };
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::atomic<uint64_t> doneOps{ 0 };

    const auto t0 = now();
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back([&start, &doneOps, t]()
        {
            std::mt19937 rng(0xBEEF0000u + static_cast<unsigned>(t));
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (size_t k = 0; k < kOpsPerThread; ++k)
            {
                const size_t size = 8u << (rng() % 13);   // 8B ~ 32KB，2 的幂
                void* p = Allocator::Memory::Malloc(size, 16);
                if (p)
                {
                    static_cast<uint8_t*>(p)[0] = static_cast<uint8_t>(k);
                    Allocator::Memory::Free(p);
                }
                doneOps.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    start.store(true, std::memory_order_release);
    for (std::thread& th : threads)
    {
        th.join();
    }
    const auto t1 = now();
    {
        const double ns = nsBetween(t0, t1);
        const double ops = static_cast<double>(doneOps.load());
        LOG_INFO("[bench] %d 线程混合尺寸: %7.1f ns/op  (%6.2f Mops/s 总吞吐)",
                 kThreads, ns / ops, ops / (ns * 1e-9) / 1e6);
    }

    // (4) 大块分配（走 TLSF 池扩容 / 系统的 mmap 路径），只做少量次数
    {
        const size_t bigSize = 4u * 1024u * 1024u;
        const size_t kBigOps = 200;
        for (size_t warm = 0; warm < 4; ++warm)
        {
            void* p = Allocator::Memory::Malloc(bigSize);
            Allocator::Memory::Free(p);
        }
        const auto tb0 = now();
        for (size_t k = 0; k < kBigOps; ++k)
        {
            void* p = Allocator::Memory::Malloc(bigSize);
            if (p)
            {
                static_cast<uint8_t*>(p)[0] = 1;
            }
            Allocator::Memory::Free(p);
        }
        const auto tb1 = now();
        const double ns = nsBetween(tb0, tb1);
        LOG_INFO("[bench] 单线程 4MB 大块: %7.0f ns/op  (%6.3f Mops/s)",
                 ns / (double)kBigOps, (double)kBigOps / (ns * 1e-9) / 1e6);
    }

    // (5) Memory 层开销：同样的往返，一次经 Memory（含归属判断 + 统计），一次直连后端。
    //     差值就是「接入分配器」这件事本身给引擎加的单次成本。
    {
        const size_t size = 256;
        const size_t kOverheadOps = 300000;
        Allocator::MallocPtr raw = Allocator::Memory::GetMalloc();

        for (size_t warm = 0; warm < 5000; ++warm)
        {
            void* p = Allocator::Memory::Malloc(size);
            Allocator::Memory::Free(p);
            p = raw->AlignedAlloc(size, 16);
            raw->Free(p);
        }

        const auto tm0 = now();
        for (size_t k = 0; k < kOverheadOps; ++k)
        {
            void* p = Allocator::Memory::Malloc(size);
            sSink = p;
            Allocator::Memory::Free(p);
        }
        const auto tm1 = now();

        const auto tr0 = now();
        for (size_t k = 0; k < kOverheadOps; ++k)
        {
            void* p = raw->AlignedAlloc(size, 16);
            sSink = p;
            raw->Free(p);
        }
        const auto tr1 = now();

        const double viaMemory = nsBetween(tm0, tm1) / (double)kOverheadOps;
        const double viaRaw = nsBetween(tr0, tr1) / (double)kOverheadOps;
        LOG_INFO("[bench] Memory 层开销(256B): 经 Memory %7.1f ns/op  直连后端 %7.1f ns/op  差 %+.1f ns/op (%.0f%%)",
                 viaMemory, viaRaw, viaMemory - viaRaw,
                 (viaRaw > 0.0) ? ((viaMemory - viaRaw) / viaRaw * 100.0) : 0.0);
    }

    Allocator::Memory::LogStats("微基准结束");
}

// ---------------------------------------------------------------------------
// 引擎负载 2：ISPC ASTC 压缩（单线程 / 多线程）
// ---------------------------------------------------------------------------
bool AstcCompress(uint32_t width, uint32_t height, bool multiThread)
{
    const uint32_t blockWidth = 6;
    const uint32_t blockHeight = 6;
    const uint32_t blockCountX = (width + blockWidth - 1) / blockWidth;
    const uint32_t blockCountY = (height + blockHeight - 1) / blockHeight;
    const size_t outputBytes = static_cast<size_t>(blockCountX) * blockCountY * 16;

    imagecodec::VImage image(imagecodec::FORMAT_RGBA8, width, height, nullptr);
    image.AllocPixels();
    if (image.GetImageData() == nullptr)
    {
        LOG_ERROR("[stress] AllocPixels 失败");
        return false;
    }
    std::memset(image.GetImageData(), 0x7F, static_cast<size_t>(image.GetImageSize()));

    std::vector<uint8_t> output(outputBytes);
    if (multiThread)
    {
        AssetProcess::CompressASTC_MT(output.data(), image.GetImageData(), width, height,
                                      blockWidth, blockHeight, image.GetBytesPerRow());
    }
    else
    {
        AssetProcess::CompressASTC(output.data(), image.GetImageData(), width, height,
                                   blockWidth, blockHeight, image.GetBytesPerRow());
    }
    return true;
}

// ---------------------------------------------------------------------------
// 引擎负载 3：跨线程分配 / 释放
//
// 分配线程把块放入队列，释放线程从队列取出并释放 —— 也就是「A 线程分配、B 线程释放」。
// 这是可插拔分配器最容易出问题的场景（TLSF 的池归属、mimalloc/TBB 的线程缓存迁移）。
// ---------------------------------------------------------------------------
class CrossThreadHandoff
{
public:
    CrossThreadHandoff()
    {
        for (int i = 0; i < kAllocThreads; ++i)
        {
            mAllocThreads.emplace_back([this, i]() { AllocWorker(static_cast<uint32_t>(i)); });
        }
        for (int i = 0; i < kFreeThreads; ++i)
        {
            mFreeThreads.emplace_back([this]() { FreeWorker(); });
        }
    }

    ~CrossThreadHandoff()
    {
        mStop.store(true, std::memory_order_release);
        for (std::thread& t : mAllocThreads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        for (std::thread& t : mFreeThreads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    bool HadFailure() const { return mFailure.load(std::memory_order_relaxed) != 0; }
    uint64_t HandoffCount() const { return mHandoff.load(std::memory_order_relaxed); }

private:
    static const int kAllocThreads = 2;
    static const int kFreeThreads  = 2;
    static const size_t kMaxQueue  = 512;

    void AllocWorker(uint32_t seed)
    {
        std::mt19937 rng(0x9E3779B9u ^ seed);
        while (!mStop.load(std::memory_order_acquire))
        {
            const size_t size = 16 + rng() % 4096;
            const uint8_t tag = static_cast<uint8_t>(rng() & 0xFF);
            void* ptr = Allocator::Memory::Malloc(size, 16);
            if (!ptr)
            {
                mFailure.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            std::memset(ptr, tag, size);

            std::unique_lock<std::mutex> lock(mMutex);
            if (mQueue.size() >= kMaxQueue)
            {
                // 队列积压时由分配线程自己释放，避免内存无界增长
                lock.unlock();
                Allocator::Memory::Free(ptr);
                std::this_thread::yield();
                continue;
            }
            mQueue.push_back(LiveBlock{ ptr, size, tag });
            mHandoff.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void FreeWorker()
    {
        while (true)
        {
            LiveBlock block{ nullptr, 0, 0 };
            {
                std::unique_lock<std::mutex> lock(mMutex);
                if (!mQueue.empty())
                {
                    block = mQueue.front();
                    mQueue.pop_front();
                }
            }

            if (!block.ptr)
            {
                if (mStop.load(std::memory_order_acquire))
                {
                    // 收尾：把剩余队列清空
                    std::unique_lock<std::mutex> lock(mMutex);
                    if (mQueue.empty())
                    {
                        return;
                    }
                    continue;
                }
                std::this_thread::yield();
                continue;
            }

            const uint8_t* bytes = static_cast<const uint8_t*>(block.ptr);
            for (size_t i = 0; i < block.size; ++i)
            {
                if (bytes[i] != block.tag)
                {
                    mFailure.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
            Allocator::Memory::Free(block.ptr);
        }
    }

    std::mutex              mMutex;
    std::deque<LiveBlock>   mQueue;
    std::atomic<bool>       mStop{ false };
    std::atomic<uint64_t>   mFailure{ 0 };
    std::atomic<uint64_t>   mHandoff{ 0 };
    std::vector<std::thread> mAllocThreads;
    std::vector<std::thread> mFreeThreads;
};

} // namespace

int main(int argc, char** argv)
{
    const size_t durationSeconds = ReadSizeArg(argc, argv, "GNX_ALLOCATOR_STRESS_SECONDS", 30);

    Allocator::Memory::LogBackendInfo();

    // AssetManager 与 demo 一样在最早处初始化（资源目录存在时才做）
    const std::string assetDir = GetProjectAssetDir();
    if (fs::exists(assetDir))
    {
        if (!AssetManager::AssetManager::GetInstance())
        {
            AssetManager::AssetManager::Initialize(assetDir);
        }
    }
    else
    {
        LOG_WARN("[stress] 资源目录不存在，跳过 AssetManager 初始化: %s", assetDir.c_str());
    }

    // -----------------------------------------------------------------------
    // 预热：让各子系统的惰性缓存（线程池、静态表、资源索引）先建立起来，
    // 之后的存活字节增长才真正反映泄漏。
    // -----------------------------------------------------------------------
    LOG_INFO("[stress] 预热中 ...");
    for (int i = 0; i < 4; ++i)
    {
        (void)ImageRoundTrip(256, 256, static_cast<uint32_t>(i));
        (void)AstcCompress(128, 128, (i % 2) == 1);
        std::mt19937 warmRng(1234u + static_cast<unsigned>(i));
        std::vector<LiveBlock> live;
        AllocatorChurn(warmRng, live, 2000);
        for (const LiveBlock& block : live)
        {
            Allocator::Memory::Free(block.ptr);
        }
    }
    Allocator::Memory::Trim(true);
    Allocator::Memory::ResetStats();

    const Workloads workloads = Workloads::FromEnvironment();

    if (workloads.probe)
    {
        ProbeImageLeak();
        return 0;
    }

    if (workloads.bench)
    {
        BenchmarkAllocator();
        return 0;
    }

    const Allocator::MemoryStats baseline = Allocator::Memory::GetStats();
    LOG_INFO("[stress] 预热完成；开始 %zu 秒压力（后端 %s）",
             durationSeconds, Allocator::Memory::GetBackendName());

    const double startTime = NowSeconds();
    const double endTime = startTime + static_cast<double>(durationSeconds);

    LOG_INFO("[stress] 工作负载: churn=%d image=%d astc=%d handoff=%d",
             (int)workloads.churn, (int)workloads.image,
             (int)workloads.astc, (int)workloads.handoff);

    std::unique_ptr<CrossThreadHandoff> handoff;
    if (workloads.handoff)
    {
        handoff.reset(new CrossThreadHandoff());
    }

    std::vector<uint64_t> liveBytesSamples;
    std::vector<LiveBlock> live;
    std::mt19937 rng(0xC0FFEEu);

    size_t iterations = 0;
    size_t imageRounds = 0;
    size_t astcRounds = 0;
    size_t failures = 0;
    double lastSampleTime = startTime;

    while (NowSeconds() < endTime)
    {
        ++iterations;

        if (workloads.churn)
        {
            // 分配器直接压力（每次都做）
            AllocatorChurn(rng, live, 200);
            if (!live.empty())
            {
                const size_t index = rng() % live.size();
                Allocator::Memory::Free(live[index].ptr);
                live[index] = live.back();
                live.pop_back();
            }

            // 控制占用：churn 本身是随机游走，长时间运行会累积大量大块。
            // 超过上限就主动释放一部分，让内存占用有界（仍然反复经过分配/扩容路径），
            // 同时也避免长跑时把机器压到 swap。
            const size_t kMaxLiveBlocks = 2048;
            while (live.size() > kMaxLiveBlocks)
            {
                Allocator::Memory::Free(live.back().ptr);
                live.pop_back();
            }
        }

        // 图像往返（每次做；512x512 会带来 1MB 级缓冲）
        if (workloads.image)
        {
            const uint32_t size = (iterations % 8 == 0) ? 512u : 256u;
            if (!ImageRoundTrip(size, size, static_cast<uint32_t>(iterations)))
            {
                ++failures;
            }
            ++imageRounds;
        }

        // ASTC（耗时较大，每 4 次做一次）
        if (workloads.astc && (iterations % 4 == 0))
        {
            if (!AstcCompress(128, 128, (iterations % 8) == 0))
            {
                ++failures;
            }
            ++astcRounds;
        }

        // 纯容器 / 字符串抖动（让标准库的小对象分配也走分配器）
        {
            std::map<std::string, std::string> scratch;
            for (int i = 0; i < 32; ++i)
            {
                scratch["key_" + std::to_string(iterations) + "_" + std::to_string(i)] =
                    std::string(64 + (i * 7) % 512, static_cast<char>('a' + (i % 26)));
            }
            std::string joined;
            for (const auto& kv : scratch)
            {
                joined += kv.second;
            }
        }

        // 周期性采样存活字节
        const double now = NowSeconds();
        if (now - lastSampleTime >= 5.0)
        {
            lastSampleTime = now;

            // 采样前先释放 churn 持有的块并回收空闲池：随机游走会让存活量上下抖动
            // 几十 MB，直接采样无法区分「抖动」和「泄漏」。统一口径后，泄漏会表现为
            // 采样值单调上升（低水位抬高）。
            for (const LiveBlock& block : live)
            {
                Allocator::Memory::Free(block.ptr);
            }
            live.clear();
            Allocator::Memory::Trim(true);

            const Allocator::MemoryStats stats = Allocator::Memory::GetStats();
            liveBytesSamples.push_back(stats.liveBytes);
            LOG_INFO("[stress] t=%.0fs iter=%zu image=%zu astc=%zu liveBlocks=%llu liveBytes=%llu peak=%llu foreignFree=%llu handoff=%llu",
                     now - startTime, iterations, imageRounds, astcRounds,
                     (unsigned long long)stats.liveBlockCount,
                     (unsigned long long)stats.liveBytes,
                     (unsigned long long)stats.peakLiveBytes,
                     (unsigned long long)stats.foreignFreeCount,
                     (unsigned long long)(handoff ? handoff->HandoffCount() : 0));
        }
    }

    // -----------------------------------------------------------------------
    // 收尾：释放全部本地缓冲，再看存活字节
    // -----------------------------------------------------------------------
    for (const LiveBlock& block : live)
    {
        Allocator::Memory::Free(block.ptr);
    }
    live.clear();

    Allocator::Memory::Trim(true);
    const Allocator::MemoryStats finalStats = Allocator::Memory::GetStats();

    LOG_INFO("[stress] 结束：iter=%zu image=%zu astc=%zu", iterations, imageRounds, astcRounds);
    LOG_INFO("[stress] 基线(baseline) liveBytes=%llu liveBlocks=%llu",
             (unsigned long long)baseline.liveBytes,
             (unsigned long long)baseline.liveBlockCount);
    Allocator::Memory::LogStats("最终统计");

    // -----------------------------------------------------------------------
    // 判定
    // -----------------------------------------------------------------------
    int exitCode = 0;

    if (failures != 0)
    {
        LOG_ERROR("[stress] 失败：%zu 次引擎负载校验失败", failures);
        exitCode = 1;
    }
    if (handoff && handoff->HadFailure())
    {
        LOG_ERROR("[stress] 失败：跨线程释放路径检测到内存破坏");
        exitCode = 1;
    }
    if (finalStats.foreignFreeCount != 0)
    {
        LOG_ERROR("[stress] 失败：出现 %llu 次「非当前后端」的释放（分配/释放错配）",
                  (unsigned long long)finalStats.foreignFreeCount);
        exitCode = 1;
    }
    if (finalStats.allocationCount == 0)
    {
        LOG_ERROR("[stress] 失败：运行期间没有记录到任何分配（接管未生效？）");
        exitCode = 1;
    }

    // 泄漏判定：预热后与结束时相比，存活字节不应显著增长。
    // 允许一定冗余（引擎的惰性缓存、容器容量不缩容），但不允许线性增长。
    const uint64_t growth = (finalStats.liveBytes > baseline.liveBytes)
                          ? (finalStats.liveBytes - baseline.liveBytes)
                          : 0;
    const uint64_t tolerance = (finalStats.peakLiveBytes / 10) + (1024 * 1024);
    if (growth > tolerance)
    {
        LOG_ERROR("[stress] 失败：存活字节增长 %llu（基线 %llu -> %llu，容差 %llu）",
                  (unsigned long long)growth,
                  (unsigned long long)baseline.liveBytes,
                  (unsigned long long)finalStats.liveBytes,
                  (unsigned long long)tolerance);
        exitCode = 1;
    }

    // 采样趋势：各采样点口径一致（已释放 churn 持有的块），因此「低水位」应当平稳。
    // 若最后一次采样比首次采样高出容差，说明运行期间存在泄漏。
    if (liveBytesSamples.size() >= 3)
    {
        const uint64_t first = liveBytesSamples.front();
        const uint64_t last = liveBytesSamples.back();
        const uint64_t trendTolerance = (finalStats.peakLiveBytes / 10) + (1024 * 1024);
        LOG_INFO("[stress] 采样序列（同口径存活字节）：");
        for (size_t i = 0; i < liveBytesSamples.size(); ++i)
        {
            LOG_INFO("[stress]   sample[%zu] = %llu",
                     i, (unsigned long long)liveBytesSamples[i]);
        }

        if (last > first && (last - first) > trendTolerance)
        {
            LOG_ERROR("[stress] 失败：存活字节呈持续增长趋势（首个采样 %llu -> 末次采样 %llu，容差 %llu）",
                      (unsigned long long)first,
                      (unsigned long long)last,
                      (unsigned long long)trendTolerance);
            exitCode = 1;
        }
    }

    LOG_INFO("[stress] 结果：%s（后端 %s，运行 %.0f 秒，%zu 次迭代）",
             (exitCode == 0) ? "通过" : "失败",
             Allocator::Memory::GetBackendName(),
             NowSeconds() - startTime,
             iterations);

    return exitCode;
}
