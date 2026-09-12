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

    bool IsInitialized() const { return mInitialized; }
    bool IsPrecomputed() const { return mPrecomputed; }

    // 执行一次完整的预计算（生成所有 LUT）
    void Precompute();

    // 每帧更新视角参数（相机、地球中心、太阳方向、曝光、白点）
    void UpdateViewParams(const Camera* camera,
                          const mathutil::Vector3f& earthCenter,
                          const mathutil::Vector3f& sunDirection,
                          float exposure,
                          const mathutil::Vector3f& whitePoint);

    // 渲染天空。调用者需提供颜色附件为场景颜色、深度附件为场景深度的 RenderEncoder。
    void RenderSky(RenderEncoderPtr renderEncoder);

private:
    void CreateResources();
    void CreatePipelines();
    void DestroyResources();

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
