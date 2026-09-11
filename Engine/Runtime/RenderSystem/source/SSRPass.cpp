//
//  SSRPass.cpp
//  GNXEngine
//

#include "SSRPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include "Runtime/MathUtil/include/SimdMath.h"
#include <tracy/Tracy.hpp>

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

namespace
{
struct cbSSRParams
{
    mathutil::simd_float4 traceParams; // max distance, step length, thickness, max steps
    mathutil::simd_float4 fadeParams;  // edge fade, max roughness, intensity, binary steps
};
}

bool SSRPass::Initialize(const SSRConfig& config)
{
    if (mInitialized)
        return true;

    mConfig = config;
    CreatePipelines();
    mParamsUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbSSRParams));
    mInitialized = mTracePipeline && mCompositePipeline && mParamsUBO;
    return mInitialized;
}

void SSRPass::CreatePipelines()
{
    GraphicsShaderInfo traceInfo = CreateGraphicsShaderInfo("SSRTrace");
    traceInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    mTracePipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(traceInfo.graphicsPipelineDesc);
    mTracePipeline->AttachGraphicsShader(traceInfo.graphicsShader);

    GraphicsShaderInfo compositeInfo = CreateGraphicsShaderInfo("SSRComposite");
    compositeInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    mCompositePipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(compositeInfo.graphicsPipelineDesc);
    mCompositePipeline->AttachGraphicsShader(compositeInfo.graphicsShader);

    SamplerDesc pointDesc;
    pointDesc.filterMin = MIN_NEAREST;
    pointDesc.filterMag = MAG_NEAREST;
    pointDesc.wrapS = CLAMP_TO_EDGE;
    pointDesc.wrapT = CLAMP_TO_EDGE;
    mPointSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(pointDesc);

    SamplerDesc linearDesc;
    linearDesc.filterMin = MIN_LINEAR;
    linearDesc.filterMag = MAG_LINEAR;
    linearDesc.wrapS = CLAMP_TO_EDGE;
    linearDesc.wrapT = CLAMP_TO_EDGE;
    mLinearSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(linearDesc);
}

SSROutput SSRPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const SSRParams& params)
{
    mWidth = params.width;
    mHeight = params.height;

    cbSSRParams shaderParams;
    shaderParams.traceParams = mathutil::make_simd_float4(
        mConfig.maxRayDistance, mConfig.stepLength, mConfig.thickness,
        static_cast<float>(mConfig.maxSteps));
    shaderParams.fadeParams = mathutil::make_simd_float4(
        mConfig.edgeFade, mConfig.maxRoughness, mConfig.intensity,
        static_cast<float>(mConfig.binarySearchSteps));
    mParamsUBO->SetData(&shaderParams, 0, sizeof(shaderParams));

    struct TraceData
    {
        FrameGraphResource reflection = -1;
        FrameGraphResource sceneColor = -1;
        FrameGraphResource gBufferA = -1;
        FrameGraphResource gBufferB = -1;
        FrameGraphResource depth = -1;
        UniformBufferPtr cameraUBO;
        UniformBufferPtr paramsUBO;
    };

    auto& trace = frameGraph.AddPass<TraceData>(
        passName + "_Trace",
        [=](FrameGraph::Builder& builder, TraceData& data)
        {
            FrameGraphTexture::Desc desc;
            desc.SetName("SSR_Reflection");
            desc.extent = Rect2D{0, 0, static_cast<int>(params.width), static_cast<int>(params.height)};
            desc.depth = 1;
            desc.format = kTexFormatRGBA16Float;
            data.reflection = builder.Create<FrameGraphTexture>(desc.name, desc);
            builder.Write(data.reflection, static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
            data.sceneColor = builder.Read(params.sceneColor, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.gBufferA = builder.Read(params.gBufferA, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.gBufferB = builder.Read(params.gBufferB, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.depth = builder.Read(params.depthTexture, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.cameraUBO = params.cameraUBO;
            data.paramsUBO = mParamsUBO;
        },
        [=](const TraceData& data, FrameGraphPassResources& resources, void*)
        {
            ZoneScopedN("SSRTracePass");
            auto& reflection = resources.Get<FrameGraphTexture>(data.reflection);
            auto& sceneColor = resources.Get<FrameGraphTexture>(data.sceneColor);
            auto& gBufferA = resources.Get<FrameGraphTexture>(data.gBufferA);
            auto& gBufferB = resources.Get<FrameGraphTexture>(data.gBufferB);
            auto& depth = resources.Get<FrameGraphTexture>(data.depth);

            float marker[4] = {0.1f, 0.65f, 0.9f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), marker);

            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, static_cast<int>(mWidth), static_cast<int>(mHeight));
            auto color = std::make_shared<RenderPassColorAttachment>();
            color->texture = reflection.texture;
            color->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            color->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            color->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(color);

            auto encoder = commandBuffer->CreateRenderEncoder(renderPass);
            encoder->SetGraphicsPipeline(mTracePipeline);
            encoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
            encoder->SetFragmentUniformBuffer("cbPerCamera", data.cameraUBO);
            encoder->SetFragmentUniformBuffer("cbSSRParams", data.paramsUBO);
            encoder->SetFragmentTextureAndSampler("gSceneColor", sceneColor.texture, mLinearSampler);
            encoder->SetFragmentTextureAndSampler("gGBufferA", gBufferA.texture, mPointSampler);
            encoder->SetFragmentTextureAndSampler("gGBufferB", gBufferB.texture, mPointSampler);
            encoder->SetFragmentTextureAndSampler("gDepth", depth.texture, mPointSampler);
            encoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            encoder->EndEncode();
        });

    struct CompositeData
    {
        SSROutput output;
        FrameGraphResource sceneColor = -1;
        FrameGraphResource reflection = -1;
        FrameGraphResource gBufferA = -1;
        FrameGraphResource gBufferB = -1;
        FrameGraphResource gBufferC = -1;
        FrameGraphResource depth = -1;
        UniformBufferPtr cameraUBO;
        UniformBufferPtr paramsUBO;
    };

    auto& composite = frameGraph.AddPass<CompositeData>(
        passName + "_Composite",
        [=](FrameGraph::Builder& builder, CompositeData& data)
        {
            FrameGraphTexture::Desc desc;
            desc.SetName("SSR_Composite");
            desc.extent = Rect2D{0, 0, static_cast<int>(params.width), static_cast<int>(params.height)};
            desc.depth = 1;
            desc.format = kTexFormatRGBA16Float;
            data.output.result = builder.Create<FrameGraphTexture>(desc.name, desc);
            builder.Write(data.output.result, static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
            data.sceneColor = builder.Read(params.sceneColor, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.reflection = builder.Read(trace.reflection, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.gBufferA = builder.Read(params.gBufferA, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.gBufferB = builder.Read(params.gBufferB, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.gBufferC = builder.Read(params.gBufferC, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.depth = builder.Read(params.depthTexture, static_cast<uint32_t>(ResourceAccessType::ShaderRead));
            data.cameraUBO = params.cameraUBO;
            data.paramsUBO = mParamsUBO;
        },
        [=](const CompositeData& data, FrameGraphPassResources& resources, void*)
        {
            ZoneScopedN("SSRCompositePass");
            auto& output = resources.Get<FrameGraphTexture>(data.output.result);
            auto& scene = resources.Get<FrameGraphTexture>(data.sceneColor);
            auto& reflection = resources.Get<FrameGraphTexture>(data.reflection);
            auto& a = resources.Get<FrameGraphTexture>(data.gBufferA);
            auto& b = resources.Get<FrameGraphTexture>(data.gBufferB);
            auto& c = resources.Get<FrameGraphTexture>(data.gBufferC);
            auto& depth = resources.Get<FrameGraphTexture>(data.depth);

            float marker[4] = {0.15f, 0.8f, 0.85f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), marker);

            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, static_cast<int>(mWidth), static_cast<int>(mHeight));
            auto color = std::make_shared<RenderPassColorAttachment>();
            color->texture = output.texture;
            color->loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
            color->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(color);

            auto encoder = commandBuffer->CreateRenderEncoder(renderPass);
            encoder->SetGraphicsPipeline(mCompositePipeline);
            encoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
            encoder->SetFragmentUniformBuffer("cbPerCamera", data.cameraUBO);
            encoder->SetFragmentUniformBuffer("cbSSRParams", data.paramsUBO);
            encoder->SetFragmentTextureAndSampler("gSceneColor", scene.texture, mLinearSampler);
            encoder->SetFragmentTextureAndSampler("gReflection", reflection.texture, mLinearSampler);
            encoder->SetFragmentTextureAndSampler("gGBufferA", a.texture, mPointSampler);
            encoder->SetFragmentTextureAndSampler("gGBufferB", b.texture, mPointSampler);
            encoder->SetFragmentTextureAndSampler("gGBufferC", c.texture, mPointSampler);
            encoder->SetFragmentTextureAndSampler("gDepth", depth.texture, mPointSampler);
            encoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
            encoder->EndEncode();
        });

    return composite.output;
}

NS_RENDERSYSTEM_END
