//
//  RenderDeviceFeatures.h
//  GNXEngine
//
//  结构化设备特性查询系统（替代旧的 DeviceExtension 虚函数模式）
//  在设备创建时由各后端填充，上层通过 RenderDevice::GetFeatures() 查询
//
//  设计原则：
//  只暴露【跨 API 通用的硬件/逻辑能力】，不包含任何单一 API 的机制细节。
//  判断标准："这是 GPU 能力，还是某个 API 的设计选择？"
//

#ifndef GNX_ENGINE_RENDER_DEVICE_FEATURES_INCLUDE_H
#define GNX_ENGINE_RENDER_DEVICE_FEATURES_INCLUDE_H

#include <string>
#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief 渲染设备硬件能力与特性集合
 *
 */
struct RenderDeviceFeatures
{
    // ============================================================
    // 一、设备基本信息
    // ============================================================
    struct DeviceInfo
    {
        std::string deviceName;       // e.g. "Apple M1 Max", "NVIDIA GeForce RTX 4090"
        std::string vendorName;       // e.g. "Apple", "NVIDIA", "AMD"

        enum class DeviceType : uint8_t
        {
            Unknown     = 0,
            Integrated  = 1,  // 集成显卡 / SoC
            Discrete    = 2,  // 独立显卡
            VirtualGPU  = 3,  // 虚拟 GPU
        } deviceType = DeviceType::Unknown;

        uint32_t apiVersion = 0;       // 如 VK_API_VERSION_1_3 或 Metal version
    } deviceInfo;

    // ============================================================
    // 二、硬件限制值（所有 GPU 都有对应的物理上限）
    // ============================================================
    struct Limits
    {
        // ---- 纹理 ----
        uint32_t maxTextureSize2D      = 0;
        uint32_t maxTextureSize3D      = 0;
        uint32_t maxTextureArrayLayers = 0;
        uint32_t maxCubeMapSize        = 0;

        // ---- 渲染目标 ----
        uint32_t maxColorAttachments   = 0;   // MRT 数量（Metal: maxColorRenderTargets）

        // ---- 顶点输入 ----
        uint32_t maxVertexBufferBindings   = 8;  // 默认 Vulkan 最低要求
        uint32_t maxVertexInputAttributes  = 16;

        // ---- 采样器 ----
        float    maxSamplerAnisotropy     = 0.0f;

        // ---- Buffer 对齐与范围（Metal UBO alignment = 4, Vulkan = 256）----
        uint32_t minUniformBufferOffsetAlignment = 256;
        uint32_t minStorageBufferOffsetAlignment = 256;
        uint32_t maxUniformBufferRange           = 0;
        uint32_t maxStorageBufferRange           = 0;

        // ---- Compute（跨 API 通用概念）----
        uint32_t maxComputeSharedMemorySize      = 0;
        uint32_t maxComputeWorkGroupInvocations  = 0;
        uint32_t maxComputeWorkGroupSize[3]      = {0, 0, 0};
        uint32_t maxComputeWorkGroupCount[3]     = {0, 0, 0};

        // ---- Framebuffer ----
        uint32_t maxFramebufferWidth  = 0;
        uint32_t maxFramebufferHeight = 0;
        uint32_t maxFramebufferLayers = 0;

        // ---- 时间戳查询 ----
        uint64_t timestampPeriod = 0;  // 纳秒/时间戳单位；0 = 不支持

        // ---- MSAA ----
        uint32_t maxSampleCountMaskBits = 0x01;  // bit mask: bit0=1x, bit1=2x, ...
    } limits;

    // ============================================================
    // 三、Shader 能力（硬件管线阶段 + 数据类型支持）
    // ============================================================
    struct Shader
    {
        // ---- 可编程管线阶段 ----
        bool meshShader         = false;  // Vulkan EXT/NV / Metal Mesh Pipeline
        bool taskShader         = false;  // Amplification Shader / Metal Object Function
        bool rayTracing         = false;  // Ray Generation / Intersection / Any-Hit / Closest-Hit

        // ---- 数据并行操作 ----
        bool waveIntrinsics     = false;  // Subgroup / Wave / SIMD-group 操作

        // ---- 数值类型支持 ----
        bool float16   = false;
        bool int8      = false;
        bool int64Atomics = false;
        bool float64   = false;
    } shader;

    // ============================================================
    // 四、资源与格式能力
    // ============================================================
    struct Resource
    {
        // ---- 纹理压缩格式 ----
        bool textureCompressionBC    = true;   // S3TC/DXT BC1-BC7（桌面 GPU 普遍支持）
        bool textureCompressionASTC   = false;  // ASTC（移动端 / Apple）
        bool textureCompressionETC2   = false;  // ETC2（移动端 OpenGL ES）
        bool textureCompressionPVRTC  = false;  // PVRTC（旧 iOS）

        // ---- 高级资源功能（跨 API 概念通用）----
        bool bindlessResources  = false;  // Descriptor Indexing / Argument Buffer / SM 6.6
        bool sparseTextures     = false;  // Sparse Binding / Tiled Resources / Partially Resident

        // ---- 格式支持 ----
        bool formatBGRA8        = true;   // BGRA8888
    } resource;

    // ============================================================
    // 五、高级渲染功能（跨 API 概念通用）
    // ============================================================
    struct Advanced
    {
        bool variableRateShading = false;  // VRS / Shading Rate / Tiled Rasterization
        bool asyncCompute        = false;  // 独立 Compute Queue
        bool multiview           = false;  // Multiview / Instanced View / Render Target Array
    } advanced;

    // ============================================================
    // 快捷组合查询
    // ============================================================

    /// 是否支持 Mesh Pipeline
    bool SupportsMeshPipeline() const { return shader.meshShader || shader.taskShader; }

    /// 是否支持光线追踪（需 RT shader + bindless 或等效机制）
    bool SupportsRayTracing() const { return shader.rayTracing && resource.bindlessResources; }

    /// 是否支持 Bindless 资源访问
    bool SupportsBindless() const { return resource.bindlessResources; }

    /// 是否支持异步计算（独立 compute queue）
    bool SupportsAsyncCompute() const { return advanced.asyncCompute; }

    /// 是否支持 VRS
    bool SupportsVariableRateShading() const { return advanced.variableRateShading; }

    /// 最大 MSAA 采样数
    uint32_t GetMaxMSAASamples() const
    {
        if (limits.maxSampleCountMaskBits & (1u << 5)) return 64;
        if (limits.maxSampleCountMaskBits & (1u << 4)) return 32;
        if (limits.maxSampleCountMaskBits & (1u << 3)) return 16;
        if (limits.maxSampleCountMaskBits & (1u << 2)) return 8;
        if (limits.maxSampleCountMaskBits & (1u << 1)) return 4;
        if (limits.maxSampleCountMaskBits & (1u << 0)) return 2;
        return 1;
    }
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_DEVICE_FEATURES_INCLUDE_H */
