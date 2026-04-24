//
//  MotionBlurPass.cpp
//  GNXEngine
//
//  Motion Blur Post-Processing Pass Implementation
//

#include "MotionBlurPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

MotionBlurPass::MotionBlurPass()
{
}

MotionBlurPass::~MotionBlurPass()
{
}

bool MotionBlurPass::Initialize(const MotionBlurConfig& config)
{
    if (mInitialized)
    {
        return true;
    }
    
    mConfig = config;
    
    CreateMotionBlurPipeline();
    
    mInitialized = true;
    return true;
}

void MotionBlurPass::CreateMotionBlurPipeline()
{
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("MotionBlur");
    
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    
    mMotionBlurPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    mMotionBlurPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
    
    SamplerDesc colorSamplerDesc;
    colorSamplerDesc.filterMin = MIN_LINEAR;
    colorSamplerDesc.filterMag = MAG_LINEAR;
    colorSamplerDesc.wrapS = CLAMP_TO_EDGE;
    colorSamplerDesc.wrapT = CLAMP_TO_EDGE;
    mColorSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(colorSamplerDesc);
    
    SamplerDesc depthSamplerDesc;
    depthSamplerDesc.filterMin = MIN_NEAREST;
    depthSamplerDesc.filterMag = MAG_NEAREST;
    depthSamplerDesc.wrapS = CLAMP_TO_EDGE;
    depthSamplerDesc.wrapT = CLAMP_TO_EDGE;
    mDepthSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(depthSamplerDesc);
}

MotionBlurOutput MotionBlurPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const MotionBlurParams& params)
{
    mWidth = params.width;
    mHeight = params.height;
    
    struct MotionBlurPassData
    {
        MotionBlurOutput output;
        
        FrameGraphResource colorTexture;
        FrameGraphResource depthTexture;
        
        UniformBufferPtr cameraUBO;
    };
    
    auto& passData = frameGraph.AddPass<MotionBlurPassData>(
        passName,
        [=](FrameGraph::Builder& builder, MotionBlurPassData& data)
        {
            FrameGraphTexture::Desc outputDesc;
            outputDesc.SetName("MotionBlur_Result");
            outputDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
            outputDesc.depth = 1;
            outputDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.output.result = builder.Create<FrameGraphTexture>(outputDesc.name, outputDesc);
            builder.Write(data.output.result, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            
            data.colorTexture = builder.Read(params.colorTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            
            data.cameraUBO = params.cameraUBO;
        },
        [=](const MotionBlurPassData& data, FrameGraphPassResources& resources, void* context)
        {
            FrameGraphTexture& colorTexture = resources.Get<FrameGraphTexture>(data.colorTexture);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.result);
            
            float debugColor[4] = {0.6f, 0.4f, 0.2f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
            
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);
            
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = outputTexture.texture;
            colorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(colorAttachment);
            
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
            renderEncoder->SetGraphicsPipeline(mMotionBlurPipeline);
            
            if (data.cameraUBO)
            {
                renderEncoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
                renderEncoder->SetFragmentUniformBuffer("cbPerCamera", data.cameraUBO);
            }
            
            renderEncoder->SetFragmentTextureAndSampler("gColorTexture", colorTexture.texture, mColorSampler);
            renderEncoder->SetFragmentTextureAndSampler("gDepthTexture", depthTexture.texture, mDepthSampler);
            
            renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            renderEncoder->EndEncode();
        }
    );
    
    return passData.output;
}

NS_RENDERSYSTEM_END
