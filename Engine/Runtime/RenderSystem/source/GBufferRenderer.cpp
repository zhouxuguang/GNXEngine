//
//  GBufferRenderer.cpp
//  GNXEngine
//
//  G-Buffer渲染器实现
//

#include "GBufferRenderer.h"
#include "Material.h"
#include "SceneRenderer.h"
#include "SceneNode.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"

NS_RENDERSYSTEM_BEGIN

GBufferRenderer::GBufferRenderer()
{
}

GBufferRenderer::~GBufferRenderer()
{
}

bool GBufferRenderer::Initialize(uint32_t width, uint32_t height)
{
	mWidth = width;
	mHeight = height;

    if (mIsInitialized)
    {
        return true;
    }
    
    // 创建渲染管线
    CreateGBufferPipeline();
    mIsInitialized = true;
    return true;
}

void GBufferRenderer::Resize(uint32_t width, uint32_t height)
{
    if (mWidth == width && mHeight == height)
    {
        return;
    }
    
    mWidth = width;
    mHeight = height;
}

void GBufferRenderer::SetMaterial(std::shared_ptr<Material> material)
{
    mCurrentMaterial = material;
    
    if (material && material->GetGBufferPSO())
    {
        // 使用材质的G-Buffer PSO
        // TODO: 设置当前渲染管线
    }
}

GBufferData GBufferRenderer::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const GBufferRenderParams& params)
{
    // 定义 GBuffer Pass 数据结构
    struct GBufferPassData
    {
        GBufferData gbuffer;
        GBufferMeshData meshes;
        GBufferUniformData uniforms;
        mathutil::Frustumf frustum;
    };

    auto& passData = frameGraph.AddPass<GBufferPassData>(
        passName,
        [=](FrameGraph::Builder& builder, GBufferPassData& data)
        {
            // 创建 SceneColor 纹理 (HDR)
            FrameGraphTexture::Desc sceneColorDesc;
            sceneColorDesc.SetName("SceneColor");
            sceneColorDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            sceneColorDesc.depth = 1;
            sceneColorDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.gbuffer.sceneColor = builder.Create<FrameGraphTexture>(sceneColorDesc.name, sceneColorDesc);
            builder.Write(data.gbuffer.sceneColor, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferA
            FrameGraphTexture::Desc gBufferADesc;
            gBufferADesc.SetName("GBufferA");
            gBufferADesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferADesc.depth = 1;
            gBufferADesc.format = RenderCore::kTexR10G10B10A2;
            data.gbuffer.gBufferA = builder.Create<FrameGraphTexture>(gBufferADesc.name, gBufferADesc);
            builder.Write(data.gbuffer.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferB
            FrameGraphTexture::Desc gBufferBDesc;
            gBufferBDesc.SetName("GBufferB");
            gBufferBDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferBDesc.depth = 1;
            gBufferBDesc.format = RenderCore::kTexFormatRGBA8;
            data.gbuffer.gBufferB = builder.Create<FrameGraphTexture>(gBufferBDesc.name, gBufferBDesc);
            builder.Write(data.gbuffer.gBufferB, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferC
            FrameGraphTexture::Desc gBufferCDesc;
            gBufferCDesc.SetName("GBufferC");
            gBufferCDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferCDesc.depth = 1;
            gBufferCDesc.format = RenderCore::kTexFormatSRGB8_ALPHA8;
            data.gbuffer.gBufferC = builder.Create<FrameGraphTexture>(gBufferCDesc.name, gBufferCDesc);
            builder.Write(data.gbuffer.gBufferC, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferD (Motion Vector，需要浮点精度)
            FrameGraphTexture::Desc gBufferDDesc;
            gBufferDDesc.SetName("GBufferD");
            gBufferDDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferDDesc.depth = 1;
            gBufferDDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.gbuffer.gBufferD = builder.Create<FrameGraphTexture>(gBufferDDesc.name, gBufferDDesc);
            builder.Write(data.gbuffer.gBufferD, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 使用 PreDepth Pass 的深度图，读取并作为只读深度附件
            // 管线总是先执行 PreDepth Pass
            data.gbuffer.depthTexture = builder.Read(params.preDepthTexture, (uint32_t)RenderCore::ResourceAccessType::DepthStencilReadOnly);

            // 保存渲染参数
            data.meshes = std::move(params.meshes);
            data.uniforms = std::move(params.uniforms);
            data.frustum = params.frustum;
            
        },
        [=](const GBufferPassData& data, FrameGraphPassResources& resources, void* context)
        {
            // 获取纹理资源
            FrameGraphTexture& sceneColorTexture = resources.Get<FrameGraphTexture>(data.gbuffer.sceneColor);
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferA);
            FrameGraphTexture& gBufferB = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferB);
            FrameGraphTexture& gBufferC = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferC);
            FrameGraphTexture& gBufferD = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferD);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.gbuffer.depthTexture);

            float debugColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);

            // 创建 RenderPass（多渲染目标）
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);

            // 颜色附件 0: SceneColor
            auto sceneColorAttachment = std::make_shared<RenderPassColorAttachment>();
            sceneColorAttachment->texture = sceneColorTexture.texture;
            sceneColorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            sceneColorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            sceneColorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(sceneColorAttachment);

            // 颜色附件 1: GBufferA 
            auto gBufferAAttachment = std::make_shared<RenderPassColorAttachment>();
            gBufferAAttachment->texture = gBufferA.texture;
            gBufferAAttachment->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            gBufferAAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            gBufferAAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(gBufferAAttachment);

            // 颜色附件 2: GBufferB
            auto gBufferBAttachment = std::make_shared<RenderPassColorAttachment>();
            gBufferBAttachment->texture = gBufferB.texture;
            gBufferBAttachment->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            gBufferBAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            gBufferBAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(gBufferBAttachment);

            // 颜色附件 3: GBufferC 
            auto gBufferCAttachment = std::make_shared<RenderPassColorAttachment>();
            gBufferCAttachment->texture = gBufferC.texture;
            gBufferCAttachment->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            gBufferCAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            gBufferCAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(gBufferCAttachment);

            // 颜色附件 4: GBufferD
            auto gBufferDAttachment = std::make_shared<RenderPassColorAttachment>();
            gBufferDAttachment->texture = gBufferD.texture;
            gBufferDAttachment->clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            gBufferDAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            gBufferDAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(gBufferDAttachment);

            // 深度附件
            // 使用 PreDepth Pass 的深度图：读取已有深度，不再写入
            auto depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            depthAttachment->texture = depthTexture.texture;
            depthAttachment->loadOp = ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            depthAttachment->readOnly = true;  // 只读深度，用于深度测试
            renderPass.depthAttachment = depthAttachment;

            // 创建 RenderEncoder
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

            // 渲染静态网格
            if (!data.meshes.staticMeshes.empty() && mGBufferPipeline)
            {
                for (const auto& meshItem : data.meshes.staticMeshes)
                {
                    if (!meshItem.mesh || !meshItem.objectUBO)
                        continue;

                    RenderInfo renderInfo;
                    renderInfo.renderEncoder = renderEncoder;
                    renderInfo.cameraUBO = data.uniforms.cameraUBO;
                    renderInfo.objectUBO = meshItem.objectUBO;
                    renderInfo.materials = meshItem.materials;

                    MeshDrawUtil::DrawMeshBasePass(*meshItem.mesh, renderInfo, mGBufferPipeline);
                }
            }

            // 渲染地形（专属渲染路径 - GPU-driven）
            if (!data.meshes.terrainItems.empty() && mTerrainGBufferPipeline)
            {
                for (TerrainComponent* terrain : data.meshes.terrainItems)
                {
                    if (!terrain)
                        continue;

                    // 获取地形的object UBO
                    UniformBufferPtr objectUBO = nullptr;
                    SceneNode* terrainNode = terrain->GetSceneNode();
                    if (terrainNode)
                    {
                        objectUBO = terrainNode->GetOrCreateModelUBO(RenderCore::GetRenderDevice());
                    }

                    terrain->Render(renderEncoder.get(),
                                    data.uniforms.cameraUBO,
                                    objectUBO,
                                    mTerrainGBufferPipeline,
                                    &data.frustum);
                }
            }

            // 渲染蒙皮网格
            /*if (!data.meshes.skinnedMeshes.empty() && data.uniforms.skinnedMatrixUBO)
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

                    MeshDrawUtil::DrawSkinnedMesh(*meshItem.mesh, renderInfo, false);
                }
            }*/

            renderEncoder->EndEncode();
        }
    );

    return passData.gbuffer;
}

void GBufferRenderer::CreateGBufferPipeline()
{
	GraphicsShaderInfo shaderInfoDepth = CreateGraphicsShaderInfo("BasePass");
    shaderInfoDepth.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    
    // 这里必须带上有相等的操作符，因为在preZ阶段已经写入深度，现阶段只是比较深度而已，理论上深度是一样的才通过绘制
	shaderInfoDepth.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionEqual;
    
    // GBuffer 有5个颜色附件：SceneColor + GBufferA + GBufferB + GBufferC + GBufferD
    shaderInfoDepth.graphicsPipelineDesc.renderTargetCount = 5;
    
    mGBufferPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfoDepth.graphicsPipelineDesc);
    mGBufferPipeline->AttachGraphicsShader(shaderInfoDepth.graphicsShader);

    // Terrain-specific G-buffer PSO (VS reads SSBO + heightmap)
    GraphicsShaderInfo shaderInfoTerrain = CreateGraphicsShaderInfo("Terrain");
    shaderInfoTerrain.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    shaderInfoTerrain.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionEqual;
    shaderInfoTerrain.graphicsPipelineDesc.renderTargetCount = 5;
    mTerrainGBufferPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfoTerrain.graphicsPipelineDesc);
    mTerrainGBufferPipeline->AttachGraphicsShader(shaderInfoTerrain.graphicsShader);
}

NS_RENDERSYSTEM_END
