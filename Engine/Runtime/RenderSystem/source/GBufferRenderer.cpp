//
//  GBufferRenderer.cpp
//  GNXEngine
//
//  G-Buffer渲染器实现
//

#include "GBufferRenderer.h"
#include "Material.h"
#include "SceneRenderer.h"
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
    };

    auto& passData = frameGraph.AddPass<GBufferPassData>(
        passName,
        [=](FrameGraph::Builder& builder, GBufferPassData& data)
        {
            // 创建 SceneColor 纹理 (HDR)
            FrameGraphTexture::Desc sceneColorDesc;
            sceneColorDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            sceneColorDesc.depth = 1;
            sceneColorDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.gbuffer.sceneColor = builder.Create<FrameGraphTexture>("SceneColor", sceneColorDesc);
            builder.Write(data.gbuffer.sceneColor, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferA
            FrameGraphTexture::Desc gBufferADesc;
            gBufferADesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferADesc.depth = 1;
            gBufferADesc.format = RenderCore::kTexFormatRGBA8;
            data.gbuffer.gBufferA = builder.Create<FrameGraphTexture>("GBufferA", gBufferADesc);
            builder.Write(data.gbuffer.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferB
            FrameGraphTexture::Desc gBufferBDesc;
            gBufferBDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferBDesc.depth = 1;
            gBufferBDesc.format = RenderCore::kTexFormatRGBA8;
            data.gbuffer.gBufferB = builder.Create<FrameGraphTexture>("GBufferB", gBufferBDesc);
            builder.Write(data.gbuffer.gBufferB, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 创建 GBufferC
            FrameGraphTexture::Desc gBufferCDesc;
            gBufferCDesc.extent = RenderCore::Rect2D{0, 0, (int)mWidth, (int)mHeight};
            gBufferCDesc.depth = 1;
            gBufferCDesc.format = RenderCore::kTexFormatSRGB8_ALPHA8;
            data.gbuffer.gBufferC = builder.Create<FrameGraphTexture>("GBufferC", gBufferCDesc);
            builder.Write(data.gbuffer.gBufferC, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);

            // 保存渲染参数
            data.meshes = std::move(params.meshes);
            data.uniforms = std::move(params.uniforms);
            
        },
        [=](const GBufferPassData& data, FrameGraphPassResources& resources, void* context)
        {
            // 获取纹理资源
            FrameGraphTexture& sceneColorTexture = resources.Get<FrameGraphTexture>(data.gbuffer.sceneColor);
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferA);
            FrameGraphTexture& gBufferB = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferB);
            FrameGraphTexture& gBufferC = resources.Get<FrameGraphTexture>(data.gbuffer.gBufferC);

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
            gBufferBAttachment->clearColor = {0.5f, 0.5f, 0.0f, 0.0f}; // 法线默认朝向
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

                    // 使用材质的 G-Buffer PSO 或默认管线
                    if (mCurrentMaterial && mCurrentMaterial->GetGBufferPSO())
                    {
                        MeshDrawUtil::DrawMesh(*meshItem.mesh, renderInfo);
                    }
                    else
                    {
                        MeshDrawUtil::DrawMesh(*meshItem.mesh, renderInfo);
                    }
                }
            }

            // 渲染蒙皮网格
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

                    MeshDrawUtil::DrawSkinnedMesh(*meshItem.mesh, renderInfo, false);
                }
            }

            renderEncoder->EndEncode();
        }
    );

    return passData.gbuffer;
}

void GBufferRenderer::CreateGBufferPipeline()
{
    // 加载G-Buffer shader
    // 这里可以加载不同材质类型的G-Buffer shader变体
    
    // 示例：
    /*
    ShaderAssetString shaderAsset = LoadShaderAsset("GBufferPBR");
    GraphicsPipelineDescriptor pipelineDesc;
    
    // 设置多渲染目标
    pipelineDesc.colorAttachmentCount = mConfig.enablePositionTexture ? 4 : 3;
    
    // 设置深度测试
    pipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;
    pipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    
    // 创建PSO
    mGBufferPipeline = GetRenderDevice()->CreateGraphicsPipeline(pipelineDesc);
    */
}

NS_RENDERSYSTEM_END
