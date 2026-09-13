//
//  test_bugfixes.cpp
//  GNXEngine
//
//  代码审计缺陷的「复现 + 回归」测试。
//
//  约定：
//    * 每个 TEST_CASE 对应审计报告 doc/BugAuditReport.md 中的一条缺陷（BUG-xx）；
//    * 注释里给出缺陷的文件:行号与触发条件；
//    * 这些用例在修复前应当失败（部分为确定性崩溃），修复后必须全部通过。
//
//  越界读类缺陷的复现手段：GuardedBuffer 把数据放在「只读页尾」，
//  其后紧跟一页不可访问内存（PROT_NONE）。任何超出数据长度的读取都会立即
//  触发 SIGSEGV —— 无需 ASAN 也能确定性复现，不依赖内存布局的偶然性。
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/RenderSystem/include/Camera.h"
#include "Runtime/RenderCore/include/RenderDefine.h"

#include <cmath>
#include <cstring>
#include <string>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#define GNX_HAVE_GUARD_PAGE 1
#else
#define GNX_HAVE_GUARD_PAGE 0
#endif

using Catch::Matchers::WithinAbs;

namespace
{
#if GNX_HAVE_GUARD_PAGE
// ---------------------------------------------------------------------------
// 保护页缓冲区：n 字节数据紧贴「不可访问页」之前的页尾。
// 越界读 = 命中 PROT_NONE 页 = 立即 SIGSEGV（确定性复现）。
// ---------------------------------------------------------------------------
class GuardedBuffer
{
public:
    explicit GuardedBuffer(size_t n)
    {
        mPageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        const size_t total = mPageSize * 2;
        void* base = mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (base == MAP_FAILED)
        {
            return;
        }
        mBase = base;
        if (mprotect(static_cast<unsigned char*>(base) + mPageSize, mPageSize, PROT_NONE) != 0)
        {
            munmap(base, total);
            mBase = nullptr;
            return;
        }
        // 数据放在第一页的最末尾，使第 n+1 个字节正好落在保护页上
        mData = static_cast<unsigned char*>(base) + mPageSize - n;
        memset(mData, 0xCD, n);
    }

    ~GuardedBuffer()
    {
        if (mBase != nullptr)
        {
            munmap(mBase, mPageSize * 2);
        }
    }

    GuardedBuffer(const GuardedBuffer&) = delete;
    GuardedBuffer& operator=(const GuardedBuffer&) = delete;

    bool IsValid() const { return mData != nullptr; }
    unsigned char* Data() const { return mData; }

private:
    void*          mBase = nullptr;
    size_t         mPageSize = 0;
    unsigned char* mData = nullptr;
};
#endif

// Matrix4x4f 未提供 operator==，测试里按内存逐字节比较
bool MatricesEqual(const mathutil::Matrix4x4f& a, const mathutil::Matrix4x4f& b)
{
    return memcmp(&a, &b, sizeof(mathutil::Matrix4x4f)) == 0;
}
}

// ===========================================================================
// BUG-01  ImageCodec 图像格式探测未校验缓冲区长度 → 短缓冲区越界读
//
// 位置：Engine/Runtime/ImageCodec/source/ImageDecoderHDR.cpp:6-16, 54-57
//        hdr_test_core() 逐字节比对签名（"#?RADIANCE\n" 共 11 字节），
//        而 IsFormat(buffer, size) 直接忽略 size 参数。
// 触发：缓冲区长度小于签名长度、且内容与签名前缀一致（例如被截断的 .hdr，
//       或伪造文件头）。此时探测会继续读取缓冲区之外的字节。
//        注意：若首字节就不匹配，函数会立即返回，不会越界 —— 因此复现必须
//        使用「签名前缀」内容，这里取 2 字节 "#?"。
// 后果：越界读堆内存；缓冲区位于页边界时崩溃（INFO 泄漏/拒绝服务）。
// ===========================================================================
TEST_CASE("BUG-01 图像格式探测不得越界读短缓冲区", "[bugfix][imagecodec]")
{
#if GNX_HAVE_GUARD_PAGE
    GuardedBuffer buffer(2);          // 仅 2 字节有效数据，紧随其后是保护页
    REQUIRE(buffer.IsValid());

    // 与 HDR 魔数 "#?RADIANCE\n" 的前 2 字节一致 → 探测会继续往下读
    buffer.Data()[0] = '#';
    buffer.Data()[1] = '?';

    imagecodec::VImage image;
    // 修复前：HDR 探测读取第 3..11 字节 → 命中保护页 → SIGSEGV
    const bool decoded = imagecodec::ImageDecoder::DecodeMemory(buffer.Data(), 2, &image);
    REQUIRE_FALSE(decoded);
#else
    SUCCEED("当前平台不支持保护页方案");
#endif
}

// ===========================================================================
// BUG-02  MathUtil::FastSin / FastCos 查表索引越界
//
// 位置：Engine/Runtime/MathUtil/source/MathUtil.cpp:148-166（FastSin）、168-182（FastCos）
//        fValue = fmod(x,360)；若 fValue < 0 则 += 360。
//        当输入是「极小的负数」时，360 + fValue 在浮点下被舍入为恰好 360.0f，
//        于是 nValueInt = (int)360.0f = 360，紧接着读取 SinTable[nValueInt + 1]，
//        即 SinTable[361] —— 而 SinTable 只有 361 个元素（合法下标 0..360）。
// 触发：MathUtil::FastSin(-1e-7f)（任意使 360+fValue 舍入为 360.0f 的输入）
// 后果：数组越界读（UB）；修复前 360 附近的插值取值非法（本次同时做数值回归）。
// 说明：越界读本身需 ASAN 才能观测（见 doc/BugAuditReport.md 的独立复现步骤），
//       本用例用作修复后的边界数值回归。
// ===========================================================================
TEST_CASE("BUG-02 FastSin/FastCos 边界角度数值正确", "[bugfix][mathutil]")
{
    constexpr float kPi = 3.14159265358979323846f;
    const auto toRad = [](float deg) { return deg * kPi / 180.0f; };

    // 覆盖：0、接近 360、恰好 360、超过 360、负数、极小负数、大数
    const float angles[] = {
        0.0f, 0.5f, 1.0f, 90.0f, 180.0f, 270.0f,
        359.9999f, 360.0f, 360.5f, 719.5f, 720.0f,
        -0.0000001f, -1.0f, -180.0f, -359.9999f, -360.0f, -720.5f,
        12345.678f, -98765.4321f
    };

    for (float deg : angles)
    {
        INFO("angle = " << deg);
        // 表是 1° 步长的线性插值，误差量级 ~1e-5，这里给 1e-3 的容差
        REQUIRE_THAT(mathutil::MathUtil::FastSin(deg), WithinAbs(std::sin(toRad(deg)), 1e-3f));
        REQUIRE_THAT(mathutil::MathUtil::FastCos(deg), WithinAbs(std::cos(toRad(deg)), 1e-3f));
    }

    // 极小负数：修复前 nValueInt 会被算成 360（越界读）
    REQUIRE_THAT(mathutil::MathUtil::FastSin(-1e-7f), WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(mathutil::MathUtil::FastCos(-1e-7f), WithinAbs(1.0f, 1e-3f));
}

// ===========================================================================
// BUG-03  Camera 修改近平面后未重算投影矩阵
//
// 位置：Engine/Runtime/RenderSystem/source/Camera.cpp:137-140
//        SetNearClipDistance() 只写 mNearZ，不重建 mProjection；
//        而投影矩阵（含 Reverse-Z 无限远平面）是用 zNear 计算出来的。
// 触发：SetLens(...) 之后调用 SetNearClipDistance(n)
// 后果：GetNearZ() 与 GetProjectionMatrix() 不一致 —— 深度、视锥、
//       以及依赖投影的剔除/SSAO/HiZ 等全部使用过期的近平面。
// ===========================================================================
TEST_CASE("BUG-03 Camera 修改近平面必须同步更新投影矩阵", "[bugfix][camera]")
{
    RenderSystem::Camera camera(RenderCore::METAL, "bugfix_camera");
    camera.SetLens(60.0f, 1280, 720, 0.1f, 1000.0f);

    const mathutil::Matrix4x4f before = camera.GetProjectionMatrix();

    camera.SetNearClipDistance(1.0f);

    REQUIRE_THAT(camera.GetNearZ(), WithinAbs(1.0f, 1e-6f));

    const mathutil::Matrix4x4f after = camera.GetProjectionMatrix();
    // 修复前：投影矩阵完全没变（断言失败）
    REQUIRE_FALSE(MatricesEqual(before, after));

    // 恢复为 0.1 后投影应回到初始值
    camera.SetNearClipDistance(0.1f);
    REQUIRE(MatricesEqual(before, camera.GetProjectionMatrix()));
}

// ===========================================================================
// BUG-03b Camera 近平面 setter 在未设置视口尺寸时不得产生 NaN/除零
// （修复方案使用 SetLens 重算，需要确认 mWidth/mHeight 为 0 时被安全跳过）
// ===========================================================================
TEST_CASE("BUG-03b Camera 未 SetLens 时修改近平面不得崩坏投影", "[bugfix][camera]")
{
    RenderSystem::Camera camera(RenderCore::METAL, "bugfix_camera2");

    camera.SetNearClipDistance(0.5f);
    REQUIRE_THAT(camera.GetNearZ(), WithinAbs(0.5f, 1e-6f));

    const mathutil::Matrix4x4f proj = camera.GetProjectionMatrix();
    for (int i = 0; i < 16; ++i)
    {
        const float v = reinterpret_cast<const float*>(&proj)[i];
        REQUIRE_FALSE(std::isnan(v));
        REQUIRE_FALSE(std::isinf(v));
    }
}
