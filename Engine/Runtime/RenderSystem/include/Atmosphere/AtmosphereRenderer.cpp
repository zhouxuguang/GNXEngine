//
//  AtmosphereRenderer.cpp
//  GNXEngine
//
//  预计算大气散射 GPU 渲染器实现
//

#include "AtmosphereRenderer.h"
#include "AtmosphereConstant.h"
#include "ShaderAssetLoader.h"
#include "RenderEngine.h"
#include "Camera.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include <cstdio>
#include <tracy/Tracy.hpp>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

// ---------------------------------------------------------------------------
// 小工具
// ---------------------------------------------------------------------------
static RenderPassColorAttachmentPtr MakeColorAttachment(RCTexturePtr texture,
                                                        uint32_t slice,
                                                        bool loadExisting)
{
    auto attachment = std::make_shared<RenderPassColorAttachment>();
    attachment->texture = texture;
    attachment->slice = slice;
    attachment->loadOp = loadExisting ? ATTACHMENT_LOAD_OP_LOAD : ATTACHMENT_LOAD_OP_CLEAR;
    attachment->storeOp = ATTACHMENT_STORE_OP_STORE;
    attachment->clearColor = MakeClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    return attachment;
}

static void EnableAdditiveBlend(GraphicsPipelineDesc& desc, uint32_t target)
{
    ColorAttachmentDesc& c = desc.colorAttachmentDescriptors[target];
    c.blendingEnabled = true;
    c.sourceRGBBlendFactor = BlendFactorOne;
    c.destinationRGBBlendFactor = BlendFactorOne;
    c.rgbBlendOperation = BlendEquationAdd;
    c.sourceAlphaBlendFactor = BlendFactorOne;
    c.destinationAplhaBlendFactor = BlendFactorOne;
    c.aplhaBlendOperation = BlendEquationAdd;
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
AtmosphereRenderer::AtmosphereRenderer()
{
}

AtmosphereRenderer::~AtmosphereRenderer()
{
    DestroyResources();
}

bool AtmosphereRenderer::Initialize(const Atmosphere::AtmosphereParameters& params,
                                    unsigned int numScatteringOrders)
{
    if (mInitialized)
    {
        return true;
    }

    mNumScatteringOrders = numScatteringOrders < 1 ? 1 : numScatteringOrders;
    mSunAngularRadius = params.sun_angular_radius;
    mAtmosphereHeight = params.top_radius - params.bottom_radius;

    CreateResources();
    CreatePipelines();

    // 上传大气参数到 UBO
    if (mAtmosphereUBO)
    {
        mAtmosphereUBO->SetData(&params, 0, sizeof(Atmosphere::AtmosphereParameters));
    }

    mInitialized = true;
    return true;
}

void AtmosphereRenderer::CreateResources()
{
    RenderDevicePtr device = GetRenderDevice();

    const TextureUsage kUsage = TextureUsage::TextureUsageShaderRead |
                                TextureUsage::TextureUsageRenderTarget;
    const TextureFormat kFormat = RenderCore::kTexFormatRGBA16Float;

    // LUT 纹理
    mTransmittanceTexture = device->CreateTexture2D(
        kFormat, kUsage,
        Atmosphere::TRANSMITTANCE_TEXTURE_WIDTH,
        Atmosphere::TRANSMITTANCE_TEXTURE_HEIGHT, 1);

    mScatteringTexture = device->CreateTexture3D(
        kFormat, kUsage,
        Atmosphere::SCATTERING_TEXTURE_WIDTH,
        Atmosphere::SCATTERING_TEXTURE_HEIGHT,
        Atmosphere::SCATTERING_TEXTURE_DEPTH, 1);

    mSingleMieTexture = device->CreateTexture3D(
        kFormat, kUsage,
        Atmosphere::SCATTERING_TEXTURE_WIDTH,
        Atmosphere::SCATTERING_TEXTURE_HEIGHT,
        Atmosphere::SCATTERING_TEXTURE_DEPTH, 1);

    mIrradianceTexture = device->CreateTexture2D(
        kFormat, kUsage,
        Atmosphere::IRRADIANCE_TEXTURE_WIDTH,
        Atmosphere::IRRADIANCE_TEXTURE_HEIGHT, 1);

    // 中间纹理
    mDeltaIrradianceTexture = device->CreateTexture2D(
        kFormat, kUsage,
        Atmosphere::IRRADIANCE_TEXTURE_WIDTH,
        Atmosphere::IRRADIANCE_TEXTURE_HEIGHT, 1);

    mDeltaRayleighTexture = device->CreateTexture3D(
        kFormat, kUsage,
        Atmosphere::SCATTERING_TEXTURE_WIDTH,
        Atmosphere::SCATTERING_TEXTURE_HEIGHT,
        Atmosphere::SCATTERING_TEXTURE_DEPTH, 1);

    mDeltaMieTexture = device->CreateTexture3D(
        kFormat, kUsage,
        Atmosphere::SCATTERING_TEXTURE_WIDTH,
        Atmosphere::SCATTERING_TEXTURE_HEIGHT,
        Atmosphere::SCATTERING_TEXTURE_DEPTH, 1);

    mDeltaScatteringDensityTexture = device->CreateTexture3D(
        kFormat, kUsage,
        Atmosphere::SCATTERING_TEXTURE_WIDTH,
        Atmosphere::SCATTERING_TEXTURE_HEIGHT,
        Atmosphere::SCATTERING_TEXTURE_DEPTH, 1);

    // UBO
    mAtmosphereUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereParameters));
    mViewUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereViewParams));
    if (HasPlanetEllipsoid())
        mPlanetUBO = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmospherePlanetParams));

    // 采样器（线性、Clamp）
    SamplerDesc samplerDesc;
    samplerDesc.filterMag = MAG_LINEAR;
    samplerDesc.filterMin = MIN_LINEAR;
    samplerDesc.wrapS = CLAMP_TO_EDGE;
    samplerDesc.wrapT = CLAMP_TO_EDGE;
    samplerDesc.wrapR = CLAMP_TO_EDGE;
    mLinearSampler = device->CreateSamplerWithDescriptor(samplerDesc);
}

void AtmosphereRenderer::DestroyResources()
{
    mTransmittanceTexture.reset();
    mScatteringTexture.reset();
    mSingleMieTexture.reset();
    mIrradianceTexture.reset();
    mDeltaIrradianceTexture.reset();
    mDeltaRayleighTexture.reset();
    mDeltaMieTexture.reset();
    mDeltaScatteringDensityTexture.reset();
    mAtmosphereUBO.reset();
    mViewUBO.reset();
    mPlanetUBO.reset();
    mLinearSampler.reset();
    mPrecomputeUBOs.clear();
    mPrecomputeSteps.clear();
    mPrecomputeCursor = 0;
    mPrecomputed = false;

    mTransmittancePipeline.reset();
    mDirectIrradiancePipeline.reset();
    mSingleScatteringPipeline.reset();
    mScatteringDensityPipeline.reset();
    mIndirectIrradiancePipeline.reset();
    mMultipleScatteringPipeline.reset();
    mSkyPipeline.reset();
    mPlanetPipeline.reset();
}

void AtmosphereRenderer::CreatePipelines()
{
    RenderDevicePtr device = GetRenderDevice();

    // ---------- 透射率 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeTransmittance");
        info.graphicsPipelineDesc.renderTargetCount = 1;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        mTransmittancePipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mTransmittancePipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 直接辐照度 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeDirectIrradiance");
        info.graphicsPipelineDesc.renderTargetCount = 2;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        mDirectIrradiancePipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mDirectIrradiancePipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 单次散射 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeSingleScattering");
        info.graphicsPipelineDesc.renderTargetCount = 4;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        mSingleScatteringPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mSingleScatteringPipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 散射密度 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeScatteringDensity");
        info.graphicsPipelineDesc.renderTargetCount = 1;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        mScatteringDensityPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mScatteringDensityPipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 间接辐照度 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeIndirectIrradiance");
        info.graphicsPipelineDesc.renderTargetCount = 2;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        EnableAdditiveBlend(info.graphicsPipelineDesc, 1);  // 累积到 irradiance
        mIndirectIrradiancePipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mIndirectIrradiancePipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 多次散射 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/ComputeMultipleScattering");
        info.graphicsPipelineDesc.renderTargetCount = 2;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        EnableAdditiveBlend(info.graphicsPipelineDesc, 1);  // 累积到 scattering
        mMultipleScatteringPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mMultipleScatteringPipeline->AttachGraphicsShader(info.graphicsShader);
    }

    // ---------- 天空渲染 ----------
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo(mSkyShaderAsset);
        info.graphicsPipelineDesc.renderTargetCount = 1;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction =
            DepthConfig::GetSkyboxDepthCompareFunc();
        mSkyPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mSkyPipeline->AttachGraphicsShader(info.graphicsShader);
    }

    if (HasPlanetEllipsoid())
    {
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/AtmospherePlanetComposite");
        info.graphicsPipelineDesc.renderTargetCount = 1;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionAlways;
        mPlanetPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mPlanetPipeline->AttachGraphicsShader(info.graphicsShader);
    }
}

RenderEncoderPtr AtmosphereRenderer::BeginPass(
    CommandBufferPtr commandBuffer,
    GraphicsPipelinePtr pipeline,
    const std::vector<RenderPassColorAttachmentPtr>& colorAttachments,
    int width, int height,
    uint32_t layerCount)
{
    RenderPass renderPass;
    renderPass.renderRegion = Rect2D(0, 0, width, height);
    renderPass.colorAttachments = colorAttachments;
    // 重要：Metal 后端仅在 layerCount > 1 时才会为 3D 纹理设置 depthPlane（切片），
    // 否则所有层都会渲染到 slice 0，导致预计算的 3D LUT 只有第 0 层有效。
    renderPass.layerCount = layerCount;

    RenderEncoderPtr encoder = commandBuffer->CreateRenderEncoder(renderPass);
    if (encoder && pipeline)
    {
        encoder->SetGraphicsPipeline(pipeline);
    }
    return encoder;
}

// ---------------------------------------------------------------------------
// 预计算
// ---------------------------------------------------------------------------
UniformBufferPtr AtmosphereRenderer::GetScatteringUBO(int layer, int order)
{
    RenderDevicePtr device = GetRenderDevice();
    if (!device)
    {
        return nullptr;
    }

    // 命令缓冲区要等到帧结束才提交执行，UBO 必须存活到那时，因此由渲染器持有，
    // 并按 (layer, order) 复用。
    const uint64_t key = ((uint64_t)(uint32_t)layer << 32) | (uint32_t)order;
    auto iter = mPrecomputeUBOs.find(key);
    if (iter != mPrecomputeUBOs.end())
    {
        return iter->second;
    }

    UniformBufferPtr ubo = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereScatteringParams));
    Atmosphere::AtmosphereScatteringParams p;
    p.layer = layer;
    p.scattering_order = order;
    p.pad0 = 0;
    p.pad1 = 0;
    ubo->SetData(&p, 0, sizeof(p));
    mPrecomputeUBOs[key] = ubo;
    return ubo;
}

void AtmosphereRenderer::BuildPrecomputeSteps()
{
    mPrecomputeSteps.clear();
    mPrecomputeCursor = 0;

    const int kScatteringD = (int)Atmosphere::SCATTERING_TEXTURE_DEPTH;

    // 1) 透射率
    mPrecomputeSteps.push_back({PrecomputeStepType::Transmittance, 0, 0});
    // 2) 直接辐照度
    mPrecomputeSteps.push_back({PrecomputeStepType::DirectIrradiance, 0, 0});
    // 3) 单次散射（逐层）
    for (int layer = 0; layer < kScatteringD; ++layer)
    {
        mPrecomputeSteps.push_back({PrecomputeStepType::SingleScattering, layer, 0});
    }
    // 4) 多次散射迭代：散射密度（逐层）→ 间接辐照度 → 多次散射（逐层）
    for (unsigned int order = 2; order <= mNumScatteringOrders; ++order)
    {
        for (int layer = 0; layer < kScatteringD; ++layer)
        {
            mPrecomputeSteps.push_back({PrecomputeStepType::ScatteringDensity, layer, (int)order});
        }
        mPrecomputeSteps.push_back({PrecomputeStepType::IndirectIrradiance, 0, (int)order});
        for (int layer = 0; layer < kScatteringD; ++layer)
        {
            mPrecomputeSteps.push_back({PrecomputeStepType::MultipleScattering, layer, (int)order});
        }
    }
}

void AtmosphereRenderer::RunPrecomputeStep(CommandBufferPtr commandBuffer, const PrecomputeStep& step)
{
    static constexpr const char* kStepNames[] = {
        "Transmittance", "DirectIrradiance", "SingleScattering",
        "ScatteringDensity", "IndirectIrradiance", "MultipleScattering"
    };
    char debugName[128];
    std::snprintf(debugName, sizeof(debugName), "%s (layer=%d, order=%d)",
                  kStepNames[static_cast<size_t>(step.type)], step.layer, step.order);
    const float debugColor[4] = {0.4f, 0.7f, 1.0f, 1.0f};
    ScopedDebugMarker debugGroup(commandBuffer, debugName, debugColor);

    const int kScatteringW = (int)Atmosphere::SCATTERING_TEXTURE_WIDTH;
    const int kScatteringH = (int)Atmosphere::SCATTERING_TEXTURE_HEIGHT;
    const uint32_t kScatteringD = Atmosphere::SCATTERING_TEXTURE_DEPTH;
    const int kTransW = (int)Atmosphere::TRANSMITTANCE_TEXTURE_WIDTH;
    const int kTransH = (int)Atmosphere::TRANSMITTANCE_TEXTURE_HEIGHT;
    const int kIrrW = (int)Atmosphere::IRRADIANCE_TEXTURE_WIDTH;
    const int kIrrH = (int)Atmosphere::IRRADIANCE_TEXTURE_HEIGHT;

    const uint32_t layer = (uint32_t)step.layer;

    switch (step.type)
    {
    case PrecomputeStepType::Transmittance:
    {
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mTransmittancePipeline,
            { MakeColorAttachment(mTransmittanceTexture, 0, false) },
            kTransW, kTransH);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }

    case PrecomputeStepType::DirectIrradiance:
    {
        commandBuffer->ResourceBarrier(mDeltaIrradianceTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mDirectIrradiancePipeline,
            { MakeColorAttachment(mDeltaIrradianceTexture, 0, false),
              MakeColorAttachment(mIrradianceTexture, 0, false) },
            kIrrW, kIrrH);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }

    case PrecomputeStepType::SingleScattering:
    {
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mDeltaMieTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mScatteringTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mSingleMieTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mSingleScatteringPipeline,
            { MakeColorAttachment(mDeltaRayleighTexture, layer, false),
              MakeColorAttachment(mDeltaMieTexture, layer, false),
              MakeColorAttachment(mScatteringTexture, layer, false),
              MakeColorAttachment(mSingleMieTexture, layer, false) },
            kScatteringW, kScatteringH, kScatteringD);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentUniformBuffer("ScatteringCB", GetScatteringUBO(step.layer, 0));
        enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }

    case PrecomputeStepType::ScatteringDensity:
    {
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaMieTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaScatteringDensityTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mScatteringDensityPipeline,
            { MakeColorAttachment(mDeltaScatteringDensityTexture, layer, false) },
            kScatteringW, kScatteringH, kScatteringD);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentUniformBuffer("ScatteringCB", GetScatteringUBO(step.layer, step.order));
        enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("single_rayleigh_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("single_mie_scattering_texture", mDeltaMieTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("multiple_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("irradiance_texture", mIrradianceTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }

    case PrecomputeStepType::IndirectIrradiance:
    {
        commandBuffer->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaMieTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaIrradianceTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mIndirectIrradiancePipeline,
            { MakeColorAttachment(mDeltaIrradianceTexture, 0, false),
              MakeColorAttachment(mIrradianceTexture, 0, true) },
            kIrrW, kIrrH);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentUniformBuffer("ScatteringCB", GetScatteringUBO(0, step.order - 1));
        enc->SetFragmentTextureAndSampler("single_rayleigh_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("single_mie_scattering_texture", mDeltaMieTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("multiple_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }

    case PrecomputeStepType::MultipleScattering:
    {
        commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaScatteringDensityTexture, ResourceAccessType::ShaderRead);
        commandBuffer->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ColorAttachment);
        commandBuffer->ResourceBarrier(mScatteringTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(commandBuffer, mMultipleScatteringPipeline,
            { MakeColorAttachment(mDeltaRayleighTexture, layer, false),
              MakeColorAttachment(mScatteringTexture, layer, true) },
            kScatteringW, kScatteringH, kScatteringD);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentUniformBuffer("ScatteringCB", GetScatteringUBO(step.layer, step.order));
        enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
        enc->SetFragmentTextureAndSampler("scattering_density_texture", mDeltaScatteringDensityTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        break;
    }
    }
}

void AtmosphereRenderer::Precompute(CommandBufferPtr commandBuffer)
{
    ZoneScopedN("AtmosphereRenderer::Precompute");
    if (!mInitialized || !commandBuffer || mPrecomputed)
    {
        return;
    }

    const float debugColor[4] = {0.4f, 0.7f, 1.0f, 1.0f};
    ScopedDebugMarker debugGroup(commandBuffer, "Atmosphere Precompute", debugColor);

    if (mPrecomputeSteps.empty())
    {
        BuildPrecomputeSteps();
    }

    // 每帧只执行有限个 Pass，录制进当前帧的命令缓冲区，直到全部完成。
    //
    // 这么切分有两个必要原因：
    // 1) Vulkan / DX12 后端的 CreateCommandBuffer() 与交换链帧同步绑定（会 acquire
    //    交换链图像并重置飞行栅栏），不能为每个 Pass 单独建命令缓冲区——一次预计算
    //    有数百个 Pass，重复创建会耗尽交换链图像导致死锁，或让飞行栅栏永远等不到
    //    信号导致永久跳帧。
    // 2) 全部 Pass 塞进一帧会撑爆 DX12 的单帧描述符环（采样器堆硬件上限 2048），
    //    描述符被回绕复用后 LUT 内容会被写成 garbage（表现为天空颜色错误/NaN）。
    constexpr size_t kMaxStepsPerFrame = 24;

    size_t executed = 0;
    while (mPrecomputeCursor < mPrecomputeSteps.size() && executed < kMaxStepsPerFrame)
    {
        RunPrecomputeStep(commandBuffer, mPrecomputeSteps[mPrecomputeCursor]);
        ++mPrecomputeCursor;
        ++executed;
    }

    if (mPrecomputeCursor < mPrecomputeSteps.size())
    {
        return;
    }

    // 收尾：以上 Pass 把 LUT 当颜色附件写，需转到 SHADER_READ_ONLY 供天空 Pass 采样
    commandBuffer->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
    commandBuffer->ResourceBarrier(mScatteringTexture, ResourceAccessType::ShaderRead);
    commandBuffer->ResourceBarrier(mSingleMieTexture, ResourceAccessType::ShaderRead);
    commandBuffer->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ShaderRead);

    mPrecomputed = true;

    LOG_INFO("AtmosphereRenderer: precompute finished (orders=%u)", mNumScatteringOrders);
}

// ---------------------------------------------------------------------------
// 每帧视角参数
// ---------------------------------------------------------------------------
void AtmosphereRenderer::UpdateViewParams(const Camera* camera,
                                          const Vector3d& cameraWorldPosition,
                                          const Vector3d& planetCenter,
                                          const Vector3f& sunDirection,
                                          float exposure,
                                          const Vector3f& whitePoint)
{
    if (!mViewUBO || !camera) return;

    Atmosphere::AtmosphereViewParams vp{};
    Matrix4x4f viewRotation = camera->GetViewMatrix();
    viewRotation[0].w = 0.0f;
    viewRotation[1].w = 0.0f;
    viewRotation[2].w = 0.0f;
    vp.inv_view_proj = (camera->GetProjectionMatrix() * viewRotation).Inverse();

    const Vector3d relative = cameraWorldPosition - planetCenter;
    vp.camera_pos_exposure = make_simd_float4(static_cast<float>(relative.x),
        static_cast<float>(relative.y), static_cast<float>(relative.z), exposure);
    vp.sun_direction_pad = make_simd_float4(sunDirection.x, sunDirection.y, sunDirection.z, 0.0f);
    vp.sun_size_pad = make_simd_float4(tanf(mSunAngularRadius), cosf(mSunAngularRadius), 0.0f, 0.0f);
    vp.white_point_pad = make_simd_float4(whitePoint.x, whitePoint.y, whitePoint.z, 0.0f);
    mViewUBO->SetData(&vp, 0, sizeof(vp));
}

void AtmosphereRenderer::UpdatePlanetParams(const Vector3d& up, double altitude)
{
    if (!mPlanetUBO) return;
    Atmosphere::AtmospherePlanetParams params{};
    params.ground_radii_top_height = make_simd_float4(
        static_cast<float>(mPlanetRadii.x), static_cast<float>(mPlanetRadii.y),
        static_cast<float>(mPlanetRadii.z), mAtmosphereHeight);
    params.camera_up_altitude = make_simd_float4(
        static_cast<float>(up.x), static_cast<float>(up.y),
        static_cast<float>(up.z), static_cast<float>(altitude));
    mPlanetUBO->SetData(&params, 0, sizeof(params));
}

// ---------------------------------------------------------------------------
// 天空渲染
// ---------------------------------------------------------------------------
void AtmosphereRenderer::RenderSky(RenderEncoderPtr renderEncoder)
{
    if (!mPrecomputed || !renderEncoder || !mSkyPipeline)
    {
        return;
    }

    renderEncoder->SetGraphicsPipeline(mSkyPipeline);
    renderEncoder->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
    renderEncoder->SetFragmentUniformBuffer("AtmosphereViewCB", mViewUBO);
    if (mSkyExtraUniformBuffer && !mSkyExtraUniformName.empty())
        renderEncoder->SetFragmentUniformBuffer(mSkyExtraUniformName.c_str(), mSkyExtraUniformBuffer);
    renderEncoder->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("scattering_texture", mScatteringTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("single_mie_scattering_texture", mSingleMieTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("irradiance_texture", mIrradianceTexture, mLinearSampler);
    renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

void AtmosphereRenderer::RenderPlanetAtmosphere(RenderEncoderPtr renderEncoder,
                                                RCTexturePtr sceneColor,
                                                RCTexturePtr sceneDepth)
{
    if (!mPrecomputed || !renderEncoder || !mPlanetPipeline || !sceneColor || !sceneDepth)
        return;
    renderEncoder->SetGraphicsPipeline(mPlanetPipeline);
    renderEncoder->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
    renderEncoder->SetFragmentUniformBuffer("AtmosphereViewCB", mViewUBO);
    renderEncoder->SetFragmentUniformBuffer("AtmospherePlanetCB", mPlanetUBO);
    renderEncoder->SetFragmentTextureAndSampler("scene_color_texture", sceneColor, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("scene_depth_texture", sceneDepth, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("scattering_texture", mScatteringTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("single_mie_scattering_texture", mSingleMieTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("irradiance_texture", mIrradianceTexture, mLinearSampler);
    renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
