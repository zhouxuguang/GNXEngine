//
//  DeferredSceneRenderer.cpp
//  GNXEngine
//
//  延迟渲染场景渲染器实现
//

#include "DeferredSceneRenderer.h"
#include "SceneManager.h"
#include "SceneNode.h"
#include "mesh/MeshRenderer.h"
#include "skinnedMesh/SkinnedMeshRenderer.h"
#include "terrain/TerrainComponent.h"
#include "SkyBoxNode.h"
#include "RenderParameter.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "FrameGraph/FrameGraphExecuteContext.h"
#include "BuildSetting.h"
#include <algorithm>
#include <functional>

NS_RENDERSYSTEM_BEGIN

DeferredSceneRenderer::DeferredSceneRenderer()
    : SceneRenderer()
{
    mGBufferRenderer = std::make_shared<GBufferRenderer>();
    mDepthRender = std::make_unique<DepthRenderer>(GetRenderDevice().get());
    mDeferredLightingPass = std::make_shared<DeferredLightingPass>();
    mHiZPass = std::make_shared<HiZPass>();
    mSSAOPass = std::make_shared<SSAOPass>();
    mMotionBlurPass = std::make_shared<MotionBlurPass>();
    
    mPostProcessing = new PostProcessing(GetRenderDevice());
}

DeferredSceneRenderer::~DeferredSceneRenderer()
{
}

void DeferredSceneRenderer::SetGBufferConfig(const GBufferRenderer::GBufferConfig& config)
{
    mGBufferRenderer->SetConfig(config);
}

void DeferredSceneRenderer::SetIBLTextures(RCTexturePtr irradianceMap,
                                           RCTexturePtr prefilteredMap,
                                           RCTexturePtr brdfLUT)
{
    mIBLIrradianceMap = irradianceMap;
    mIBLPrefilteredMap = prefilteredMap;
    mIBLBRDFLUT = brdfLUT;
}

void DeferredSceneRenderer::SetMotionBlurEnabled(bool enabled)
{
    mEnableMotionBlur = enabled;
}

bool DeferredSceneRenderer::IsMotionBlurEnabled() const
{
    return mEnableMotionBlur;
}

const GBufferRenderer::GBufferConfig& DeferredSceneRenderer::GetGBufferConfig() const
{
    return mGBufferRenderer->GetConfig();
}

void DeferredSceneRenderer::Render(SceneManager *sceneManager, float deltaTime)
{
    if (!sceneManager)
    {
        return;
    }
    
    UpdateCameraView(sceneManager);

    CommandQueuePtr graphicsQueue = RenderCore::GetRenderDevice()->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }

    // 创建FrameGraph执行上下文，包含命令缓冲区
    FrameGraphExecuteContext executeContext;
    executeContext.commandBuffer = commandBuffer;
    // Vulkan后端会利用commandBuffer自动处理layout转换

    // 创建FrameGraph
    FrameGraph frameGraph;

    // ========== 统一收集场景数据（只收集一次）==========
    std::vector<DepthMeshItem> meshItems;
    std::vector<DepthSkinnedMeshItem> skinnedMeshItems;
    std::vector<TerrainComponent*> terrainItems;

    SceneNode* rootNode = sceneManager->GetRootNode();
    if (rootNode)
    {
        CollectMeshesRecursive(rootNode, meshItems, skinnedMeshItems, terrainItems);
    }

    // 获取相机UBO
    RenderInfo renderInfo = sceneManager->GetRenderInfo();
    UniformBufferPtr cameraUBO = renderInfo.cameraUBO;

    // 计算视锥体（用于地形逐patch剔除）
    // 引擎使用Vulkan/Metal后端，VP矩阵的NDC z范围始终为[0,1]
    // （Reverse-Z投影本身输出[0,1]；标准投影经mAdjust重映射后也是[0,1]）
    mathutil::Frustumf frustum;
    CameraPtr camera = sceneManager->GetCamera("MainCamera");
    if (camera)
    {
        mathutil::Matrix4x4f vp = camera->GetProjectionMatrix() * camera->GetViewMatrix();
        frustum.InitFrustum(vp, true);  // ndcZeroToOne = true
    }

    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // ========== 执行渲染 Pass ==========

    // Terrain GPU Culling（FrameGraph Compute Pass，必须在 PreDepth + BasePass 之前）
    // 注册到 FrameGraph 后，CS 在 Execute 阶段执行，输出 IndirectArgs 供后续 Pass 消费
    // 注意：Mesh Shader 地形不需要此步骤（TS 内部自己做视锥体剔除）
    if (!terrainItems.empty() && camera)
    {
        mathutil::Matrix4x4f vp = camera->GetProjectionMatrix() * camera->GetViewMatrix();
        for (TerrainComponent* terrain : terrainItems)
        {
            if (terrain && terrain->IsInitialized()
                && !terrain->IsUsingMeshShader()       // MS 跳过：TS 自带剔除
                && terrain->IsUsingGPUCulling())
            {
                terrain->DispatchCullViaFrameGraph(frameGraph, commandBuffer, vp, cameraUBO);
            }
        }
    }

    // PreZPass
    FrameGraphResource depthResource = RenderPreDepthPass(
        frameGraph, commandBuffer, meshItems, skinnedMeshItems, cameraUBO, terrainItems, frustum);

    // Hi-Z Pass（在PreDepth之后、BasePass之前）
    HiZOutput hiZOutput;
    if (depthResource != -1)
    {
        hiZOutput = BuildHiZPass(frameGraph, commandBuffer, depthResource);
        mLastHiZOutput = hiZOutput;
    }

    // BasePass (G-Buffer)
    GBufferData gbufferData = RenderBasePass(
        frameGraph, commandBuffer, meshItems, skinnedMeshItems, cameraUBO, depthResource, terrainItems, frustum);

    // SSAO Pass（在G-Buffer之后、DeferredLighting之前）
    SSAOOutput ssaoOutput;
    ssaoOutput.ssaoResult = -1;
    if (mSSAOPass && depthResource != -1 && gbufferData.gBufferA != -1)
    {
        if (!mSSAOPass->IsInitialized())
        {
            SSAOConfig ssaoConfig;
            ssaoConfig.kernelSize = 64;
            ssaoConfig.noiseSize = 4;
            ssaoConfig.radius = 0.5f;
            ssaoConfig.bias = 0.025f;
            ssaoConfig.enableBlur = true;
            ssaoConfig.blurRadius = 2;
            mSSAOPass->Initialize(ssaoConfig);
        }

        SSAOParams ssaoParams;
        ssaoParams.width = mWidth;
        ssaoParams.height = mHeight;
        ssaoParams.gBufferA = gbufferData.gBufferA;
        ssaoParams.depthTexture = depthResource;
        ssaoParams.cameraUBO = cameraUBO;
        ssaoParams.radius = 0.5f;
        ssaoParams.bias = 0.025f;
        ssaoParams.kernelSize = 64;
        ssaoParams.enableBlur = true;
        ssaoParams.blurRadius = 2;

        ssaoOutput = mSSAOPass->AddToFrameGraph("SSAOPass", frameGraph, commandBuffer, ssaoParams);
    }

    // Deferred Lighting Pass
    FrameGraphResource lightingResult = RenderDeferredLightingPass(
        frameGraph, commandBuffer, gbufferData, depthResource, cameraUBO, hiZOutput, ssaoOutput);

    // Skybox Pass（在 Deferred Lighting 之后、MotionBlur 之前）
    // 天空盒通过深度测试 LEQUAL 只填充远平面区域（无几何体覆盖的部分）
    FrameGraphResource skyboxResult = lightingResult;
    {
        SkyBoxNode* skyBoxNode = sceneManager->GetSkyBox();
        if (skyBoxNode)
        {
            skyboxResult = RenderSkyboxPass(frameGraph, commandBuffer, lightingResult, depthResource, cameraUBO, skyBoxNode);
        }
    }

    // Motion Blur Pass（在 Skybox 之后、Present之前）
    FrameGraphResource finalResult = skyboxResult;
    if (mEnableMotionBlur && mMotionBlurPass && depthResource != -1)
    {
        if (!mMotionBlurPass->IsInitialized())
        {
            MotionBlurConfig motionBlurConfig;
            mMotionBlurPass->Initialize(motionBlurConfig);
        }

        MotionBlurParams motionBlurParams;
        motionBlurParams.width = mWidth;
        motionBlurParams.height = mHeight;
        motionBlurParams.colorTexture = skyboxResult;
        motionBlurParams.depthTexture = depthResource;
        motionBlurParams.cameraUBO = cameraUBO;

        MotionBlurOutput motionBlurOutput = mMotionBlurPass->AddToFrameGraph(
            "MotionBlurPass", frameGraph, commandBuffer, motionBlurParams);
        finalResult = motionBlurOutput.result;
    }

    RenderPresentPass(frameGraph, commandBuffer, finalResult);

    frameGraph.Compile();
    // 执行FrameGraph，RHI层会自动处理资源状态转换
    frameGraph.Execute(&executeContext, mTransientResources);

    return;

    // 阶段1: G-Buffer Pass

    // 阶段2: 延迟光照Pass

    // 阶段3: 渲染天空盒

    // 阶段4: 前向渲染Pass（半透明物体）
    RenderForwardPass();
}

void DeferredSceneRenderer::UpdateCameraView(SceneManager *sceneManager)
{
    uint32_t width = 1;
    uint32_t height = 1;
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (cameraPtr)
    {
        Vector2i viewSize = cameraPtr->GetViewSize();
        width = viewSize.x;
        height = viewSize.y;
    }
    mWidth = width;
    mHeight = height;
}

FrameGraphResource DeferredSceneRenderer::RenderPreDepthPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const std::vector<DepthMeshItem>& meshItems,
    const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
    UniformBufferPtr cameraUBO,
    const std::vector<TerrainComponent*>& terrainItems,
    const mathutil::Frustumf& frustum)
{
    FrameGraphResource depthResource = -1;

    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // 构建 DepthRenderParams
    DepthRenderParams params;
    params.width = mWidth;
    params.height = mHeight;
    params.meshes.staticMeshes = meshItems;
    params.meshes.skinnedMeshes = skinnedMeshItems;
    params.meshes.terrainItems = terrainItems;
    params.uniforms.cameraUBO = cameraUBO;
    params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
    params.frustum = frustum;

    // 使用FrameGraph渲染深度图
    depthResource = mDepthRender->Render("DepthPass", frameGraph, commandBuffer, params);

    return depthResource;
}

GBufferData DeferredSceneRenderer::RenderBasePass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const std::vector<DepthMeshItem>& meshItems,
    const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
    UniformBufferPtr cameraUBO,
    FrameGraphResource preDepthTexture,
    const std::vector<TerrainComponent*>& terrainItems,
    const mathutil::Frustumf& frustum)
{
    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // 构建 GBuffer 渲染参数
    GBufferRenderParams params;
    params.meshes.staticMeshes = meshItems;
    params.meshes.skinnedMeshes = skinnedMeshItems;
    params.meshes.terrainItems = terrainItems;
    params.uniforms.cameraUBO = cameraUBO;
    params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
    params.preDepthTexture = preDepthTexture;  // 传递 PreDepth 深度图
    params.frustum = frustum;

    // 初始化 GBufferRenderer（如果需要）
    if (!mGBufferRenderer->IsInitialized())
    {
        mGBufferRenderer->Initialize(mWidth, mHeight);
    }

    // 调用 GBufferRenderer::AddToFrameGraph
    return mGBufferRenderer->AddToFrameGraph("BasePass", frameGraph, commandBuffer, params);
}

HiZOutput DeferredSceneRenderer::BuildHiZPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    FrameGraphResource depthTexture)
{
    // 初始化Hi-Z Pass（如果需要）
    if (!mHiZPass->IsInitialized())
    {
        mHiZPass->Initialize();
    }

    // 构建Hi-Z参数
    HiZParams params;
    params.width = mWidth;
    params.height = mHeight;
    params.depthTexture = depthTexture;
    params.useReverseZ = BuildSetting::mUseReverseZ;

    // 添加Hi-Z Pass到FrameGraph
    return mHiZPass->AddToFrameGraph("HiZPass", frameGraph, commandBuffer, params);
}

void DeferredSceneRenderer::RenderPresentPass(FrameGraph& frameGraph, CommandBufferPtr commandBuffer, FrameGraphResource depthResource)
{
    frameGraph.AddPass("PresentPass",
    [=](RenderSystem::FrameGraph::Builder &builder, RenderSystem::FrameGraph::NoData &data)
    {
        builder.Read(depthResource, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);

        // present的pass必须设置这个标记，要不然不会执行
        builder.SetSideEffect();
    },
    [=](const RenderSystem::FrameGraph::NoData &data, RenderSystem::FrameGraphPassResources &resources, void *)
    {
        RenderSystem::FrameGraphTexture &colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(depthResource);
        
        float color[4] = {1.0, 0.0, 0.0, 1.0};
        SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        
        mPostProcessing->SetRenderTexture(colorTexture.texture);
        mPostProcessing->Process(renderEncoder);
        
        renderEncoder->EndEncode();
        commandBuffer->PresentFrameBuffer();
    });
}

FrameGraphResource DeferredSceneRenderer::RenderDeferredLightingPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const GBufferData& gbufferData,
    FrameGraphResource depthTexture,
    UniformBufferPtr cameraUBO,
    const HiZOutput& hiZOutput,
    const SSAOOutput& ssaoOutput)
{
    // 初始化延迟光照Pass（如果需要）
    if (!mDeferredLightingPass->IsInitialized())
    {
        DeferredLightingConfig config;
        config.enableSSAO = (ssaoOutput.ssaoResult != -1);
        config.enableSSR = false;
        config.enableTiledLighting = false;
        mDeferredLightingPass->Initialize(config);
    }

    // 收集场景中的光源
    std::vector<DirectionLight*> directionalLights;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    CollectLights(directionalLights, pointLights, spotLights);

    // 构建延迟光照参数
    DeferredLightingParams params;
    params.gSceneColor = gbufferData.sceneColor;
    params.gBufferA = gbufferData.gBufferA;
    params.gBufferB = gbufferData.gBufferB;
    params.gBufferC = gbufferData.gBufferC;
    params.gBufferD = gbufferData.gBufferD;
    params.depthTexture = depthTexture;
    params.directionalLights = directionalLights;
    params.pointLights = pointLights;
    params.spotLights = spotLights;
    params.cameraUBO = cameraUBO;
    params.width = mWidth;
    params.height = mHeight;
    params.ssaoTexture = ssaoOutput.ssaoResult;
    
    // 传递 IBL 资源到延迟光照 Pass
    if (mIBLBRDFLUT || mIBLIrradianceMap || mIBLPrefilteredMap)
    {
        params.enableIBL = true;
        params.brdfLUT = mIBLBRDFLUT;
        params.irradianceMap = mIBLIrradianceMap;
        params.prefilteredMap = mIBLPrefilteredMap;
    }
    
    // 传递Hi-Z资源（如果可用）
    // 注意：这里需要在DeferredLightingParams中添加hiZTexture字段
    // 现在先预留接口，等实现SSR时使用
    // params.hiZTexture = hiZOutput.hiZTexture;

    // 执行延迟光照Pass
    DeferredLightingOutput output = mDeferredLightingPass->AddToFrameGraph(
        "DeferredLightingPass", frameGraph, commandBuffer, params);

    return output.lightingResult;
}

FrameGraphResource DeferredSceneRenderer::RenderSkyboxPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    FrameGraphResource colorTexture,
    FrameGraphResource depthTexture,
    UniformBufferPtr cameraUBO,
    SkyBoxNode* skyBoxNode)
{
    // 定义 Skybox Pass 数据结构
    struct SkyboxPassData
    {
        FrameGraphResource outputResult;
        FrameGraphResource inputColor;
        FrameGraphResource inputDepth;
        UniformBufferPtr cameraUBO;
    };

    auto& passData = frameGraph.AddPass<SkyboxPassData>(
        "Skybox_Pass",
        [=](FrameGraph::Builder& builder, SkyboxPassData& data)
        {
            // 创建输出纹理（与输入颜色相同规格）
            FrameGraphTexture::Desc outputDesc;
            outputDesc.SetName("Skybox_Output");
            outputDesc.extent = Rect2D{0, 0, (int)mWidth, (int)mHeight};
            outputDesc.depth = 1;
            outputDesc.format = kTexFormatBGRA32;
            //data.inputColor = builder.Read(colorTexture, (uint32_t)ResourceAccessType::ShaderRead);
            data.outputResult = builder.Write(colorTexture, (uint32_t)ResourceAccessType::ColorAttachment);

            // 读取延迟光照结果（颜色）和深度（用于深度测试）
            
            data.inputDepth = builder.Read(depthTexture, (uint32_t)ResourceAccessType::ShaderRead);

            data.cameraUBO = cameraUBO;
        },
        [this, commandBuffer, skyBoxNode](const SkyboxPassData& data, FrameGraphPassResources& resources, void* ctx)
        {
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.outputResult);
            FrameGraphTexture& inputColor = resources.Get<FrameGraphTexture>(data.inputColor);
            FrameGraphTexture& depthTex = resources.Get<FrameGraphTexture>(data.inputDepth);

            float debugColor[4] = {0.2f, 0.6f, 1.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, "Skybox Pass", debugColor);

            // 创建 RenderPass：颜色附件 LOAD（保留光照结果），深度只读
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mWidth, (int)mHeight);

            // 颜色附件：加载已有内容（延迟光照结果），不清除
            // 天空盒会通过深度测试只绘制远平面区域
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = outputTexture.texture;
            colorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_LOAD;  // 加载已有内容
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(colorAttachment);

            // 深度附件：从 G-Buffer 继承，只读（用于深度测试）
            auto depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            depthAttachment->texture = depthTex.texture;
            depthAttachment->loadOp = ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            depthAttachment->readOnly = true;  // 只读深度，天空盒用 LEQUAL 测试
            renderPass.depthAttachment = depthAttachment;

            // 创建 RenderEncoder 并绘制天空盒
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

            // 绘制天空盒（SkyBox 内部管理自己的 Pipeline/VB/Shader/CubeMap）
            skyBoxNode->Render(renderEncoder);

            renderEncoder->EndEncode();
        }
    );

    return passData.outputResult;
}

void DeferredSceneRenderer::CollectLights(
    std::vector<DirectionLight*>& directionalLights,
    std::vector<PointLight*>& pointLights,
    std::vector<SpotLight*>& spotLights)
{
    SceneManager* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }

    // 从 SceneManager 获取所有光源
    const std::vector<Light*>& allLights = sceneManager->GetAllLights();

    // 按类型分类
    for (Light* light : allLights)
    {
        if (!light)
        {
            continue;
        }

        switch (light->getLightType())
        {
        case Light::LightType::DirectionLight:
            directionalLights.push_back(static_cast<DirectionLight*>(light));
            break;
        case Light::LightType::PointLight:
            pointLights.push_back(static_cast<PointLight*>(light));
            break;
        case Light::LightType::SpotLight:
            spotLights.push_back(static_cast<SpotLight*>(light));
            break;
        }
    }
}

void DeferredSceneRenderer::RenderForwardPass()
{
    auto* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }
}

void DeferredSceneRenderer::CollectMeshesRecursive(
    SceneNode* node,
    std::vector<DepthMeshItem>& meshItems,
    std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
    std::vector<TerrainComponent*>& terrainItems)
{
    if (!node || !node->IsVisible())
    {
        return;
    }
    
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();

    // 查询MeshRenderer组件（静态网格）
    MeshRenderer* meshRenderer = node->QueryComponentT<MeshRenderer>();
    if (meshRenderer)
    {
        MeshPtr mesh = meshRenderer->GetSharedMesh();
        if (mesh)
        {
            DepthMeshItem item;
            item.mesh = mesh;
            item.objectUBO = node->GetOrCreateModelUBO(renderDevice);
            item.materials = meshRenderer->GetMaterials();
            meshItems.push_back(item);
        }
    }

    // 查询SkinnedMeshRenderer组件（蒙皮网格）
    SkinnedMeshRenderer* skinnedMeshRenderer = node->QueryComponentT<SkinnedMeshRenderer>();
    if (skinnedMeshRenderer)
    {
        SkinnedMeshPtr mesh = skinnedMeshRenderer->GetSharedMesh();
        if (mesh)
        {
            DepthSkinnedMeshItem item;
            item.mesh = mesh;
            item.objectUBO = node->GetOrCreateModelUBO(renderDevice);
            item.materials = skinnedMeshRenderer->GetMaterials();
            skinnedMeshItems.push_back(item);
        }
    }

    // 查询TerrainComponent组件（地形 - 专属渲染路径）
    TerrainComponent* terrain = node->QueryComponentT<TerrainComponent>();
    if (terrain && terrain->IsInitialized())
    {
        terrainItems.push_back(terrain);
    }

    // 递归处理子节点
    const auto& children = node->GetAllNodes();
    for (SceneNode* child : children)
    {
        CollectMeshesRecursive(child, meshItems, skinnedMeshItems, terrainItems);
    }
}

NS_RENDERSYSTEM_END
