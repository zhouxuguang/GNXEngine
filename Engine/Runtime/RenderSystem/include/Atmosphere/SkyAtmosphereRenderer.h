#ifndef GNX_SKY_ATMOSPHERE_RENDERER_H
#define GNX_SKY_ATMOSPHERE_RENDERER_H

#include "AtmosphereConstant.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include <memory>
#include <string>
#include <vector>

NS_RENDERSYSTEM_BEGIN

class Camera;

// Hillaire / UE Sky Atmosphere: static transmittance and multi-scattering LUTs,
// followed by view-dependent sky-view and aerial-perspective LUTs each frame.
// The legacy Bruneton renderer is independent and retains its resources.
class RENDERSYSTEM_API SkyAtmosphereRenderer
{
public:
    bool Initialize(const Atmosphere::AtmosphereParameters& params);
    bool IsInitialized() const { return mInitialized; }
    bool IsPrecomputed() const { return mStaticStep == 2; }
    bool IsDynamicReady() const { return mDynamicReady; }

    void SetCompositeShaderAsset(const std::string& asset) { mCompositeShaderAsset = asset; }
    void SetSkyExtraUniformBuffer(const std::string& name, UniformBufferPtr buffer)
    { mSkyExtraUniformName = name; mSkyExtraUniformBuffer = std::move(buffer); }
    void SetPlanetEllipsoidRadii(const mathutil::Vector3d& radii) { mPlanetRadii = radii; }
    void Precompute(CommandBufferPtr commandBuffer);
    void UpdateViewParams(const Camera* camera,
                          const mathutil::Vector3d& cameraWorldPosition,
                          const mathutil::Vector3d& planetCenter,
                          const mathutil::Vector3f& sunDirection,
                          float exposure,
                          const mathutil::Vector3f& whitePoint);
    void UpdatePlanetParams(const mathutil::Vector3d& up, double altitude);
    void UpdateDynamicLuts(CommandBufferPtr commandBuffer);
    void RenderComposite(RenderEncoderPtr encoder, RCTexturePtr sceneColor,
                         RCTexturePtr sceneDepth);

private:
    RenderEncoderPtr BeginPass(CommandBufferPtr commandBuffer, GraphicsPipelinePtr pipeline,
                               RCTexturePtr target, int width, int height,
                               uint32_t slice = 0, uint32_t layerCount = 1);

    RCTexture2DPtr mTransmittanceTexture;
    RCTexture2DPtr mMultiScatteringTexture;
    RCTexture2DPtr mSkyViewTexture;
    RCTexture3DPtr mAerialTexture;
    UniformBufferPtr mAtmosphereUBO;
    UniformBufferPtr mViewUBO;
    UniformBufferPtr mPlanetUBO;
    std::vector<UniformBufferPtr> mAerialSliceUBOs;
    TextureSamplerPtr mLinearSampler;
    GraphicsPipelinePtr mTransmittancePipeline;
    GraphicsPipelinePtr mMultiScatteringPipeline;
    GraphicsPipelinePtr mSkyViewPipeline;
    GraphicsPipelinePtr mAerialPipeline;
    GraphicsPipelinePtr mCompositePipeline;
    mathutil::Vector3d mPlanetRadii{0.0, 0.0, 0.0};
    float mBottomRadius = 0.0f;
    float mAtmosphereHeight = 0.0f;
    float mSunAngularRadius = 0.0f;
    std::string mCompositeShaderAsset = "Atmosphere/NewSkyComposite";
    std::string mSkyExtraUniformName;
    UniformBufferPtr mSkyExtraUniformBuffer;
    int mStaticStep = 0;
    bool mInitialized = false;
    bool mDynamicReady = false;
};

using SkyAtmosphereRendererPtr = std::shared_ptr<SkyAtmosphereRenderer>;

NS_RENDERSYSTEM_END

#endif
