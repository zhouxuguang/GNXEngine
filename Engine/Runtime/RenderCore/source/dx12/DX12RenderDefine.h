//
//  DX12RenderDefine.h
//  rendercore
//
//  D3D12 后端公共定义：平台校验、DirectX 头文件、HRESULT 检查宏、
//  以及描述符表布局常量（与 HLSL 的 b/t/u/s 寄存器一一对应）。
//

#ifndef GNX_ENGINE_DX12_RENDER_DEFINE_INCLUDE_GDFGLK
#define GNX_ENGINE_DX12_RENDER_DEFINE_INCLUDE_GDFGLK

// DX12 后端只在 Windows 上编译。若被其他平台包含则直接报错，避免误用。
#if !defined(_WIN32)
#error "DX12 backend is only available on Windows (_WIN32)."
#endif

// ---- Windows / DirectX 头文件必须先于引擎头文件包含 ----
// NOMINMAX：禁止 Windows 的 min/max 宏，避免破坏 <algorithm> 与引擎代码。
// WIN32_LEAN_AND_MEAN：裁剪 windows.h 中与图形无关的部分。
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>

// 引擎内部头文件（内含 PreCompile.h 提供的 GNX_OS_* 宏）
#include "RenderDefine.h"
#include "Runtime/BaseLib/include/DebugBreaker.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

NAMESPACE_RENDERCORE_BEGIN

using Microsoft::WRL::ComPtr;

// ============================================================================
// 调试层开关
//   Release 下默认关闭（调试层会显著拖慢 GPU）。需要排查问题时，
//   在 CMake 里加 -DGNX_DX12_DEBUG_LAYER=ON 即可打开。
// ============================================================================
#if defined(_DEBUG) || defined(GNX_DX12_DEBUG_LAYER)
    #ifndef GNX_DX12_DEBUG_LAYER
        #define GNX_DX12_DEBUG_LAYER 1
    #endif
    #ifndef GNX_DX12_GPU_BASED_VALIDATION
        #define GNX_DX12_GPU_BASED_VALIDATION 0
    #endif
#else
    #ifndef GNX_DX12_DEBUG_LAYER
        #define GNX_DX12_DEBUG_LAYER 0
    #endif
    #ifndef GNX_DX12_GPU_BASED_VALIDATION
        #define GNX_DX12_GPU_BASED_VALIDATION 0
    #endif
#endif

// 启用 DRED（设备移除根因面包屑）。DX12 上等价于 Vulkan 的 Aftermath，
// 是排查 TDR / 设备移除的主要手段。
#ifndef GNX_DX12_ENABLE_DRED
    #define GNX_DX12_ENABLE_DRED 1
#endif

// ============================================================================
// 帧同步常量
// ============================================================================
// 同时在飞行中的帧数（back buffer 数量）。与 Vulkan 后端的多重缓冲语义一致。
constexpr uint32_t GNX_DX12_FRAME_COUNT = 2;

// 默认的 back buffer 格式。R8G8B8A8_UNORM 与 Vulkan 后端的
// VK_FORMAT_R8G8B8A8_UNORM（kTexFormatRGBA8）保持一致，保证两个后端的
// 线性/伽马处理路径完全一致，便于图像 diff 对比。
// 后台缓冲格式。
//
// 保持 UNORM：曾假设应改为 _SRGB 以对齐 Vulkan 的 sRGB 交换链，并实测验证过——
// 改为 _SRGB 后 pbr 与 Vulkan 基线的 MAE 由 0.109 劣化到 0.166，故该假设被否掉。
// 目前 DX12 与 Vulkan 的画面差异是全图一致约 +30/255 的加性亮度偏移
// （高光被裁到 255），成因尚未定位；不要在这里盲改格式。
constexpr DXGI_FORMAT GNX_DX12_BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

// 默认深度缓冲格式（对应 kTexFormatDepth32Float，Reverse-Z 精度最好）。
constexpr DXGI_FORMAT GNX_DX12_DEPTH_FORMAT = DXGI_FORMAT_D32_FLOAT;

// ============================================================================
// HLSL 寄存器 → 描述符表布局
//
// 引擎的 HLSL 使用 b / t / u / s 四类寄存器（隐式或显式分配）。
// 实测全部 43 个 .shader + 10 个 .hlsl 中出现的最大寄存器号为：
//   b0..b3   t0..t4   u0..u4   s0..s4
// 这里给足余量（16），保证后续新增 shader 不会因为表宽不足而静默失效。
// 表中的槽位下标即 HLSL 寄存器号，因此绑定语义与“按名字反射出的寄存器”直接对应。
// ============================================================================
// 每类描述符表在根签名中声明的槽位数。
//
// 注意：这是"可寻址的寄存器上界"，必须不小于所有着色器实际用到的最大寄存器号 + 1。
// 取 16 时曾出现 SRV 绑定到 t16 越界（GetCpuHandle 判非法并返回空句柄 → id=646），
// 因此放宽到 32。表本身不占根签名的 DWORD（只有表基址占 1 个 DWORD），
// 每帧描述符环容量为 65536，放宽的代价仅是每块多预留一些槽位。
constexpr uint32_t GNX_DX12_CBV_TABLE_WIDTH     = 32;  // register(b0..b31)
constexpr uint32_t GNX_DX12_SRV_TABLE_WIDTH     = 32;  // register(t0..t31)
constexpr uint32_t GNX_DX12_UAV_TABLE_WIDTH     = 32;  // register(u0..u31)
constexpr uint32_t GNX_DX12_SAMPLER_TABLE_WIDTH = 32;  // register(s0..s31)

// 根参数下标（与 DX12RootSignature 中的定义保持一致）
constexpr uint32_t GNX_DX12_ROOT_PARAM_CBV_TABLE     = 0;
constexpr uint32_t GNX_DX12_ROOT_PARAM_SRV_TABLE     = 1;
constexpr uint32_t GNX_DX12_ROOT_PARAM_UAV_TABLE     = 2;
constexpr uint32_t GNX_DX12_ROOT_PARAM_SAMPLER_TABLE = 3;
constexpr uint32_t GNX_DX12_ROOT_PARAM_COUNT         = 4;

// ============================================================================
// Mesh 管线的 stage 组
//
// ── 为什么必须按 stage 分组 ──
// HLSL 的 b/t/u/s 寄存器是**每个 stage 独立分配**的：同一个资源名在 TS 里可能是 t0，
// 在 MS 里却是 t1（因为各 stage 用到的资源集合不同，DXC 按各自的声明顺序编号）。
// 因此"一张表服务所有 stage"的模型在 Mesh 管线上必然错位：
// 例如 MeshletDemo 中 TS 的 Meshlets=t0 / Instances=t1，而 MS 的
// Vertices=t0 / Meshlets=t1 / … / Instances=t5。
//
// Vulkan 后端用"每 stage 一个描述符集偏移"（VKGraphicsPipeline::mStageSetOffsets）
// 解决同一问题；DX12 侧的等价物是根签名的三组可见性表
// （AMPLIFICATION / MESH / PIXEL，见 DX12RootSignature），
// 描述符块也按同样的 3 组布局分配，于是每个 stage 都按自己的寄存器号寻址。
//
// 非 Mesh 管线只有 1 组（根签名的 4 张表是 SHADER_VISIBILITY_ALL）。
// ============================================================================
constexpr uint32_t GNX_DX12_STAGE_GROUP_TASK     = 0;   // amplification shader
constexpr uint32_t GNX_DX12_STAGE_GROUP_MESH     = 1;   // mesh shader
constexpr uint32_t GNX_DX12_STAGE_GROUP_FRAGMENT = 2;   // pixel shader
constexpr uint32_t GNX_DX12_STAGE_GROUP_MAX      = 3;

/// ShaderStage → stage 组下标（非 Mesh 管线恒为 0）
inline uint32_t DX12StageGroupIndex(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage_Task:     return GNX_DX12_STAGE_GROUP_TASK;
        case ShaderStage_Mesh:     return GNX_DX12_STAGE_GROUP_MESH;
        case ShaderStage_Fragment: return GNX_DX12_STAGE_GROUP_FRAGMENT;
        default:                   return GNX_DX12_STAGE_GROUP_TASK;
    }
}

// 每个 draw 需要的描述符块容量（4 类 × 表宽）
constexpr uint32_t GNX_DX12_DESCRIPTORS_PER_BLOCK =
    GNX_DX12_CBV_TABLE_WIDTH + GNX_DX12_SRV_TABLE_WIDTH +
    GNX_DX12_UAV_TABLE_WIDTH + GNX_DX12_SAMPLER_TABLE_WIDTH;

// 采样器堆的硬件上限为 2048，是三类堆里最容易成为瓶颈的。
// 单帧描述符环形缓冲容量：SRV/UAV/CBV 给足空间，采样器按硬件上限。
constexpr uint32_t GNX_DX12_FRAME_CBV_SRV_UAV_CAPACITY = 65536;
constexpr uint32_t GNX_DX12_FRAME_SAMPLER_CAPACITY     = 2048;

// D3D12 常量缓冲区大小必须是 256 字节对齐
constexpr uint32_t GNX_DX12_CONSTANT_BUFFER_ALIGNMENT = 256;

// ============================================================================
// HRESULT 检查
// ============================================================================
const char* DX12HResultToString(HRESULT hr);

#ifdef GNX_DX12_RESULT_CHECK
    // Log HRESULT failures without interrupting the process.
    #define DX12_CHECK(x)                                                                   \
        do                                                                                  \
        {                                                                                   \
            HRESULT dx12_check_hr = (x);                                                    \
            if (FAILED(dx12_check_hr))                                                      \
            {                                                                               \
                LOG_ERROR("[DX12] %s failed: %s (hr=0x%08X) at %s:%d", #x,                  \
                          RenderCore::DX12HResultToString(dx12_check_hr),                   \
                          (unsigned)dx12_check_hr, __FILE__, __LINE__);                     \
            }                                                                               \
        } while (0)
#else
    #define DX12_CHECK(x) (void)(x)
#endif

// 可恢复失败：打印错误但不中断（用于 Resize / Present 等允许失败重试的路径）
#define DX12_CHECK_MSG(x, fmt, ...)                                                     \
    do                                                                                  \
    {                                                                                   \
        HRESULT dx12_check_hr = (x);                                                    \
        if (FAILED(dx12_check_hr))                                                      \
        {                                                                               \
            LOG_ERROR("[DX12] %s failed: %s (hr=0x%08X) at %s:%d | " fmt, #x,           \
                      RenderCore::DX12HResultToString(dx12_check_hr),                   \
                      (unsigned)dx12_check_hr, __FILE__, __LINE__, ##__VA_ARGS__);       \
        }                                                                               \
    } while (0)

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_RENDER_DEFINE_INCLUDE_GDFGLK */
