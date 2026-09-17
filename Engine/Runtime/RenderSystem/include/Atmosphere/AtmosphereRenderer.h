//
//  AtmosphereRenderer.h
//  GNXEngine
//
//  预计算大气散射（Precomputed Atmospheric Scattering）GPU 渲染器
//
//  参考: https://ebruneton.github.io/precomputed_atmospheric_scattering/
//
//  职责:
//    1) 预计算 LUT 纹理（一次性 / 参数变化时）：
//         - 透射率 transmittance        (2D  256x64)
//         - 散射累积 scattering           (3D  256x128x32, rayleigh+mie+多次散射)
//         - 单次 mie single_mie          (3D  256x128x32)
//         - 地面辐照度 irradiance        (2D  64x16)
//       中间纹理：
//         - delta_irradiance             (2D  64x16)
//         - delta_rayleigh(与 delta_multiple 共用) (3D 256x128x32)
//         - delta_mie                    (3D  256x128x32)
//         - delta_scattering_density     (3D  256x128x32)
//    2) 每帧根据 LUT 渲染天空（全屏）。
//

#ifndef GNX_ENGINE_ATMOSPHERE_RENDERER_IOSDGNJFHJSDGJ
#define GNX_ENGINE_ATMOSPHERE_RENDERER_IOSDGNJFHJSDGJ

#include "AtmosphereModel.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include <vector>
#include <map>
#include <memory>
#include <string>

NS_RENDERSYSTEM_BEGIN

class Camera;

class RENDERSYSTEM_API AtmosphereRenderer
{
public:
    AtmosphereRenderer();
    ~AtmosphereRenderer();

    // 使用 CPU 侧参数模型初始化：创建 LUT 纹理与渲染管线
    bool Initialize(const Atmosphere::AtmosphereParameters& params,
                    unsigned int numScatteringOrders = 4);

    bool IsInitialized() const
    {
        return mInitialized;
    }

    bool IsPrecomputed() const
    {
        return mPrecomputed;
    }

    /**
     * @brief 推进 LUT 预计算，把本帧要做的 Pass 录制进当前帧的命令缓冲区
     *
     * 每帧调用一次，直到 IsPrecomputed() 为真。Pass 被切成小批分摊到多帧执行：
     * 不要为每个 Pass 单独 CreateCommandBuffer()（Vulkan / DX12 的命令缓冲区与
     * 交换链帧同步绑定，重复创建会耗尽交换链图像导致死锁），也不要一次性塞进
     * 一帧（会撑爆 DX12 单帧描述符环，采样器堆硬件上限仅 2048）。
     */
    void Precompute(CommandBufferPtr commandBuffer);

    // 每帧更新视角参数（相机、地球中心、太阳方向、曝光、白点、场景几何体）
    void UpdateViewParams(const Camera* camera,
                          const mathutil::Vector3f& earthCenter,
                          const mathutil::Vector3f& sunDirection,
                          float exposure,
                          const mathutil::Vector3f& whitePoint,
                          const Atmosphere::AtmosphereSceneGeometry& geometry);

    // 渲染天空。调用者需提供颜色附件为场景颜色、深度附件为场景深度的 RenderEncoder。
    void RenderSky(RenderEncoderPtr renderEncoder);

private:
    void CreateResources();
    void CreatePipelines();
    void DestroyResources();

    // 预计算步骤（每个步骤 = 一个 draw 的 Pass），跨帧按游标推进
    enum class PrecomputeStepType
    {
        Transmittance,
        DirectIrradiance,
        SingleScattering,
        ScatteringDensity,
        IndirectIrradiance,
        MultipleScattering,
    };

    struct PrecomputeStep
    {
        PrecomputeStepType type = PrecomputeStepType::Transmittance;
        int layer = 0;
        int order = 0;
    };

    void BuildPrecomputeSteps();
    void RunPrecomputeStep(CommandBufferPtr commandBuffer, const PrecomputeStep& step);
    UniformBufferPtr GetScatteringUBO(int layer, int order);

    RenderEncoderPtr BeginPass(
        CommandBufferPtr commandBuffer,
        GraphicsPipelinePtr pipeline,
        const std::vector<RenderPassColorAttachmentPtr>& colorAttachments,
        int width, int height,
        uint32_t layerCount = 1);

    // 所有 LUT 纹理

    RCTexture2DPtr mTransmittanceTexture;
    RCTexture3DPtr mScatteringTexture;        // 累积：rayleigh + mie + 多次散射
    RCTexture3DPtr mSingleMieTexture;         // 单次 mie 散射
    RCTexture2DPtr mIrradianceTexture;        // 累积地面辐照度

    RCTexture2DPtr mDeltaIrradianceTexture;
    RCTexture3DPtr mDeltaRayleighTexture;     // 同时作为 delta_multiple_scattering
    RCTexture3DPtr mDeltaMieTexture;
    RCTexture3DPtr mDeltaScatteringDensityTexture;

    UniformBufferPtr mAtmosphereUBO;          // AtmosphereParametersCB
    UniformBufferPtr mViewUBO;                // AtmosphereViewCB

    // 预计算 Pass 用到的 (layer, order) 参数 UBO。命令缓冲区随帧提交，
    // 这些 UBO 必须存活到 GPU 执行完，故由渲染器持有并按 key 复用。
    std::map<uint64_t, UniformBufferPtr> mPrecomputeUBOs;

    std::vector<PrecomputeStep> mPrecomputeSteps;
    size_t mPrecomputeCursor = 0;

    TextureSamplerPtr mLinearSampler;

    GraphicsPipelinePtr mTransmittancePipeline;
    GraphicsPipelinePtr mDirectIrradiancePipeline;
    GraphicsPipelinePtr mSingleScatteringPipeline;
    GraphicsPipelinePtr mScatteringDensityPipeline;
    GraphicsPipelinePtr mIndirectIrradiancePipeline;
    GraphicsPipelinePtr mMultipleScatteringPipeline;
    GraphicsPipelinePtr mSkyPipeline;

    bool mInitialized = false;
    bool mPrecomputed = false;
    unsigned int mNumScatteringOrders = 4;
};

typedef std::shared_ptr<AtmosphereRenderer> AtmosphereRendererPtr;

// 依据默认参数创建大气模型（保留旧接口，供外部快速构建）
RENDERSYSTEM_API AtmosphereModel* CreateAtmoModel();

NS_RENDERSYSTEM_END

#endif // GNX_ENGINE_ATMOSPHERE_RENDERER_IOSDGNJFHJSDGJ
