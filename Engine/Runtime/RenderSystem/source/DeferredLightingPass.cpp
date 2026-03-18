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
        lightData.Strength = mathutil::make_simd_float3(strength.x, strength.y, strength.z);
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
        lightData.Strength = mathutil::make_simd_float3(strength.x, strength.y, strength.z);
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
        lightData.Strength = mathutil::make_simd_float3(strength.x, strength.y, strength.z);
        lightData.FalloffStart = light->getFalloffStart();
        lightData.FalloffEnd = light->getFalloffEnd();
        lightData.SpotPower = light->getSpotPower();
    }
    
    // 更新到UBO
    mLightDataUBO->SetData(&lightData, 0, sizeof(lightData));
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
        
        // Uniform Buffers
        UniformBufferPtr cameraUBO;
        UniformBufferPtr lightUBO;
        
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
            builder.Write(data.output.lightingResult, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            
            // 读取G-Buffer纹理
            data.gBufferA = builder.Read(params.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferB = builder.Read(params.gBufferB, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferC = builder.Read(params.gBufferC, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferD = builder.Read(params.gBufferD, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            // 深度纹理作为只读深度附件使用
            data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::DepthStencilReadOnly);
            
            // 保存Uniform Buffers
            data.cameraUBO = params.cameraUBO;
            data.lightUBO = mLightDataUBO;
            
            // 保存IBL参数
            data.enableIBL = params.enableIBL;
            data.irradianceMap = params.irradianceMap;
            data.prefilteredMap = params.prefilteredMap;
            data.brdfLUT = params.brdfLUT;
        },
        [=](const LightingPassData& data, FrameGraphPassResources& resources, void* context)
        {
            // 更新光源数据
            UpdateLightData(params);
            
            // 获取纹理资源
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gBufferA);
            FrameGraphTexture& gBufferB = resources.Get<FrameGraphTexture>(data.gBufferB);
            FrameGraphTexture& gBufferC = resources.Get<FrameGraphTexture>(data.gBufferC);
            FrameGraphTexture& gBufferD = resources.Get<FrameGraphTexture>(data.gBufferD);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.lightingResult);
            
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
            
            // 绑定G-Buffer纹理
            renderEncoder->SetFragmentTextureAndSampler("gGBufferA", gBufferA.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferB", gBufferB.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferC", gBufferC.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gGBufferD", gBufferD.texture, mGBufferSampler);
            // gDepthTexture: Position（从深度重建，这里绑定深度纹理）
            renderEncoder->SetFragmentTextureAndSampler("gDepth", depthTexture.texture, mGBufferSampler);
            
            // 绑定IBL纹理（如果启用）
            if (data.enableIBL)
            {
                if (data.irradianceMap)
                {
                    renderEncoder->SetFragmentTextureAndSampler("texEnvMapIrradiance", data.irradianceMap, mGBufferSampler);
                }
                if (data.prefilteredMap)
                {
                    renderEncoder->SetFragmentTextureAndSampler("texEnvMap", data.prefilteredMap, mGBufferSampler);
                }
                if (data.brdfLUT)
                {
                    renderEncoder->SetFragmentTextureAndSampler("texBRDF_LUT", data.brdfLUT, mGBufferSampler);
                }
            }
            
            // 绘制全屏三角形（3个顶点）
            // Shader中使用SV_VertexID生成顶点，不需要顶点缓冲区
            renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
            
            renderEncoder->EndEncode();
        }
    );
    
    return passData.output;
}

NS_RENDERSYSTEM_END
