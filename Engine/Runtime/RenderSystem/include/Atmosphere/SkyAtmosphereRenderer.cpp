#include "SkyAtmosphereRenderer.h"
#include "ShaderAssetLoader.h"
#include "RenderEngine.h"
#include "Camera.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <cmath>
#include <cstdio>
#include <tracy/Tracy.hpp>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

namespace
{
constexpr float kSkyDebugColor[4] = {0.4f, 0.7f, 1.0f, 1.0f};

struct alignas(16) SkyLutParams
{
    float slice;
    float depthRangeKm;
    float sliceCount;
    float pad;
};
static_assert(sizeof(SkyLutParams) == 16);

GraphicsPipelinePtr MakeLutPipeline(RenderDevicePtr device, const char* asset)
{
    GraphicsShaderInfo info = CreateGraphicsShaderInfo(asset);
    info.graphicsPipelineDesc.renderTargetCount = 1;
    info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    auto pipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
    if (pipeline) pipeline->AttachGraphicsShader(info.graphicsShader);
    return pipeline;
}
}

bool SkyAtmosphereRenderer::Initialize(const Atmosphere::AtmosphereParameters& params)
{
    if (mInitialized) return true;
    RenderDevicePtr device = GetRenderDevice();
    if (!device) return false;

    mBottomRadius = params.bottom_radius;
    mAtmosphereHeight = params.top_radius - params.bottom_radius;
    mSunAngularRadius = params.sun_angular_radius;
    if (mPlanetRadii.x <= 0.0 || mPlanetRadii.y <= 0.0 || mPlanetRadii.z <= 0.0)
        mPlanetRadii = Vector3d(mBottomRadius, mBottomRadius, mBottomRadius);

    const TextureUsage usage = TextureUsage::TextureUsageShaderRead |
                               TextureUsage::TextureUsageRenderTarget;
    const TextureFormat format = RenderCore::kTexFormatRGBA16Float;
    mTransmittanceTexture = device->CreateTexture2D(format, usage,
        Atmosphere::TRANSMITTANCE_TEXTURE_WIDTH, Atmosphere::TRANSMITTANCE_TEXTURE_HEIGHT, 1);
    mMultiScatteringTexture = device->CreateTexture2D(format, usage,
        Atmosphere::SKY_MULTI_SCATTERING_WIDTH, Atmosphere::SKY_MULTI_SCATTERING_HEIGHT, 1);
    mSkyViewTexture = device->CreateTexture2D(format, usage,
        Atmosphere::SKY_VIEW_WIDTH, Atmosphere::SKY_VIEW_HEIGHT, 1);
    mAerialTexture = device->CreateTexture3D(format, usage,
        Atmosphere::SKY_AERIAL_WIDTH, Atmosphere::SKY_AERIAL_HEIGHT,
        Atmosphere::SKY_AERIAL_DEPTH, 1);
    mAtmosphereUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereParameters));
    mViewUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereViewParams));
    mPlanetUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmospherePlanetParams));
    if (!mTransmittanceTexture || !mMultiScatteringTexture || !mSkyViewTexture ||
        !mAerialTexture || !mAtmosphereUBO || !mViewUBO || !mPlanetUBO)
        return false;
    mAtmosphereUBO->SetData(&params, 0, sizeof(params));

    mAerialSliceUBOs.reserve(Atmosphere::SKY_AERIAL_DEPTH);
    for (uint32_t i = 0; i < Atmosphere::SKY_AERIAL_DEPTH; ++i)
    {
        auto buffer = device->CreateUniformBufferWithSize(sizeof(SkyLutParams));
        if (!buffer) return false;
        const SkyLutParams lut{float(i), Atmosphere::SKY_AERIAL_DISTANCE_METERS / 1000.0f,
                               float(Atmosphere::SKY_AERIAL_DEPTH), 0.0f};
        buffer->SetData(&lut, 0, sizeof(lut));
        mAerialSliceUBOs.push_back(buffer);
    }

    SamplerDesc samplerDesc;
    samplerDesc.filterMag = MAG_LINEAR;
    samplerDesc.filterMin = MIN_LINEAR;
    samplerDesc.wrapS = CLAMP_TO_EDGE;
    samplerDesc.wrapT = CLAMP_TO_EDGE;
    samplerDesc.wrapR = CLAMP_TO_EDGE;
    mLinearSampler = device->CreateSamplerWithDescriptor(samplerDesc);

    mTransmittancePipeline = MakeLutPipeline(device, "Atmosphere/NewSkyTransmittance");
    mMultiScatteringPipeline = MakeLutPipeline(device, "Atmosphere/NewSkyMultiScattering");
    mSkyViewPipeline = MakeLutPipeline(device, "Atmosphere/NewSkyView");
    mAerialPipeline = MakeLutPipeline(device, "Atmosphere/NewSkyAerialPerspective");
    mCompositePipeline = MakeLutPipeline(device, mCompositeShaderAsset.c_str());
    mInitialized = mLinearSampler && mTransmittancePipeline && mMultiScatteringPipeline &&
                   mSkyViewPipeline && mAerialPipeline && mCompositePipeline;
    if (mInitialized)
        LOG_INFO("SkyAtmosphereRenderer: initialized four LUTs (256x64, 32x32, 192x104, 32x32x16)");
    return mInitialized;
}

RenderEncoderPtr SkyAtmosphereRenderer::BeginPass(CommandBufferPtr commandBuffer,
    GraphicsPipelinePtr pipeline, RCTexturePtr target, int width, int height,
    uint32_t slice, uint32_t layerCount)
{
    RenderPass pass;
    pass.renderRegion = Rect2D(0, 0, width, height);
    // Metal requires the full depth-plane count to select a 3D slice.
    pass.layerCount = layerCount;
    auto attachment = std::make_shared<RenderPassColorAttachment>();
    attachment->texture = target;
    attachment->slice = slice;
    attachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
    attachment->storeOp = ATTACHMENT_STORE_OP_STORE;
    attachment->clearColor = MakeClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    pass.colorAttachments.push_back(attachment);
    auto encoder = commandBuffer->CreateRenderEncoder(pass);
    if (encoder && pipeline) encoder->SetGraphicsPipeline(pipeline);
    return encoder;
}

void SkyAtmosphereRenderer::Precompute(CommandBufferPtr commandBuffer)
{
    ZoneScopedN("SkyAtmosphereRenderer::Precompute");
    if (!mInitialized || !commandBuffer || IsPrecomputed()) return;
    ScopedDebugMarker precomputeGroup(commandBuffer, "SkyAtmosphere Precompute", kSkyDebugColor);
    if (mStaticStep == 0)
    {
        ScopedDebugMarker passGroup(commandBuffer, "Transmittance LUT", kSkyDebugColor);
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ColorAttachment);
        auto enc = BeginPass(commandBuffer, mTransmittancePipeline, mTransmittanceTexture,
            Atmosphere::TRANSMITTANCE_TEXTURE_WIDTH, Atmosphere::TRANSMITTANCE_TEXTURE_HEIGHT);
        if (!enc) return;
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        ++mStaticStep;
        return;
    }
    ScopedDebugMarker passGroup(commandBuffer, "MultiScattering LUT", kSkyDebugColor);
    commandBuffer->ResourceBarrier(mMultiScatteringTexture, ResourceAccessType::ColorAttachment);
    auto enc = BeginPass(commandBuffer, mMultiScatteringPipeline, mMultiScatteringTexture,
        Atmosphere::SKY_MULTI_SCATTERING_WIDTH, Atmosphere::SKY_MULTI_SCATTERING_HEIGHT);
    if (!enc) return;
    enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
    enc->SetFragmentTextureAndSampler("new_sky_transmittance_texture",
        mTransmittanceTexture, mLinearSampler);
    enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
    enc->EndEncode();
    commandBuffer->ResourceBarrier(mMultiScatteringTexture, ResourceAccessType::ShaderRead);
    ++mStaticStep;
    LOG_INFO("SkyAtmosphereRenderer: static LUTs ready");
}

void SkyAtmosphereRenderer::UpdateViewParams(const Camera* camera,
    const Vector3d& cameraWorldPosition, const Vector3d& planetCenter,
    const Vector3f& sunDirection, float exposure, const Vector3f& whitePoint)
{
    if (!mViewUBO || !camera) return;
    Atmosphere::AtmosphereViewParams vp{};
    Matrix4x4f viewRotation = camera->GetViewMatrix();
    viewRotation[0].w = 0.0f;
    viewRotation[1].w = 0.0f;
    viewRotation[2].w = 0.0f;
    vp.inv_view_proj = (camera->GetProjectionMatrix() * viewRotation).Inverse();
    const Vector3d relative = cameraWorldPosition - planetCenter;
    vp.camera_pos_exposure = make_simd_float4(float(relative.x), float(relative.y),
                                              float(relative.z), exposure);
    vp.sun_direction_pad = make_simd_float4(sunDirection.x, sunDirection.y, sunDirection.z, 0.0f);
    vp.sun_size_pad = make_simd_float4(std::tan(mSunAngularRadius),
                                      std::cos(mSunAngularRadius), 0.0f, 0.0f);
    vp.white_point_pad = make_simd_float4(whitePoint.x, whitePoint.y, whitePoint.z, 0.0f);
    mViewUBO->SetData(&vp, 0, sizeof(vp));

    // Default sphere for the standalone demo. The map overrides this with the
    // double-precision ellipsoid geodetic frame immediately afterward.
    const double r = std::sqrt(relative.x * relative.x + relative.y * relative.y +
                               relative.z * relative.z);
    if (r > 0.0)
        UpdatePlanetParams(Vector3d(relative.x / r, relative.y / r, relative.z / r),
                           r - mBottomRadius);
    mDynamicReady = false;
}

void SkyAtmosphereRenderer::UpdatePlanetParams(const Vector3d& up, double altitude)
{
    if (!mPlanetUBO) return;
    Atmosphere::AtmospherePlanetParams params{};
    params.ground_radii_top_height = make_simd_float4(float(mPlanetRadii.x),
        float(mPlanetRadii.y), float(mPlanetRadii.z), mAtmosphereHeight);
    params.camera_up_altitude = make_simd_float4(float(up.x), float(up.y), float(up.z),
                                                 float(altitude));
    mPlanetUBO->SetData(&params, 0, sizeof(params));
}

void SkyAtmosphereRenderer::UpdateDynamicLuts(CommandBufferPtr commandBuffer)
{
    ZoneScopedN("SkyAtmosphereRenderer::UpdateDynamicLuts");
    if (!IsPrecomputed() || !commandBuffer || !mViewUBO || !mPlanetUBO) return;
    ScopedDebugMarker dynamicGroup(commandBuffer, "SkyAtmosphere Dynamic LUTs", kSkyDebugColor);
    {
        ScopedDebugMarker passGroup(commandBuffer, "SkyView LUT", kSkyDebugColor);
        commandBuffer->ResourceBarrier(mSkyViewTexture, ResourceAccessType::ColorAttachment);
        auto sky = BeginPass(commandBuffer, mSkyViewPipeline, mSkyViewTexture,
            Atmosphere::SKY_VIEW_WIDTH, Atmosphere::SKY_VIEW_HEIGHT);
        if (!sky) return;
        sky->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        sky->SetFragmentUniformBuffer("AtmosphereViewCB", mViewUBO);
        sky->SetFragmentUniformBuffer("AtmospherePlanetCB", mPlanetUBO);
        sky->SetFragmentTextureAndSampler("new_sky_transmittance_texture",
            mTransmittanceTexture, mLinearSampler);
        sky->SetFragmentTextureAndSampler("new_sky_multiscattering_texture",
            mMultiScatteringTexture, mLinearSampler);
        sky->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        sky->EndEncode();
        commandBuffer->ResourceBarrier(mSkyViewTexture, ResourceAccessType::ShaderRead);
    }

    ScopedDebugMarker aerialGroup(commandBuffer, "AerialPerspective LUT", kSkyDebugColor);
    commandBuffer->ResourceBarrier(mAerialTexture, ResourceAccessType::ColorAttachment);
    for (uint32_t slice = 0; slice < Atmosphere::SKY_AERIAL_DEPTH; ++slice)
    {
        char debugName[64];
        std::snprintf(debugName, sizeof(debugName), "AerialPerspective (slice=%u)", slice);
        ScopedDebugMarker sliceGroup(commandBuffer, debugName, kSkyDebugColor);
        auto aerial = BeginPass(commandBuffer, mAerialPipeline, mAerialTexture,
            Atmosphere::SKY_AERIAL_WIDTH, Atmosphere::SKY_AERIAL_HEIGHT,
            slice, Atmosphere::SKY_AERIAL_DEPTH);
        if (!aerial) return;
        aerial->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        aerial->SetFragmentUniformBuffer("AtmosphereViewCB", mViewUBO);
        aerial->SetFragmentUniformBuffer("AtmospherePlanetCB", mPlanetUBO);
        aerial->SetFragmentUniformBuffer("NewSkyLutCB", mAerialSliceUBOs[slice]);
        aerial->SetFragmentTextureAndSampler("new_sky_transmittance_texture",
            mTransmittanceTexture, mLinearSampler);
        aerial->SetFragmentTextureAndSampler("new_sky_multiscattering_texture",
            mMultiScatteringTexture, mLinearSampler);
        aerial->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        aerial->EndEncode();
    }
    commandBuffer->ResourceBarrier(mAerialTexture, ResourceAccessType::ShaderRead);
    mDynamicReady = true;
}

void SkyAtmosphereRenderer::RenderComposite(RenderEncoderPtr encoder,
    RCTexturePtr sceneColor, RCTexturePtr sceneDepth)
{
    if (!mDynamicReady || !encoder || !sceneColor || !sceneDepth || !mCompositePipeline) return;
    encoder->SetGraphicsPipeline(mCompositePipeline);
    encoder->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
    encoder->SetFragmentUniformBuffer("AtmosphereViewCB", mViewUBO);
    encoder->SetFragmentUniformBuffer("AtmospherePlanetCB", mPlanetUBO);
    if (mSkyExtraUniformBuffer && !mSkyExtraUniformName.empty())
        encoder->SetFragmentUniformBuffer(mSkyExtraUniformName.c_str(), mSkyExtraUniformBuffer);
    encoder->SetFragmentTextureAndSampler("scene_color_texture", sceneColor, mLinearSampler);
    encoder->SetFragmentTextureAndSampler("scene_depth_texture", sceneDepth, mLinearSampler);
    encoder->SetFragmentTextureAndSampler("new_sky_transmittance_texture",
        mTransmittanceTexture, mLinearSampler);
    encoder->SetFragmentTextureAndSampler("new_sky_multiscattering_texture",
        mMultiScatteringTexture, mLinearSampler);
    encoder->SetFragmentTextureAndSampler("new_sky_view_texture",
        mSkyViewTexture, mLinearSampler);
    encoder->SetFragmentTextureAndSampler("new_sky_aerial_texture",
        mAerialTexture, mLinearSampler);
    encoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
