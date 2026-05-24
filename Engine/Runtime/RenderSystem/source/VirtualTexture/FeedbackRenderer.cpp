//
//  FeedbackRenderer.cpp
//  GNXEngine
//

#include "VirtualTexture/FeedbackRenderer.h"
#include "ShaderAssetLoader.h"
#include "SceneNode.h"

NS_RENDERSYSTEM_BEGIN

FeedbackRenderer::FeedbackRenderer(RenderDevicePtr device)
    : mDevice(device)
{
}

FeedbackRenderer::~FeedbackRenderer()
{
    Shutdown();
}

bool FeedbackRenderer::Initialize()
{
    if (mInitialized)
        return true;

    // 创建反馈渲染 PSO — PS 输出 R32Uint，开启深度写入
    {
        GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("vt/Feedback");
        shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
        shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;
        mFeedbackPipeline = mDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
        if (!mFeedbackPipeline)
            return false;
        mFeedbackPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
    }

    // 地形反馈渲染 PSO（使用 Terrain 的专用 VS — 内部读取 heightmap + SSBO）
    {
//        GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("vt/TerrainFeedback");
//        shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
//        shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
//        mTerrainFeedbackPipeline = mDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
//        if (mTerrainFeedbackPipeline)
//            mTerrainFeedbackPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
    }

    mInitialized = true;
    return true;
}

void FeedbackRenderer::Shutdown()
{
    mFeedbackPipeline = nullptr;
    mTerrainFeedbackPipeline = nullptr;
    mInitialized = false;
}

FrameGraphResource FeedbackRenderer::Render(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const FeedbackRenderParams& params,
    RCTexturePtr externalFeedbackTexture,
    RCTexturePtr externalFeedbackDepth)
{
    // 先 Import 外部 feedback RT + depth 到 FrameGraph
    FrameGraphTexture::Desc feedbackDesc;
    feedbackDesc.SetName("VtFeedback");
    feedbackDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
    feedbackDesc.depth = 1;
    feedbackDesc.format = kTexFormatR32Uint;

    FrameGraphTexture externalFGTex;
    externalFGTex.texture = externalFeedbackTexture;
    FrameGraphResource feedbackResource = frameGraph.Import<FrameGraphTexture>(
        "VtFeedback", feedbackDesc, std::move(externalFGTex));

    FrameGraphTexture::Desc depthDesc;
    depthDesc.SetName("VtFeedbackDepth");
    depthDesc.extent = RenderCore::Rect2D{0, 0, (int)params.width, (int)params.height};
    depthDesc.depth = 1;
    depthDesc.format = kTexFormatDepth16;

    FrameGraphTexture externalDepthTex;
    externalDepthTex.texture = externalFeedbackDepth;
    FrameGraphResource depthResource = frameGraph.Import<FrameGraphTexture>(
        "VtFeedbackDepth", depthDesc, std::move(externalDepthTex));

    struct FeedbackPassData
    {
        FrameGraphResource feedbackTexture;
        FrameGraphResource depthTexture;
        std::vector<FeedbackMeshItem> staticMeshes;
        std::vector<TerrainComponent*> terrainItems;
        UniformBufferPtr cameraUBO;
    };

    auto& passData = frameGraph.AddPass<FeedbackPassData>(
        passName,
        [=](FrameGraph::Builder& builder, FeedbackPassData& data)
        {
            data.feedbackTexture = builder.Write(feedbackResource, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            data.depthTexture = builder.Write(depthResource, (uint32_t)RenderCore::ResourceAccessType::DepthStencilAttachment);
            builder.SetSideEffect();

            // 保存引用
            data.staticMeshes = params.staticMeshes;
            data.terrainItems = params.terrainItems;
            data.cameraUBO = params.cameraUBO;
        },
        [=](const FeedbackPassData& data, FrameGraphPassResources& resources, void* context)
        {
            FrameGraphTexture& feedbackTexture = resources.Get<FrameGraphTexture>(data.feedbackTexture);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);

            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)params.width, (int)params.height);

            // 颜色附件：R32Uint feedback target
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = feedbackTexture.texture;
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            colorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            renderPass.colorAttachments.push_back(colorAttachment);

            // 深度附件：反馈 depth target，开启深度测试
            auto depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            depthAttachment->texture = depthTexture.texture;
            depthAttachment->clearDepth = DepthConfig::GetDefaultClearDepth();
            depthAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.depthAttachment = depthAttachment;

            float color[4] = {0.0f, 0.8f, 0.2f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

            // 渲染静态 VT 网格
            if (!data.staticMeshes.empty() && mFeedbackPipeline)
            {
                for (const auto& meshItem : data.staticMeshes)
                {
                    if (!meshItem.mesh || !meshItem.objectUBO)
                        continue;

                    RenderInfo renderInfo;
                    renderInfo.renderEncoder = renderEncoder;
                    renderInfo.cameraUBO = data.cameraUBO;
                    renderInfo.objectUBO = meshItem.objectUBO;

                    MeshDrawUtil::DrawMeshFeedback(*meshItem.mesh, renderInfo, mFeedbackPipeline);
                }
            }

            // 渲染地形
            if (!data.terrainItems.empty())
            {
                for (TerrainComponent* terrain : data.terrainItems)
                {
                    if (!terrain || !terrain->IsInitialized())
                        continue;

                    UniformBufferPtr objectUBO = nullptr;
                    SceneNode* terrainNode = terrain->GetSceneNode();
                    if (terrainNode)
                        objectUBO = terrainNode->GetOrCreateModelUBO(RenderCore::GetRenderDevice());

                    if (mTerrainFeedbackPipeline)
                    {
                        // terrain 的 feedback 渲染走自定义路径
                        // TODO: terrain->RenderFeedback(...)
                        // 暂时 fallback 到普通 feedback pipeline
                        if (mFeedbackPipeline)
                        {
                            terrain->RenderDepthOnly(renderEncoder.get(), data.cameraUBO, objectUBO, mFeedbackPipeline, nullptr);
                        }
                    }
                    else if (mFeedbackPipeline)
                    {
                        terrain->RenderDepthOnly(renderEncoder.get(), data.cameraUBO, objectUBO, mFeedbackPipeline, nullptr);
                    }
                }
            }

            renderEncoder->EndEncode();
        }
    );

    return passData.feedbackTexture;
}

NS_RENDERSYSTEM_END
