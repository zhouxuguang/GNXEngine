//
//  DepthRenderer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/11.
//

#include "DepthRenderer.h"
#include <cmath>
#include <algorithm>
#include <limits>

#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

DepthRenderer::DepthRenderer(RenderDevice* device) : mDevice(device)
{
    GraphicsShaderInfo shaderInfoDepth = CreateGraphicsShaderInfo("DepthGenerate");
    shaderInfoDepth.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    mDepthOnlyPipeline = mDevice->CreateGraphicsPipeline(shaderInfoDepth.graphicsPipelineDesc);
    mDepthOnlyPipeline->AttachGraphicsShader(shaderInfoDepth.graphicsShader);
    
    GraphicsShaderInfo shaderInfoSkinnedDepth = CreateGraphicsShaderInfo("SkinnedDepthGenerate");
    shaderInfoSkinnedDepth.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    mSkinnedDepthOnlyPipeline = mDevice->CreateGraphicsPipeline(shaderInfoSkinnedDepth.graphicsPipelineDesc);
    mSkinnedDepthOnlyPipeline->AttachGraphicsShader(shaderInfoSkinnedDepth.graphicsShader);
}

DepthRenderer::~DepthRenderer()
{
    // 智能指针会自动释放资源
}

bool DepthRenderer::Initialize(const DepthRenderConfig& config)
{
    if (!mDevice)
    {
        return false;
    }

    mConfig = config;
    mInitialized = true;
    return true;
}

void DepthRenderer::Shutdown()
{
    mDepthOnlyPipeline = nullptr;
    mSkinnedDepthOnlyPipeline = nullptr;
    mInitialized = false;
}

void DepthRenderer::UpdateConfig(const DepthRenderConfig& config)
{
    if (config.width != mConfig.width || config.height != mConfig.height ||
        config.depthFormat != mConfig.depthFormat)
    {
        // 需要重新创建纹理
        Shutdown();
        Initialize(config);
    }
}

const DepthRenderConfig& DepthRenderer::GetConfig() const
{
    return mConfig;
}

//=============================================================================
// FrameGraph 渲染函数（推荐接口 - 使用结构化参数）
//=============================================================================

FrameGraphResource DepthRenderer::Render(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const DepthRenderParams& params)
{
    // 定义 Pass 数据结构
    struct DepthPassData
    {
        FrameGraphResource depthTexture;
        DepthMeshData meshes;
        DepthUniformData uniforms;
    };

    // 添加深度渲染 Pass
    auto& passData = frameGraph.AddPass<DepthPassData>(
        passName,
        [=](FrameGraph::Builder& builder, DepthPassData& data)
        {
            // 创建深度纹理
            FrameGraphTexture::Desc depthDesc;
            depthDesc.SetName("DepthTexture");
            depthDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
            depthDesc.depth = 1;
            depthDesc.format = mConfig.depthFormat;

            data.depthTexture = builder.Create<FrameGraphTexture>(depthDesc.name, depthDesc);
            builder.Write(data.depthTexture, (uint32_t)RenderCore::ResourceAccessType::DepthStencilAttachment);

            // 保存引用
            data.meshes = std::move(params.meshes);
            data.uniforms = std::move(params.uniforms);
        },
        [=](const DepthPassData& data, FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphTexture &depthTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.depthTexture);
            
            RenderPass renderPass;
            
            renderPass.renderRegion = Rect2D(0, 0, (int)params.width, (int)params.height);
            
            renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            renderPass.depthAttachment->texture = depthTexture.texture;
            renderPass.depthAttachment->clearDepth = DepthConfig::GetDefaultClearDepth();
            renderPass.depthAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            
            float color[4] = {1.0, 0.0, 0.0, 1.0};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

            // 渲染静态网格，每个mesh使用自己的objectUBO
            if (!data.meshes.staticMeshes.empty())
            {
                for (const auto& meshItem : data.meshes.staticMeshes)
                {
                    if (!meshItem.mesh || !meshItem.objectUBO)
                        continue;

                    RenderInfo renderInfo;
                    renderInfo.renderEncoder = renderEncoder;
                    renderInfo.cameraUBO = data.uniforms.cameraUBO;
                    renderInfo.objectUBO = meshItem.objectUBO;

                    MeshDrawUtil::DrawMeshDepthOnly(*meshItem.mesh, renderInfo, mDepthOnlyPipeline);
                }
            }

            // 渲染蒙皮网格，每个mesh使用自己的objectUBO
            if (!data.meshes.skinnedMeshes.empty() && data.uniforms.skinnedMatrixUBO)
            {
                for (const auto& meshItem : data.meshes.skinnedMeshes)
                {
                    if (!meshItem.mesh || !meshItem.objectUBO)
                        continue;

                    RenderInfo renderInfo;
                    renderInfo.renderEncoder = renderEncoder;
                    renderInfo.cameraUBO = data.uniforms.cameraUBO;
                    renderInfo.objectUBO = meshItem.objectUBO;
                    renderInfo.skinnedMatrixUBO = data.uniforms.skinnedMatrixUBO;

                    MeshDrawUtil::DrawSkinnedMeshDepthOnly(*meshItem.mesh, renderInfo, mSkinnedDepthOnlyPipeline);
                }
            }
            renderEncoder->EndEncode();
        }
    );

    // 保存资源 ID
    mDepthResource = passData.depthTexture;

    return mDepthResource;
}

NS_RENDERSYSTEM_END
