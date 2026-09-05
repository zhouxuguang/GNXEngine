//
//  DeferredLightingPass.cpp
//  GNXEngine
//
//  延迟光照渲染Pass实现
//

#include "DeferredLightingPass.h"
#include "ShaderAssetLoader.h"
#include "RenderParameter.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include <cmath>
#include <tracy/Tracy.hpp>

USING_NS_MATHUTIL
USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// 构造/析构函数
//=============================================================================

DeferredLightingPass::DeferredLightingPass()
{
}

DeferredLightingPass::~DeferredLightingPass()
{
}

//=============================================================================
// 初始化/关闭
//=============================================================================

bool DeferredLightingPass::Initialize(const DeferredLightingConfig& config)
{
    if (mInitialized)
    {
        return true;
    }
    
    mConfig = config;
    
    // 创建延迟光照管线
    CreateLightingPipeline();
    
    // 创建光源UBO
    CreateLightUniformBuffers();

    // 创建阴影UBO
    CreateShadowUniformBuffer();
    
    mInitialized = true;
    return true;
}

//=============================================================================
// 创建渲染管线
//=============================================================================

void DeferredLightingPass::CreateLightingPipeline()
{
    // 创建延迟光照着色器
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("DeferredLighting");
    
    // 配置深度测试（只读深度）
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    
    // 创建管线
    mLightingPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    mLightingPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
    
    // 创建纹理采样器
    SamplerDesc samplerDesc;
    samplerDesc.filterMin = MIN_LINEAR;
    samplerDesc.filterMag = MAG_LINEAR;
    samplerDesc.wrapS = CLAMP_TO_EDGE;
    samplerDesc.wrapT = CLAMP_TO_EDGE;
    mGBufferSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(samplerDesc);

    // IBL cubemap 采样器：mip trilinear + 允许 mip 范围，供预过滤环境贴图 roughness LOD 采样
    SamplerDesc iblSamplerDesc;
    iblSamplerDesc.filterMin = MIN_LINEAR;
    iblSamplerDesc.filterMag = MAG_LINEAR;
    iblSamplerDesc.filterMip = MIN_LINEAR_MIPMAP_LINEAR;
    iblSamplerDesc.wrapS = CLAMP_TO_EDGE;
    iblSamplerDesc.wrapT = CLAMP_TO_EDGE;
    iblSamplerDesc.wrapR = CLAMP_TO_EDGE;
    iblSamplerDesc.maxLod = 12;
    mIBLCubeSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(iblSamplerDesc);

    // ShadowMap 采样器：PCSS 需要手动读取深度并做比较，因此使用非比较的最近点采样
    // 越界采样在 shader 端手动判定为"无阴影"，因此 wrap 用 CLAMP_TO_EDGE 即可
    SamplerDesc shadowSamplerDesc;
    shadowSamplerDesc.filterMin = MIN_NEAREST;
    shadowSamplerDesc.filterMag = MAG_NEAREST;
    shadowSamplerDesc.filterMip = MIN_NEAREST_MIPMAP_NEAREST;
    shadowSamplerDesc.wrapS = CLAMP_TO_EDGE;
    shadowSamplerDesc.wrapT = CLAMP_TO_EDGE;
    shadowSamplerDesc.wrapR = CLAMP_TO_EDGE;
    mShadowSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(shadowSamplerDesc);
}

//=============================================================================
// 创建光源UBO
//=============================================================================

void DeferredLightingPass::CreateLightUniformBuffers()
{
    // 创建光源数据UBO - 使用引擎统一的cbLighting结构
    mLightDataUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbLighting));
}

//=============================================================================
// 更新光源数据
//=============================================================================

void DeferredLightingPass::UpdateLightData(const DeferredLightingParams& params)
{
    if (!mLightDataUBO)
    {
        return;
    }
    
    // 使用引擎统一的cbLighting结构传递主光源数据
    cbLighting lightData;
    memset(&lightData, 0, sizeof(lightData));
    
    // 优先使用第一个方向光
    if (!params.directionalLights.empty() && params.directionalLights[0])
    {
        DirectionLight* light = params.directionalLights[0];
        Vector3f dir = light->getDirection();
        // 方向光：w=0表示方向
        lightData.WorldSpaceLightPos = mathutil::make_simd_float4(dir.x, dir.y, dir.z, 0.0f);
        
        Vector3f color = light->getColor();
        Vector3f strength = light->getStrength();
        lightData.LightColor = mathutil::make_simd_float4(color.x, color.y, color.z, 1.0f);
        lightData.Strength = mathutil::make_simd_float4(strength.x, strength.y, strength.z, 0.0f);
        lightData.FalloffStart = light->getFalloffStart();
        lightData.FalloffEnd = light->getFalloffEnd();
        lightData.SpotPower = 0.0f;
    }
    // 如果没有方向光，使用第一个点光源
    else if (!params.pointLights.empty() && params.pointLights[0])
    {
        PointLight* light = params.pointLights[0];
        Vector3f pos = light->getPosition();
        // 点光源：w=1表示位置
        lightData.WorldSpaceLightPos = mathutil::make_simd_float4(pos.x, pos.y, pos.z, 1.0f);
        
        Vector3f color = light->getColor();
        Vector3f strength = light->getStrength();
        lightData.LightColor = mathutil::make_simd_float4(color.x, color.y, color.z, 1.0f);
        lightData.Strength = mathutil::make_simd_float4(strength.x, strength.y, strength.z, 0.0f);
        lightData.FalloffStart = light->getFalloffStart();
        lightData.FalloffEnd = light->getFalloffEnd();
        lightData.SpotPower = 0.0f;
    }
    // 如果没有点光源，使用第一个聚光灯
    else if (!params.spotLights.empty() && params.spotLights[0])
    {
        SpotLight* light = params.spotLights[0];
        Vector3f pos = light->getPosition();
        lightData.WorldSpaceLightPos = mathutil::make_simd_float4(pos.x, pos.y, pos.z, 1.0f);
        
        Vector3f color = light->getColor();
        Vector3f strength = light->getStrength();
        lightData.LightColor = mathutil::make_simd_float4(color.x, color.y, color.z, 1.0f);
        lightData.Strength = mathutil::make_simd_float4(strength.x, strength.y, strength.z, 0.0f);
        lightData.FalloffStart = light->getFalloffStart();
        lightData.FalloffEnd = light->getFalloffEnd();
        lightData.SpotPower = light->getSpotPower();
    }
    
    // 更新到UBO
    mLightDataUBO->SetData(&lightData, 0, sizeof(lightData));
}

//=============================================================================
// 创建阴影UBO
//=============================================================================

void DeferredLightingPass::CreateShadowUniformBuffer()
{
    // 创建阴影数据UBO - 使用引擎统一的cbShadow结构
    mShadowUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbShadow));
}

//=============================================================================
// 更新阴影数据
//=============================================================================

void DeferredLightingPass::UpdateShadowData(const DeferredLightingParams& params)
{
    if (!mShadowUBO)
    {
        return;
    }

    cbShadow shadowData;
    memset(&shadowData, 0, sizeof(shadowData));

    if (params.enableShadow)
    {
        shadowData.LightView = params.shadowLightView;
        shadowData.LightProj = params.shadowLightProj;
        shadowData.ShadowMapSize = params.shadowMapSize;
        shadowData.ShadowParams  = params.shadowParams;
        shadowData.ShadowFlags   = mathutil::make_simd_float4(1.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        shadowData.LightView = mathutil::Matrix4x4f::ZERO;
        shadowData.LightProj = mathutil::Matrix4x4f::ZERO;
        shadowData.ShadowMapSize = mathutil::make_simd_float4(0.0f, 0.0f, 0.0f, 0.0f);
        shadowData.ShadowParams  = mathutil::make_simd_float4(0.0f, 0.0f, 0.0f, 0.0f);
        shadowData.ShadowFlags   = mathutil::make_simd_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    mShadowUBO->SetData(&shadowData, 0, sizeof(shadowData));
}

//=============================================================================
// 添加到FrameGraph
//=============================================================================

DeferredLightingOutput DeferredLightingPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const DeferredLightingParams& params)
{
    mWidth = params.width;
    mHeight = params.height;
    
    // 定义Pass数据结构
    struct LightingPassData
    {
        DeferredLightingOutput output;
        
        // 输入纹理
        FrameGraphResource gSceneColor;
        FrameGraphResource gBufferA;
        FrameGraphResource gBufferB;
        FrameGraphResource gBufferC;
        FrameGraphResource gBufferD;
        FrameGraphResource depthTexture;
        
        // SSAO纹理
        FrameGraphResource ssaoTexture;
        bool hasSSAO;
        
        // ShadowMap（FrameGraph 资源）
        FrameGraphResource shadowMap;
        bool enableShadow;

        // Uniform Buffers
        UniformBufferPtr cameraUBO;
        UniformBufferPtr lightUBO;
        UniformBufferPtr shadowUBO;
        
        // IBL资源
        bool enableIBL;
        RCTexturePtr irradianceMap;
        RCTexturePtr prefilteredMap;
        RCTexturePtr brdfLUT;
    };
    
    auto& passData = frameGraph.AddPass<LightingPassData>(
        passName,
        [=](FrameGraph::Builder& builder, LightingPassData& data)
        {
            // 创建输出纹理（HDR格式）
            FrameGraphTexture::Desc outputDesc;
            outputDesc.SetName("SceneColor");
            outputDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
            outputDesc.depth = 1;
            outputDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.output.lightingResult = builder.Create<FrameGraphTexture>(outputDesc.name, outputDesc);
            (void)builder.Write(data.output.lightingResult, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            
            // 读取G-Buffer纹理（包含RT0 SceneColor/Emissive）
            data.gSceneColor = builder.Read(params.gSceneColor, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferA = builder.Read(params.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferB = builder.Read(params.gBufferB, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferC = builder.Read(params.gBufferC, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferD = builder.Read(params.gBufferD, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            // 深度纹理作为只读深度附件使用
            data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::DepthStencilReadOnly);
            
            // SSAO纹理（如果可用）
            data.hasSSAO = (params.ssaoTexture != -1);
            if (data.hasSSAO)
            {
                data.ssaoTexture = builder.Read(params.ssaoTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            }

            // ShadowMap 深度纹理（作为只读采样）
            data.enableShadow = params.enableShadow && (params.shadowMap != -1);
            if (data.enableShadow)
            {
                data.shadowMap = builder.Read(params.shadowMap, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            }
            else
            {
                data.shadowMap = -1;
            }
            
            // 保存Uniform Buffers
            data.cameraUBO = params.cameraUBO;
            data.lightUBO = mLightDataUBO;
            data.shadowUBO = mShadowUBO;
            
            // 保存IBL参数
            data.enableIBL = params.enableIBL;
            data.irradianceMap = params.irradianceMap;
            data.prefilteredMap = params.prefilteredMap;
            data.brdfLUT = params.brdfLUT;
        },
        [=](const LightingPassData& data, FrameGraphPassResources& resources, void* context)
        {
            ZoneScopedN("DeferredLightingPass");
            // 更新光源数据
            UpdateLightData(params);

            // 更新阴影数据
            UpdateShadowData(params);
            
            // 获取纹理资源
            FrameGraphTexture& gSceneColor = resources.Get<FrameGraphTexture>(data.gSceneColor);
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gBufferA);
            FrameGraphTexture& gBufferB = resources.Get<FrameGraphTexture>(data.gBufferB);
            FrameGraphTexture& gBufferC = resources.Get<FrameGraphTexture>(data.gBufferC);
            FrameGraphTexture& gBufferD = resources.Get<FrameGraphTexture>(data.gBufferD);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.lightingResult);
            
            // SSAO纹理（如果可用）
            FrameGraphTexture* ssaoTexture = nullptr;
            if (data.hasSSAO)
            {
                ssaoTexture = &resources.Get<FrameGraphTexture>(data.ssaoTexture);
            }
            
            float debugColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
            
            // 创建RenderPass
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);
            
            // 颜色附件
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = outputTexture.texture;
            colorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(colorAttachment);
            
            // 深度附件（从G-Buffer Pass读取，只读）
            // storeOp 使用 STORE 以保留深度值供后续使用
            auto depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            depthAttachment->texture = depthTexture.texture;
            depthAttachment->loadOp = ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            depthAttachment->readOnly = true;  // 只读深度，用于深度测试
            renderPass.depthAttachment = depthAttachment;
            
            // 创建RenderEncoder
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
            
            // 绑定渲染管线
            renderEncoder->SetGraphicsPipeline(mLightingPipeline);
            
            // 绑定相机UBO - 使用名称绑定匹配cbPerCamera
            if (data.cameraUBO)
            {
                renderEncoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
                renderEncoder->SetFragmentUniformBuffer("cbPerCamera", data.cameraUBO);
            }
            
            // 绑定光源UBO - 使用名称绑定匹配cbLighting
            if (data.lightUBO)
            {
                renderEncoder->SetFragmentUniformBuffer("cbLighting", data.lightUBO);
            }

            // 绑定阴影UBO - 使用名称绑定匹配cbShadow
            // 无论是否启用阴影都始终绑定：UpdateShadowData 在未启用时写入 flags=0 的
            // 全零数据，shader 中 _ShadowFlags.x<=0.5 不会进入 PCSS 分支。
            // （若只在启用时绑定，未绑定的 cbuffer 在 Metal/Vulkan 上是未初始化内存，
            //   _ShadowFlags.x 可能读到垃圾值 >0.5 而错误触发 PCSS。）
            if (data.shadowUBO)
            {
                renderEncoder->SetFragmentUniformBuffer("cbShadow", data.shadowUBO);
            }
            
            // 绑定G-Buffer纹理（RT0: Emissive from BasePass）
            renderEncoder->SetFragmentTextureAndSampler("gGBufferSceneColor", gSceneColor.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferA", gBufferA.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferB", gBufferB.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferC", gBufferC.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferD", gBufferD.texture, mGBufferSampler);
            // gDepthTexture: Position（从深度重建，这里绑定深度纹理）
            renderEncoder->SetFragmentTextureAndSampler("gDepth", depthTexture.texture, mGBufferSampler);
            
            // 绑定SSAO纹理（如果可用）
            if (ssaoTexture)
            {
                renderEncoder->SetFragmentTextureAndSampler("gSSAO", ssaoTexture->texture, mGBufferSampler);
            }

            // 绑定ShadowMap纹理（如果启用）
            if (data.enableShadow)
            {
                FrameGraphTexture& shadowMapTex = resources.Get<FrameGraphTexture>(data.shadowMap);
                if (shadowMapTex.texture)
                {
                    renderEncoder->SetFragmentTextureAndSampler("gShadowMap", shadowMapTex.texture,
                                                                mShadowSampler ? mShadowSampler : mGBufferSampler);
                }
            }
            
            // 绑定IBL纹理（如果启用）
            if (data.enableIBL)
            {
                if (data.irradianceMap)
                {
                    renderEncoder->SetFragmentTextureAndSampler("texEnvMapIrradiance", data.irradianceMap, mGBufferSampler);
                }
                if (data.prefilteredMap)
                {
                    // 预过滤环境贴图需要按 roughness 采样不同 mip，使用带 mip trilinear 的采样器
                    renderEncoder->SetFragmentTextureAndSampler("texEnvMap", data.prefilteredMap,
                                                                mIBLCubeSampler ? mIBLCubeSampler : mGBufferSampler);
                }
                if (data.brdfLUT)
                {
                    renderEncoder->SetFragmentTextureAndSampler("texBRDF_LUT", data.brdfLUT, mGBufferSampler);
                }
            }
            
            // 绘制全屏三角形（3个顶点）
            // Shader中使用SV_VertexID生成顶点，不需要顶点缓冲区
            renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            
            renderEncoder->EndEncode();
        }
    );
    
    return passData.output;
}

NS_RENDERSYSTEM_END
