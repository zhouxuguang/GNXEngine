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
#include "Runtime/RenderCore/include/CommandQueue.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
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
    mLinearSampler.reset();

    mTransmittancePipeline.reset();
    mDirectIrradiancePipeline.reset();
    mSingleScatteringPipeline.reset();
    mScatteringDensityPipeline.reset();
    mIndirectIrradiancePipeline.reset();
    mMultipleScatteringPipeline.reset();
    mSkyPipeline.reset();
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
        GraphicsShaderInfo info = CreateGraphicsShaderInfo("Atmosphere/AtmosphereShader");
        info.graphicsPipelineDesc.renderTargetCount = 1;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
        info.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction =
            DepthConfig::GetSkyboxDepthCompareFunc();
        mSkyPipeline = device->CreateGraphicsPipeline(info.graphicsPipelineDesc);
        mSkyPipeline->AttachGraphicsShader(info.graphicsShader);
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
void AtmosphereRenderer::Precompute()
{
    ZoneScopedN("AtmosphereRenderer::Precompute");
    if (!mInitialized)
    {
        return;
    }

    RenderDevicePtr device = GetRenderDevice();
    CommandQueuePtr queue = device->GetCommandQueue(QueueType::Graphics, 0);
    if (!queue)
    {
        return;
    }

    const uint32_t kScatteringW = Atmosphere::SCATTERING_TEXTURE_WIDTH;
    const uint32_t kScatteringH = Atmosphere::SCATTERING_TEXTURE_HEIGHT;
    const uint32_t kScatteringD = Atmosphere::SCATTERING_TEXTURE_DEPTH;
    const uint32_t kTransW = Atmosphere::TRANSMITTANCE_TEXTURE_WIDTH;
    const uint32_t kTransH = Atmosphere::TRANSMITTANCE_TEXTURE_HEIGHT;
    const uint32_t kIrrW = Atmosphere::IRRADIANCE_TEXTURE_WIDTH;
    const uint32_t kIrrH = Atmosphere::IRRADIANCE_TEXTURE_HEIGHT;

    // 保持每层/每重数参数的 UBO 存活到命令执行完成
    std::vector<UniformBufferPtr> paramUBOs;
    auto makeScatteringUBO = [&](int layer, int order) -> UniformBufferPtr {
        UniformBufferPtr ubo = device->CreateUniformBufferWithSize(sizeof(Atmosphere::AtmosphereScatteringParams));
        Atmosphere::AtmosphereScatteringParams p;
        p.layer = layer;
        p.scattering_order = order;
        p.pad0 = 0;
        p.pad1 = 0;
        ubo->SetData(&p, 0, sizeof(p));
        paramUBOs.push_back(ubo);
        return ubo;
    };

    // 关键：每个 Pass 使用独立命令缓冲区并 Submit + WaitUntilCompleted。
    // Metal(TBDR) 下，跨编码器读取“上一个 Pass 作为渲染目标写入”的纹理需要显式同步，
    // 否则可能读到未定义数据（表现为 NaN），导致预计算的 LUT 出错（天空全黑）。
    auto flush = [](CommandBufferPtr& cmd) {
        if (cmd)
        {
            cmd->Submit();
            cmd->WaitUntilCompleted();
        }
    };

    // ---- 1) 透射率 ----
    {
        CommandBufferPtr cmd = queue->CreateCommandBuffer();
        cmd->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ColorAttachment);
        RenderEncoderPtr enc = BeginPass(cmd, mTransmittancePipeline,
            { MakeColorAttachment(mTransmittanceTexture, 0, false) },
            (int)kTransW, (int)kTransH);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        flush(cmd);
    }

    // ---- 2) 直接辐照度 -> [delta_irradiance, irradiance] ----
    {
        CommandBufferPtr cmd = queue->CreateCommandBuffer();
        cmd->ResourceBarrier(mDeltaIrradianceTexture, ResourceAccessType::ColorAttachment);
        cmd->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ColorAttachment);
        cmd->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        RenderEncoderPtr enc = BeginPass(cmd, mDirectIrradiancePipeline,
            { MakeColorAttachment(mDeltaIrradianceTexture, 0, false),
              MakeColorAttachment(mIrradianceTexture, 0, false) },
            (int)kIrrW, (int)kIrrH);
        enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
        enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
        enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
        enc->EndEncode();
        flush(cmd);
    }

    // ---- 3) 单次散射 -> [delta_rayleigh, delta_mie, scattering, single_mie] ----
    {
        CommandBufferPtr cmd = queue->CreateCommandBuffer();
        cmd->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
        for (int layer = 0; layer < (int)kScatteringD; ++layer)
        {
            cmd->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ColorAttachment);
            cmd->ResourceBarrier(mDeltaMieTexture, ResourceAccessType::ColorAttachment);
            cmd->ResourceBarrier(mScatteringTexture, ResourceAccessType::ColorAttachment);
            cmd->ResourceBarrier(mSingleMieTexture, ResourceAccessType::ColorAttachment);

            RenderEncoderPtr enc = BeginPass(cmd, mSingleScatteringPipeline,
                { MakeColorAttachment(mDeltaRayleighTexture, (uint32_t)layer, false),
                  MakeColorAttachment(mDeltaMieTexture, (uint32_t)layer, false),
                  MakeColorAttachment(mScatteringTexture, (uint32_t)layer, false),
                  MakeColorAttachment(mSingleMieTexture, (uint32_t)layer, false) },
                (int)kScatteringW, (int)kScatteringH, kScatteringD);
            enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
            enc->SetFragmentUniformBuffer("ScatteringCB", makeScatteringUBO(layer, 0));
            enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
            enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            enc->EndEncode();
        }
        flush(cmd);
    }

    // ---- 4) 多次散射迭代 ----
    for (unsigned int order = 2; order <= mNumScatteringOrders; ++order)
    {
        // 4a) 散射密度 -> delta_scattering_density
        {
            CommandBufferPtr cmd = queue->CreateCommandBuffer();
            cmd->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
            cmd->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ShaderRead);
            cmd->ResourceBarrier(mDeltaMieTexture, ResourceAccessType::ShaderRead);
            cmd->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ShaderRead);
            for (int layer = 0; layer < (int)kScatteringD; ++layer)
            {
                cmd->ResourceBarrier(mDeltaScatteringDensityTexture, ResourceAccessType::ColorAttachment);
                RenderEncoderPtr enc = BeginPass(cmd, mScatteringDensityPipeline,
                    { MakeColorAttachment(mDeltaScatteringDensityTexture, (uint32_t)layer, false) },
                    (int)kScatteringW, (int)kScatteringH, kScatteringD);
                enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
                enc->SetFragmentUniformBuffer("ScatteringCB", makeScatteringUBO(layer, (int)order));
                enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
                enc->SetFragmentTextureAndSampler("single_rayleigh_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
                enc->SetFragmentTextureAndSampler("single_mie_scattering_texture", mDeltaMieTexture, mLinearSampler);
                enc->SetFragmentTextureAndSampler("multiple_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
                enc->SetFragmentTextureAndSampler("irradiance_texture", mIrradianceTexture, mLinearSampler);
                enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
                enc->EndEncode();
            }
            flush(cmd);
        }

        // 4b) 间接辐照度 -> [delta_irradiance, irradiance(混合累积)]
        {
            CommandBufferPtr cmd = queue->CreateCommandBuffer();
            cmd->ResourceBarrier(mDeltaIrradianceTexture, ResourceAccessType::ColorAttachment);
            cmd->ResourceBarrier(mIrradianceTexture, ResourceAccessType::ColorAttachment);
            RenderEncoderPtr enc = BeginPass(cmd, mIndirectIrradiancePipeline,
                { MakeColorAttachment(mDeltaIrradianceTexture, 0, false),
                  MakeColorAttachment(mIrradianceTexture, 0, true) },
                (int)kIrrW, (int)kIrrH);
            enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
            enc->SetFragmentUniformBuffer("ScatteringCB", makeScatteringUBO(0, (int)order - 1));
            enc->SetFragmentTextureAndSampler("single_rayleigh_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
            enc->SetFragmentTextureAndSampler("single_mie_scattering_texture", mDeltaMieTexture, mLinearSampler);
            enc->SetFragmentTextureAndSampler("multiple_scattering_texture", mDeltaRayleighTexture, mLinearSampler);
            enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            enc->EndEncode();
            flush(cmd);
        }

        // 4c) 多次散射 -> [delta_multiple(=delta_rayleigh), scattering(混合累积)]
        {
            CommandBufferPtr cmd = queue->CreateCommandBuffer();
            cmd->ResourceBarrier(mTransmittanceTexture, ResourceAccessType::ShaderRead);
            cmd->ResourceBarrier(mDeltaScatteringDensityTexture, ResourceAccessType::ShaderRead);
            for (int layer = 0; layer < (int)kScatteringD; ++layer)
            {
                cmd->ResourceBarrier(mDeltaRayleighTexture, ResourceAccessType::ColorAttachment);
                cmd->ResourceBarrier(mScatteringTexture, ResourceAccessType::ColorAttachment);
                RenderEncoderPtr enc = BeginPass(cmd, mMultipleScatteringPipeline,
                    { MakeColorAttachment(mDeltaRayleighTexture, (uint32_t)layer, false),
                      MakeColorAttachment(mScatteringTexture, (uint32_t)layer, true) },
                    (int)kScatteringW, (int)kScatteringH, kScatteringD);
                enc->SetFragmentUniformBuffer("AtmosphereParametersCB", mAtmosphereUBO);
                enc->SetFragmentUniformBuffer("ScatteringCB", makeScatteringUBO(layer, (int)order));
                enc->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
                enc->SetFragmentTextureAndSampler("scattering_density_texture", mDeltaScatteringDensityTexture, mLinearSampler);
                enc->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
                enc->EndEncode();
            }
            flush(cmd);
        }
    }

    mPrecomputed = true;

    LOG_INFO("AtmosphereRenderer: precompute finished (orders=%u)", mNumScatteringOrders);
}

// ---------------------------------------------------------------------------
// 每帧视角参数
// ---------------------------------------------------------------------------
void AtmosphereRenderer::UpdateViewParams(const Camera* camera,
                                          const Vector3f& earthCenter,
                                          const Vector3f& sunDirection,
                                          float exposure,
                                          const Vector3f& whitePoint)
{
    if (!mViewUBO || !camera)
    {
        return;
    }

    Atmosphere::AtmosphereViewParams vp{};
    Matrix4x4f vpMat = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    vp.inv_view_proj = vpMat.Inverse();

    const Vector3f& camPos = camera->GetPosition();
    vp.camera_pos_exposure = make_simd_float4(camPos.x, camPos.y, camPos.z, exposure);
    vp.earth_center_pad = make_simd_float4(earthCenter.x, earthCenter.y, earthCenter.z, 0.0f);
    vp.sun_direction_pad = make_simd_float4(sunDirection.x, sunDirection.y, sunDirection.z, 0.0f);

    // sun_size = (tan(sunAngularRadius), cos(sunAngularRadius))
    const float sunAngularRadius = Atmosphere::kSunAngularRadius;
    vp.sun_size_pad = make_simd_float4(tanf(sunAngularRadius), cosf(sunAngularRadius), 0.0f, 0.0f);
    vp.white_point_pad = make_simd_float4(whitePoint.x, whitePoint.y, whitePoint.z, 0.0f);

    mViewUBO->SetData(&vp, 0, sizeof(vp));
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
    renderEncoder->SetFragmentTextureAndSampler("transmittance_texture", mTransmittanceTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("scattering_texture", mScatteringTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("single_mie_scattering_texture", mSingleMieTexture, mLinearSampler);
    renderEncoder->SetFragmentTextureAndSampler("irradiance_texture", mIrradianceTexture, mLinearSampler);
    renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

// ---------------------------------------------------------------------------
// 默认大气模型构建（保留旧接口）
// ---------------------------------------------------------------------------
AtmosphereModel* CreateAtmoModel()
{
    // 太阳光谱
    constexpr int kLambdaMin = 360;
    constexpr int kLambdaMax = 830;
    constexpr double kSolarIrradiance[48] = {
        1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.68870, 1.61253,
        1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.82980,
        1.86850, 1.89310, 1.85149, 1.85040, 1.83410, 1.83450, 1.81470, 1.78158, 1.7533,
        1.69650, 1.68194, 1.64654, 1.60480, 1.52143, 1.55622, 1.51130, 1.47400, 1.4482,
        1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.23670, 1.20820,
        1.18737, 1.14683, 1.12362, 1.10580, 1.07124, 1.04992
    };
    // http://www.iup.uni-bremen.de/gruppen/molspec/databases
    // /referencespectra/o3spectra2011/index.html
    constexpr double kOzoneCrossSection[48] = {
        1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
        8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
        1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
        4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
        2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
        6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
        2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
    };
    // https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
    constexpr double kDobsonUnit = 2.687e20;
    // 臭氧层最大密度
    constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
    // 与波长无关的太阳辐照度光谱
    constexpr double kConstantSolarIrradiance = 1.5;
    constexpr double kBottomRadius = 6360000.0;
    constexpr double kTopRadius = 6420000.0;
    constexpr double kRayleigh = 1.24062e-6;
    // rayleigh散射缩放高度
    constexpr double kRayleighScaleHeight = 8000.0;
    // mie散射缩放高度
    constexpr double kMieScaleHeight = 1200.0;
    constexpr double kMieAngstromAlpha = 0.0;
    constexpr double kMieAngstromBeta = 5.328e-3;
    constexpr double kMieSingleScatteringAlbedo = 0.9;

    double kMiePhaseFunctionG = 0.8;
    constexpr double kGroundAlbedo = 0.1;
    const double max_sun_zenith_angle = (120.0) / 180.0 * Atmosphere::kPi;

    // rayleigh层，即空气分子层     //width,exp_term,exp_scale,linear_term,constant_term
    DensityProfileLayer rayleigh_layer(0.0, 1.0,
                                       -1.0 / kRayleighScaleHeight,
                                       0.0, 0.0);
    // mie层，气溶胶层            //width,exp_term,exp_scale,linear_term,constant_term
    DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
    std::vector<DensityProfileLayer> ozone_density;
    ozone_density.push_back(DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
    ozone_density.push_back(DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

    std::vector<double> wavelengths;            // 波长
    std::vector<double> solar_irradiance;       // 太阳辐照度
    std::vector<double> rayleigh_scattering;    // rayleigh散射
    std::vector<double> mie_scattering;         // mie散射
    std::vector<double> mie_extinction;         // mie消光
    std::vector<double> absorption_extinction;  // 吸收光线的空气分子消光
    std::vector<double> ground_albedo;          // 地面反照率
    for (int l = kLambdaMin; l <= kLambdaMax; l += 10)
    {
        double lambda = static_cast<double>(l) * 1e-3;
        double mie = kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
        // 太阳光波波长
        wavelengths.push_back(l);

        // 太阳辐照度
        if (0)  // use_constant_solar_spectrum_
        {
            solar_irradiance.push_back(kConstantSolarIrradiance);
        }
        else
        {
            solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
        }
        rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
        mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
        mie_extinction.push_back(mie);
        absorption_extinction.push_back(1 * kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10]);
        ground_albedo.push_back(kGroundAlbedo);
    }

    // 创建新模型
    AtmosphereModel* model = new AtmosphereModel(
                wavelengths,                            // 太阳波长，单位nm
                solar_irradiance,                       // 太阳辐照度
                Atmosphere::kSunAngularRadius,          // 太阳角半径
                kBottomRadius,                          // 大气层底层到星球中心的距离(内半径)
                kTopRadius,                             // 大气层外层到星球中心的距离(外半径)
                {rayleigh_layer},                       // 大气空气分子密度分布
                rayleigh_scattering,                    // rayleigh散射系数
                {mie_layer},                            // 大气气溶胶密度分布
                mie_scattering,                         // (气溶胶)mie散射系数
                mie_extinction,                         // (气溶胶)mie消光系数
                kMiePhaseFunctionG,                     // 气溶胶Cornette-Shanks的相位函数参数值g
                ozone_density,                          // 大气中吸收光线的空气分子密度
                absorption_extinction,                  // 吸收光线的空气分子消光
                ground_albedo,                          // 地面的平均反照率
                max_sun_zenith_angle,                   // 太阳最大的天顶角,弧度制
                Atmosphere::kLengthUnitInMeters,        // 长度单位
                false,                                  // 是否把单次散射合并到一个纹理
                true);                                  // 半精度浮点数

    return model;
}

NS_RENDERSYSTEM_END
