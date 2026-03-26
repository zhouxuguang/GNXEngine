//
//  SSAOPass.cpp
//  GNXEngine
//
//  屏幕空间环境光遮蔽(SSAO)渲染Pass实现
//

#include "SSAOPass.h"
#include "ShaderAssetLoader.h"
#include "RenderParameter.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/MathUtil/include/RandomMath.h"
#include "Runtime/MathUtil/include/MathUtil.h"

USING_NS_MATHUTIL
USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

// SSAO参数结构体
struct cbSSAOParams
{
    simd_float4 samples[64];        // 采样核心
    simd_float4 noiseScale;         // 噪声缩放 (width/noiseSize, height/noiseSize, 0, 0)
    float radius;                   // 采样半径
    float bias;                     // 深度偏移
    int kernelSize;                 // 采样核心大小
    float padding;
};

SSAOPass::SSAOPass()
{
}

SSAOPass::~SSAOPass()
{
}

bool SSAOPass::Initialize(const SSAOConfig& config)
{
    if (mInitialized)
    {
        return true;
    }
    
    mConfig = config;
    
    // 生成采样核心
    GenerateSampleKernel();
    
    // 生成随机旋转纹理
    GenerateRandomRotationTexture();
    
    // 创建SSAO管线
    CreateSSAOPipeline();
    
    // 创建模糊管线
    if (mConfig.enableBlur)
    {
        CreateBlurPipeline();
    }
    
    // 创建SSAO参数UBO
    CreateSSAOUniformBuffers();
    
    mInitialized = true;
    return true;
}

//=============================================================================
// 生成SSAO采样核心
//=============================================================================

void SSAOPass::GenerateSampleKernel()
{
    mSampleKernel.clear();
    mSampleKernel.reserve(mConfig.kernelSize);
    
    for (int i = 0; i < mConfig.kernelSize; ++i)
    {
        // 在半球内生成采样点
        Vector3f sample = UniformHemisphere();
        
        // 随机缩放，使采样点更靠近中心
        float scale = ((float)(i * i)) / (mConfig.kernelSize * mConfig.kernelSize);
        scale = Mix(0.1f, 1.0f, scale * scale);
        
        sample = sample * scale;
        
        mSampleKernel.push_back(sample);
    }
}

//=============================================================================
// 生成随机旋转纹理
//=============================================================================

void SSAOPass::GenerateRandomRotationTexture()
{
    const int noiseSize = mConfig.noiseSize;
    const int noiseCount = noiseSize * noiseSize;
    
    // 创建随机旋转向量数据
    std::vector<float> noiseData;
    noiseData.reserve(noiseCount * 3);
    
    for (int i = 0; i < noiseCount; ++i)
    {
        // 绕Z轴的随机旋转向量
        Vector3f noise = UniformCircle();
        
        noiseData.push_back(noise.x);
        noiseData.push_back(noise.y);
        noiseData.push_back(noise.z);
        noiseData.push_back(0.0f);  // padding
    }
    
    // 创建纹理
    TextureDesc textureDesc;
    textureDesc.width = noiseSize;
    textureDesc.height = noiseSize;
    textureDesc.depth = 1;
    textureDesc.format = kTexFormatRGBA32Float;
    textureDesc.usage = TextureUsage::TextureUsageShaderRead;
    
    mNoiseTexture = RenderCore::GetRenderDevice()->CreateTexture2D(textureDesc.format, textureDesc.usage,
                                                        textureDesc.width, textureDesc.height, 1);
    
    // 创建噪声采样器（重复模式）
    SamplerDesc samplerDesc;
    samplerDesc.filterMin = MIN_NEAREST;
    samplerDesc.filterMag = MAG_NEAREST;
    samplerDesc.wrapS = REPEAT;
    samplerDesc.wrapT = REPEAT;
    mNoiseSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(samplerDesc);
}

void SSAOPass::CreateSSAOPipeline()
{
    // 创建SSAO着色器
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("SSAO");
    
    // 禁用深度测试
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    
    // 创建管线
    mSSAOPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    mSSAOPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
    
    // 创建纹理采样器
    SamplerDesc samplerDesc;
    samplerDesc.filterMin = MIN_NEAREST;
    samplerDesc.filterMag = MAG_NEAREST;
    samplerDesc.wrapS = CLAMP_TO_EDGE;
    samplerDesc.wrapT = CLAMP_TO_EDGE;
    mGBufferSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(samplerDesc);
}

void SSAOPass::CreateBlurPipeline()
{
    // 创建模糊着色器
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("SSAOBlur");
    
    // 禁用深度测试
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    
    // 创建管线
    mBlurPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    mBlurPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
}

void SSAOPass::CreateSSAOUniformBuffers()
{
    mSSAOParamsUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbSSAOParams));
}

SSAOOutput SSAOPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const SSAOParams& params)
{
    mWidth = params.width;
    mHeight = params.height;
    
    // 更新SSAO参数UBO
    if (mSSAOParamsUBO)
    {
        cbSSAOParams ssaoParams;
        memset(&ssaoParams, 0, sizeof(ssaoParams));
        
        // 复制采样核心
        for (int i = 0; i < mConfig.kernelSize && i < 64; ++i)
        {
            ssaoParams.samples[i] = mathutil::make_simd_float4(
                mSampleKernel[i].x, mSampleKernel[i].y, mSampleKernel[i].z, 0.0f);
        }
        
        // 噪声缩放
        ssaoParams.noiseScale = mathutil::make_simd_float4(
            (float)mWidth / (float)mConfig.noiseSize,
            (float)mHeight / (float)mConfig.noiseSize,
            0.0f, 0.0f);
        
        ssaoParams.radius = params.radius;
        ssaoParams.bias = params.bias;
        ssaoParams.kernelSize = params.kernelSize;
        
        mSSAOParamsUBO->SetData(&ssaoParams, 0, sizeof(ssaoParams));
    }
    
    // 定义SSAO Pass数据结构
    struct SSAOPassData
    {
        SSAOOutput output;
        
        FrameGraphResource gBufferA;
        FrameGraphResource depthTexture;
        
        UniformBufferPtr cameraUBO;
        UniformBufferPtr ssaoParamsUBO;
        RCTexturePtr noiseTexture;
    };
    
    // 第一阶段：SSAO计算
    auto& ssaoPassData = frameGraph.AddPass<SSAOPassData>(
        passName + "_SSAO",
        [=](FrameGraph::Builder& builder, SSAOPassData& data)
        {
            // 创建输出纹理（单通道）
            FrameGraphTexture::Desc outputDesc;
            outputDesc.SetName("SSAO_Result");
            outputDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
            outputDesc.depth = 1;
            outputDesc.format = RenderCore::kTexFormatR32Float;
            data.output.ssaoResult = builder.Create<FrameGraphTexture>(outputDesc.name, outputDesc);
            builder.Write(data.output.ssaoResult, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            
            // 读取G-Buffer和深度纹理
            data.gBufferA = builder.Read(params.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            
            data.cameraUBO = params.cameraUBO;
            data.ssaoParamsUBO = mSSAOParamsUBO;
            data.noiseTexture = mNoiseTexture;
        },
        [=](const SSAOPassData& data, FrameGraphPassResources& resources, void* context)
        {
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gBufferA);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.ssaoResult);
            
            float debugColor[4] = {0.5f, 0.5f, 1.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
            
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);
            
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = outputTexture.texture;
            colorAttachment->clearColor = {1.0f, 0.0f, 0.0f, 0.0f};
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(colorAttachment);
            
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
            renderEncoder->SetGraphicsPipeline(mSSAOPipeline);
            
            // 绑定UBO
            if (data.cameraUBO)
            {
                renderEncoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
                renderEncoder->SetFragmentUniformBuffer("cbPerCamera", data.cameraUBO);
            }
            if (data.ssaoParamsUBO)
            {
                renderEncoder->SetFragmentUniformBuffer("cbSSAOParams", data.ssaoParamsUBO);
            }
            
            // 绑定纹理
            renderEncoder->SetFragmentTextureAndSampler("gGBufferA", gBufferA.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("gDepth", depthTexture.texture, mGBufferSampler);
            renderEncoder->SetFragmentTextureAndSampler("texNoise", data.noiseTexture, mNoiseSampler);
            
            renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
            renderEncoder->EndEncode();
        }
    );
    
    // 如果启用模糊，添加模糊Pass
    if (params.enableBlur && mBlurPipeline)
    {
        struct BlurPassData
        {
            SSAOOutput output;
            FrameGraphResource ssaoInput;
            FrameGraphResource depthTexture;
        };
        
        auto& blurPassData = frameGraph.AddPass<BlurPassData>(
            passName + "_Blur",
            [=](FrameGraph::Builder& builder, BlurPassData& data)
            {
                // 创建模糊输出纹理
                FrameGraphTexture::Desc outputDesc;
                outputDesc.SetName("SSAO_BlurResult");
                outputDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
                outputDesc.depth = 1;
                outputDesc.format = RenderCore::kTexFormatR32Float;
                data.output.ssaoResult = builder.Create<FrameGraphTexture>(outputDesc.name, outputDesc);
                builder.Write(data.output.ssaoResult, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
                
                // 读取SSAO结果
                data.ssaoInput = builder.Read(ssaoPassData.output.ssaoResult, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
                // 读取深度纹理（用于双边模糊）
                data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            },
            [=](const BlurPassData& data, FrameGraphPassResources& resources, void* context)
            {
                FrameGraphTexture& ssaoInput = resources.Get<FrameGraphTexture>(data.ssaoInput);
                FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
                FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.ssaoResult);
                
                float debugColor[4] = {0.3f, 0.3f, 0.8f, 1.0f};
                SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
                
                RenderPass renderPass;
                renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);
                
                auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
                colorAttachment->texture = outputTexture.texture;
                colorAttachment->clearColor = {1.0f, 0.0f, 0.0f, 0.0f};
                colorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
                renderPass.colorAttachments.push_back(colorAttachment);
                
                RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
                renderEncoder->SetGraphicsPipeline(mBlurPipeline);
                
                // 绑定SSAO纹理
                renderEncoder->SetFragmentTextureAndSampler("texSSAO", ssaoInput.texture, mGBufferSampler);
                // 绑定深度纹理（用于双边模糊）
                renderEncoder->SetFragmentTextureAndSampler("gDepth", depthTexture.texture, mGBufferSampler);
                
                renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
                renderEncoder->EndEncode();
            }
        );
        
        return blurPassData.output;
    }
    
    return ssaoPassData.output;
}

NS_RENDERSYSTEM_END
