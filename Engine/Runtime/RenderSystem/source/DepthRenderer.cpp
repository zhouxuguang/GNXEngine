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
    shaderInfoDepth.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionLess;
    mDepthOnlyPipeline = mDevice->CreateGraphicsPipeline(shaderInfoDepth.graphicsPipelineDesc);
    mDepthOnlyPipeline->AttachGraphicsShader(shaderInfoDepth.graphicsShader);
    
    GraphicsShaderInfo shaderInfoSkinnedDepth = CreateGraphicsShaderInfo("SkinnedDepthGenerate");
    shaderInfoSkinnedDepth.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionLess;
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
// FrameGraph 渲染函数
//=============================================================================

FrameGraphResource DepthRenderer::Render(
    const std::string& passName,
    FrameGraph& frameGraph,
    const std::vector<MeshPtr>& meshes,
    UniformBufferPtr cameraUBO,
    UniformBufferPtr objectUBO)
{
    // 定义 Pass 数据结构
    struct DepthPassData
    {
        FrameGraphResource depthTexture;
        std::vector<MeshPtr> meshList;
        UniformBufferPtr cameraUBO;
        UniformBufferPtr objectUBO;
    };
    
    // 添加深度渲染 Pass
    auto& passData = frameGraph.AddPass<DepthPassData>(
        passName,
        [&](FrameGraph::Builder& builder, DepthPassData& data)
        {
            // 创建深度纹理
            FrameGraphTexture::Desc depthDesc;
            depthDesc.extent = RenderCore::Rect2D{0, 0, (int)mConfig.width, (int)mConfig.height};
            depthDesc.depth = 1;
            depthDesc.format = mConfig.depthFormat;
            
            data.depthTexture = builder.Create<FrameGraphTexture>("DepthTexture", depthDesc);
            builder.Write(data.depthTexture);
            
            // 保存引用
            data.meshList = meshes;
            data.cameraUBO = cameraUBO;
            data.objectUBO = objectUBO;
        },
        [](const DepthPassData& data, FrameGraphPassResources& resources, void* context) 
        {
            // 获取 RenderEncoder（从 context 中获取）
            // 注意：这里需要根据实际的 FrameGraph 实现调整
            // RenderEncoderPtr renderEncoder = static_cast<RenderEncoderPtr>(context);
            // if (!renderEncoder) return;
            
            // 设置深度渲染管线
            // renderEncoder->SetGraphicsPipeline(depthOnlyPipeline);
            
            // 设置 UBO
            // renderEncoder->SetVertexUniformBuffer("cbPerCamera", data.cameraUBO);
            // renderEncoder->SetVertexUniformBuffer("cbPerObject", data.objectUBO);
            
            // 渲染所有静态网格
            // RenderInfo renderInfo;
            // renderInfo.renderEncoder = renderEncoder;
            // renderInfo.cameraUBO = data.cameraUBO;
            // renderInfo.objectUBO = data.objectUBO;
            
            // for (const auto& mesh : data.meshList)
            // {
            //     MeshDrawUtil::DrawMeshDepthOnly(*mesh, renderInfo, depthOnlyPipeline);
            // }
        }
    );
    
    // 保存资源 ID
    mDepthResource = passData.depthTexture;
    
    return mDepthResource;
}

FrameGraphResource DepthRenderer::Render(
    const std::string& passName,
    FrameGraph& frameGraph,
    const std::vector<MeshPtr>& meshes,
    const std::vector<SkinnedMeshPtr>& skinnedMeshes,
    UniformBufferPtr cameraUBO,
    UniformBufferPtr objectUBO,
    UniformBufferPtr skinnedMatrixUBO)
{
    // 定义 Pass 数据结构
    struct DepthPassData
    {
        FrameGraphResource depthTexture;
        std::vector<MeshPtr> meshList;
        std::vector<SkinnedMeshPtr> skinnedMeshList;
        UniformBufferPtr cameraUBO;
        UniformBufferPtr objectUBO;
        UniformBufferPtr skinnedMatrixUBO;
    };
    
    // 添加深度渲染 Pass
    auto& passData = frameGraph.AddPass<DepthPassData>(
        passName,
        [&](FrameGraph::Builder& builder, DepthPassData& data)
        {
            // 创建深度纹理
            FrameGraphTexture::Desc depthDesc;
            depthDesc.extent = RenderCore::Rect2D{0, 0, (int)mConfig.width, (int)mConfig.height};
            depthDesc.depth = 1;
            depthDesc.format = mConfig.depthFormat;
            
            data.depthTexture = builder.Create<FrameGraphTexture>("DepthTexture", depthDesc);
            builder.Write(data.depthTexture);
            
            // 保存引用
            data.meshList = meshes;
            data.skinnedMeshList = skinnedMeshes;
            data.cameraUBO = cameraUBO;
            data.objectUBO = objectUBO;
            data.skinnedMatrixUBO = skinnedMatrixUBO;
        },
        [=](const DepthPassData& data, FrameGraphPassResources& resources, void* context)
        {
            // 获取 RenderEncoder（从 context 中获取）
            RenderEncoderPtr renderEncoder = nullptr;
             if (!renderEncoder) return;
            
             // 渲染静态网格
             RenderInfo renderInfo;
             renderInfo.renderEncoder = renderEncoder;
             renderInfo.cameraUBO = data.cameraUBO;
             renderInfo.objectUBO = data.objectUBO;
            
             for (const auto& mesh : data.meshList)
             {
                 MeshDrawUtil::DrawMeshDepthOnly(*mesh, renderInfo, mDepthOnlyPipeline);
             }
            
             // 渲染蒙皮网格
             if (data.skinnedMatrixUBO)
             {
                 renderInfo.skinnedMatrixUBO = data.skinnedMatrixUBO;
                 
                 for (const auto& mesh : data.skinnedMeshList)
                 {
                     MeshDrawUtil::DrawSkinnedMeshDepthOnly(*mesh, renderInfo, mSkinnedDepthOnlyPipeline);
                 }
             }
        }
    );
    
    // 保存资源 ID
    mDepthResource = passData.depthTexture;
    
    return mDepthResource;
}

NS_RENDERSYSTEM_END
