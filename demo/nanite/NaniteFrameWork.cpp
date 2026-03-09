//
//  NaniteFrameWork.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NaniteFrameWork.h"
#include "ClusterCull.h"
#include "RasterClear.h"
#include "NodeAndClusterCull.h"
#include "SwapChain.h"
#include "HardwareRasterization.h"
#include "Visualization.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraph.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphHelper.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphBuffer.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphExecuteContext.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/GNXEngine/include/Input.h"
#include <cstring>

static int sCurrentMipLevelIndex = 0;

//0, 1, 2, 3, 4, 5, 6, 7, 8, 10
static uint32_t mipLevels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};

NaniteFrameWork::NaniteFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
    mWidth = props.width;
    mHeight = props.height;
}

void NaniteFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    
    mRenderDevice = RenderCore::GetRenderDevice();
    mHierarchyBuffer = InitHierarchyBuffer(mRenderDevice);
    
    mGlobalBuffer = mRenderDevice->CreateUniformBufferWithSize(sizeof(GlobaleData));
    
    InitRasterClearPass(mRenderDevice);
    InitNodeAndClusterCullPass(mRenderDevice);
    InitClusterCullPass(mRenderDevice);

    mClusterPageData = InitNaniteMeshBuffer();

    InitHWRasterizePass(mRenderDevice, mWidth, mHeight);
    InitVisualizationPass(mRenderDevice);
    InitSwapChainPass(mRenderDevice);

    mTransientResources = new RenderSystem::TransientResources(mRenderDevice);
}

void NaniteFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();

    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }

    cameraPtr->LookAt(mathutil::Vector3f(330.0f, 330.0f, -330.0f), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(width) / height, 0.1f, 1000.f);

    mGlobalData.modelMatrix = mathutil::Matrix4x4f::CreateRotation(0, 1, 0, -90) * mathutil::Matrix4x4f::CreateRotation(1, 0, 0, 180);
    mGlobalData.misc0[0] = mipLevels[sCurrentMipLevelIndex];

    mathutil::Matrix4x4f projectionMatrix = cameraPtr->GetProjectionMatrix();

    float lodScale = (0.5f * projectionMatrix[1][1] * float(width)) / 1.0f;
    float lodScaleHW = (0.5f * projectionMatrix[1][1] * float(height)) / 32.0f; // ue5 CVAR

    mathutil::Vector3f camPos = cameraPtr->GetPosition();
    mGlobalData.Nanite_ViewOrigin[0] = camPos.x;
    mGlobalData.Nanite_ViewOrigin[1] = camPos.y;
    mGlobalData.Nanite_ViewOrigin[2] = camPos.z;
    mGlobalData.Nanite_ViewOrigin[3] = lodScale;
    
    mathutil::Vector3f viewDirection = cameraPtr->GetViewDirection();
    mGlobalData.Nanite_ViewForward[0] = viewDirection.x;
    mGlobalData.Nanite_ViewForward[1] = viewDirection.y;
    mGlobalData.Nanite_ViewForward[2] = viewDirection.z;
    mGlobalData.Nanite_ViewForward[3] = lodScaleHW;

    mGlobalBuffer->SetData(&mGlobalData, 0, sizeof(GlobaleData));
    //mGlobalBuffer->SetName("Nanite.GlobalBuffer");
}

void NaniteFrameWork::RenderFrame()
{
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    LOG_INFO("deltaTime = %f\n", deltaTime);

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    sceneManager->Update(deltaTime);

    {
		// WS按键前后移动
		int moveState = 0;
		if (GNXEngine::Input::IsKeyPressed(GNXEngine::W))
		{
			moveState = 1;
		}
		else if (GNXEngine::Input::IsKeyPressed(GNXEngine::S))
		{
			moveState = 2;
		}

		float moveSpeed = 100.0f;
        RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
        mathutil::Vector3f camPossition = cameraPtr->GetPosition();

		if (moveState == 1)
		{
            camPossition += cameraPtr->GetViewDirection() * moveSpeed * deltaTime;
            cameraPtr->SetPosition(camPossition);
		}
		else if (moveState == 2)
		{
			camPossition -= cameraPtr->GetViewDirection() * moveSpeed * deltaTime;
            cameraPtr->SetPosition(camPossition);
		}

		mathutil::Vector3f camPos = cameraPtr->GetPosition();
		mGlobalData.Nanite_ViewOrigin[0] = camPos.x;
		mGlobalData.Nanite_ViewOrigin[1] = camPos.y;
		mGlobalData.Nanite_ViewOrigin[2] = camPos.z;

		mathutil::Vector3f viewDirection = cameraPtr->GetViewDirection();
		mGlobalData.Nanite_ViewForward[0] = viewDirection.x;
		mGlobalData.Nanite_ViewForward[1] = viewDirection.y;
		mGlobalData.Nanite_ViewForward[2] = viewDirection.z;

		mGlobalBuffer->SetData(&mGlobalData, 0, sizeof(GlobaleData));
    }

    // 从Graphics队列创建命令缓冲区
    RenderCore::CommandQueuePtr graphicsQueue = mRenderDevice->GetCommandQueue(RenderCore::QueueType::Graphics, 0);
    RenderCore::CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }

    // 创建FrameGraph
    RenderSystem::FrameGraph frameGraph;

    // Import真正的外部只读资源到FrameGraph
    // 注意：只Import真正不变的外部资源（如从文件加载的静态数据）
    // transient资源在FrameGraph内部创建，避免版本不一致
    RenderSystem::FrameGraphResource importedHierarchyBuffer = RenderSystem::ImportBuffer(frameGraph, "ImportedHierarchyBuffer", mHierarchyBuffer);
    RenderSystem::FrameGraphResource importedClusterPageData = RenderSystem::ImportBuffer(frameGraph, "ImportedClusterPageData", mClusterPageData);

    // 定义RasterClearPass
    struct RasterClearPassData
    {
        RenderSystem::FrameGraphResource queueState;
        RenderSystem::FrameGraphResource workArgs0;
        RenderSystem::FrameGraphResource workArgs1;
        RenderSystem::FrameGraphResource visBuffer64;
    };

    auto& rasterClearData = frameGraph.AddPass<RasterClearPassData>(
        "RasterClear",
        [&](RenderSystem::FrameGraph::Builder& builder, RasterClearPassData& data)
        {
            // 创建queueState资源
            RenderSystem::FrameGraphBuffer::Desc queueStateDesc;
            memset(queueStateDesc.name, 0, sizeof(queueStateDesc.name));
            strncpy(queueStateDesc.name, "QueueState", sizeof(queueStateDesc.name) - 1);
            queueStateDesc.size = 8;

            // 创建workArgs0和workArgs1资源
            RenderSystem::FrameGraphBuffer::Desc workArgsDesc;
            memset(workArgsDesc.name, 0, sizeof(workArgsDesc.name));
            strncpy(workArgsDesc.name, "WorkArgs0", sizeof(workArgsDesc.name) - 1);
            workArgsDesc.size = 4 * 1024 * 1024;

            // 创建visBuffer64资源
            RenderSystem::FrameGraphTexture::Desc visBuffer64Desc;
            memset(visBuffer64Desc.name, 0, sizeof(visBuffer64Desc.name));
            strncpy(visBuffer64Desc.name, "VisBuffer64", sizeof(visBuffer64Desc.name) - 1);
            visBuffer64Desc.extent = RenderCore::Rect2D(0, 0, mWidth, mHeight);
            visBuffer64Desc.depth = 1;
            visBuffer64Desc.numMipLevels = 1;
            visBuffer64Desc.layers = 1;
            visBuffer64Desc.format = RenderCore::kTexFormatRG32Uint;

            data.queueState = builder.Create<RenderSystem::FrameGraphBuffer>("QueueState", queueStateDesc);
            data.queueState = builder.Write(data.queueState, (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

            data.workArgs0 = builder.Create<RenderSystem::FrameGraphBuffer>("WorkArgs0", workArgsDesc);
            data.workArgs0 = builder.Write(data.workArgs0, (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

            data.workArgs1 = builder.Create<RenderSystem::FrameGraphBuffer>("WorkArgs1", workArgsDesc);
            data.workArgs1 = builder.Write(data.workArgs1, (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

            data.visBuffer64 = builder.Create<RenderSystem::FrameGraphTexture>("VisBuffer64", visBuffer64Desc);
            data.visBuffer64 = builder.Write(data.visBuffer64, (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

            //builder.SetSideEffect();
        },
        [&](const RasterClearPassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
            RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

            RenderCore::RCBufferPtr queueState = RenderSystem::GetBuffer(resources, data.queueState);
            RenderCore::RCBufferPtr workArgs0 = RenderSystem::GetBuffer(resources, data.workArgs0);
            RenderCore::RCBufferPtr workArgs1 = RenderSystem::GetBuffer(resources, data.workArgs1);
            RenderCore::RCTexturePtr visBuffer64 = RenderSystem::GetTexture(resources, data.visBuffer64);

            ExecuteRasterClearPass(cmdBuffer, queueState, workArgs0, workArgs1,
                std::dynamic_pointer_cast<RenderCore::RCTexture2D>(visBuffer64));
        }
    );

    // 获取资源ID，供后续pass使用
    RenderSystem::FrameGraphResource visBuffer64 = rasterClearData.visBuffer64;
    RenderSystem::FrameGraphResource queueState = rasterClearData.queueState;
    RenderSystem::FrameGraphResource workArgs0 = rasterClearData.workArgs0;
    RenderSystem::FrameGraphResource workArgs1 = rasterClearData.workArgs1;

    // 定义NodeAndClusterCullPass（执行4次）
    struct NodeAndClusterCullPassData
    {
        uint32_t level;
        RenderSystem::FrameGraphResource hierarchyBuffer;
        RenderSystem::FrameGraphResource inWorkArgs;
        RenderSystem::FrameGraphResource outResult;
        RenderSystem::FrameGraphResource queueState;
        RenderSystem::FrameGraphResource mainAndPostNodeAndClusterBatches;
    };

    RenderSystem::FrameGraphResource currentWorkArgs = workArgs0;
    RenderSystem::FrameGraphResource nextWorkArgs = workArgs1;
    RenderSystem::FrameGraphResource mainAndPostNodeAndClusterBatches = RenderSystem::FrameGraphResource{};

    for (int i = 0; i < 4; i++)
    {
        char passName[64];
        snprintf(passName, 64, "NodeAndClusterCull_%d", i);

        auto& nodeAndClusterCullData = frameGraph.AddPass<NodeAndClusterCullPassData>(
            passName,
            [&](RenderSystem::FrameGraph::Builder& builder, NodeAndClusterCullPassData& data)
            {
                //return;
                data.level = i;
                data.hierarchyBuffer = builder.Read(importedHierarchyBuffer, static_cast<uint32_t>(RenderCore::ResourceAccessType::ComputeShaderRead));
                data.inWorkArgs = builder.Read(currentWorkArgs, static_cast<uint32_t>(RenderCore::ResourceAccessType::ComputeShaderRead));
                // Write返回新版本的资源ID，保存起来供下次使用
                RenderSystem::FrameGraphResource newOutResult = builder.Write(nextWorkArgs, static_cast<uint32_t>(RenderCore::ResourceAccessType::ComputeShaderWrite));
                data.outResult = newOutResult;
                // 更新nextWorkArgs为新的版本
                nextWorkArgs = newOutResult;
                data.queueState = data.queueState = builder.Read(queueState, 
                                  (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

                // 第一次迭代时创建mainAndPostNodeAndClusterBatches资源
                if (i == 0)
                {
                    RenderSystem::FrameGraphBuffer::Desc mainAndPostNodeAndClusterBatchesDesc;
                    memset(mainAndPostNodeAndClusterBatchesDesc.name, 0, sizeof(mainAndPostNodeAndClusterBatchesDesc.name));
                    strncpy(mainAndPostNodeAndClusterBatchesDesc.name, "MainAndPostNodeAndClusterBatches", sizeof(mainAndPostNodeAndClusterBatchesDesc.name) - 1);
                    mainAndPostNodeAndClusterBatchesDesc.size = 4 * 1024 * 1024;

                    data.mainAndPostNodeAndClusterBatches = builder.Create<RenderSystem::FrameGraphBuffer>("MainAndPostNodeAndClusterBatches", mainAndPostNodeAndClusterBatchesDesc);
                    data.mainAndPostNodeAndClusterBatches = builder.Write(data.mainAndPostNodeAndClusterBatches, 
                        (uint32_t)(RenderCore::ResourceAccessType::ComputeShaderWrite));
                }
                else
                {
                    data.mainAndPostNodeAndClusterBatches = builder.Write(mainAndPostNodeAndClusterBatches,
                        static_cast<uint32_t>(RenderCore::ResourceAccessType::ComputeShaderWrite));
                }
            },
            [&](const NodeAndClusterCullPassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
            {
                RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
                RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

                RenderCore::RCBufferPtr hierarchyBuffer = RenderSystem::GetBuffer(resources, data.hierarchyBuffer);
                RenderCore::RCBufferPtr inWorkArgs = RenderSystem::GetBuffer(resources, data.inWorkArgs);
                RenderCore::RCBufferPtr outResult = RenderSystem::GetBuffer(resources, data.outResult);
                RenderCore::RCBufferPtr queueState = RenderSystem::GetBuffer(resources, data.queueState);
                RenderCore::RCBufferPtr mainAndPostNodeAndClusterBatches = RenderSystem::GetBuffer(resources, data.mainAndPostNodeAndClusterBatches);

                ExecuteNodeAndClusterCullPass(cmdBuffer, data.level, hierarchyBuffer, inWorkArgs,
                    outResult, queueState, mainAndPostNodeAndClusterBatches, mGlobalBuffer);
            }
        );

        // 迭代后保存mainAndPostNodeAndClusterBatches的资源ID
        mainAndPostNodeAndClusterBatches = nodeAndClusterCullData.mainAndPostNodeAndClusterBatches;

        // 交换work args
        RenderSystem::FrameGraphResource temp = currentWorkArgs;
        currentWorkArgs = nextWorkArgs;
        nextWorkArgs = temp;
    }

    // 定义ClusterCullPass
    struct ClusterCullPassData
    {
        RenderSystem::FrameGraphResource mainAndPostNodeAndClusterBatches;
        RenderSystem::FrameGraphResource workArgs;
        RenderSystem::FrameGraphResource clusterPageData;
        RenderSystem::FrameGraphResource visibleClustersSWHW;
    };

    auto& clusterCullData = frameGraph.AddPass<ClusterCullPassData>(
        "ClusterCull",
        [&](RenderSystem::FrameGraph::Builder& builder, ClusterCullPassData& data)
        {
            // 创建visibleClustersSWHW资源
            RenderSystem::FrameGraphBuffer::Desc visibleClustersSWHWDesc;
            memset(visibleClustersSWHWDesc.name, 0, sizeof(visibleClustersSWHWDesc.name));
            strncpy(visibleClustersSWHWDesc.name, "VisibleClustersSWHW", sizeof(visibleClustersSWHWDesc.name) - 1);
            visibleClustersSWHWDesc.size = 4 * 1024 * 1024;

            data.visibleClustersSWHW = builder.Create<RenderSystem::FrameGraphBuffer>("VisibleClustersSWHW", visibleClustersSWHWDesc);
            data.visibleClustersSWHW = builder.Write(data.visibleClustersSWHW, (uint32_t)RenderCore::ResourceAccessType::ComputeShaderWrite);

            data.mainAndPostNodeAndClusterBatches = builder.Read(mainAndPostNodeAndClusterBatches,
                static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead | RenderCore::ResourceAccessType::ComputeShaderWrite));
            data.clusterPageData = builder.Read(importedClusterPageData, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
            data.workArgs = builder.Read(currentWorkArgs, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
        },
        [&](const ClusterCullPassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
            RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

            RenderCore::RCBufferPtr mainAndPostNodeAndClusterBatches = RenderSystem::GetBuffer(resources, data.mainAndPostNodeAndClusterBatches);
            RenderCore::RCBufferPtr clusterPageData = RenderSystem::GetBuffer(resources, data.clusterPageData);
            RenderCore::RCBufferPtr workArgs = RenderSystem::GetBuffer(resources, data.workArgs);
            RenderCore::RCBufferPtr visibleClustersSWHW = RenderSystem::GetBuffer(resources, data.visibleClustersSWHW);

            ExecuteClusterCullPass(cmdBuffer, mainAndPostNodeAndClusterBatches, workArgs,
                clusterPageData, visibleClustersSWHW, mGlobalBuffer);
        }
    );

    // 获取visibleClustersSWHW的资源ID，供后续pass使用
    RenderSystem::FrameGraphResource visibleClustersSWHW = clusterCullData.visibleClustersSWHW;

    // 定义HWRasterizePass
    struct HWRasterizePassData
    {
        RenderSystem::FrameGraphResource visBuffer64;
        RenderSystem::FrameGraphResource clusterPageData;
        RenderSystem::FrameGraphResource drawArgs;
        RenderSystem::FrameGraphResource visibleClustersSWHW;
        RenderSystem::FrameGraphResource mainAndPostNodeAndClusterBatches;
    };

    auto& hwRasterizeData = frameGraph.AddPass<HWRasterizePassData>(
        "HWRasterize",
        [&](RenderSystem::FrameGraph::Builder& builder, HWRasterizePassData& data)
        {
            //return;
            // Write返回新版本的资源ID，更新visBuffer64
            RenderSystem::FrameGraphResource newVisBuffer64 = builder.Write(visBuffer64, 
                static_cast<uint32_t>(RenderCore::ResourceAccessType::ColorAttachment));
            data.visBuffer64 = newVisBuffer64;
            visBuffer64 = newVisBuffer64;
            data.clusterPageData = builder.Read(importedClusterPageData, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
            data.drawArgs = builder.Read(currentWorkArgs, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
            data.visibleClustersSWHW = builder.Read(visibleClustersSWHW, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
            data.mainAndPostNodeAndClusterBatches = builder.Read(mainAndPostNodeAndClusterBatches, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
        },
        [&](const HWRasterizePassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
            RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

            RenderCore::RCTexturePtr visBuffer64 = RenderSystem::GetTexture(resources, data.visBuffer64);
            RenderCore::RCBufferPtr clusterPageData = RenderSystem::GetBuffer(resources, data.clusterPageData);
            RenderCore::RCBufferPtr drawArgs = RenderSystem::GetBuffer(resources, data.drawArgs);
            RenderCore::RCBufferPtr visibleClustersSWHW = RenderSystem::GetBuffer(resources, data.visibleClustersSWHW);
            RenderCore::RCBufferPtr mainAndPostNodeAndClusterBatches = RenderSystem::GetBuffer(resources, data.mainAndPostNodeAndClusterBatches);

            ExecuteHWRasterizePass(cmdBuffer, std::dynamic_pointer_cast<RenderCore::RCTexture2D>(visBuffer64),
                clusterPageData, drawArgs, visibleClustersSWHW, mGlobalBuffer, mWidth, mHeight);
        }
    );

    // 定义VisualizationPass
    struct VisualizationPassData
    {
        RenderSystem::FrameGraphResource visBuffer64;
        RenderSystem::FrameGraphResource visBuffer;
    };

    auto& visualizationData = frameGraph.AddPass<VisualizationPassData>(
        "Visualization",
        [&](RenderSystem::FrameGraph::Builder& builder, VisualizationPassData& data)
        {
            //return;
            data.visBuffer64 = builder.Read(visBuffer64, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));

            // 创建visBuffer资源
            RenderSystem::FrameGraphTexture::Desc visBufferDesc;
            memset(visBufferDesc.name, 0, sizeof(visBufferDesc.name));
            strncpy(visBufferDesc.name, "VisBuffer", sizeof(visBufferDesc.name) - 1);
            visBufferDesc.extent = RenderCore::Rect2D(0, 0, mWidth, mHeight);
            visBufferDesc.depth = 1;
            visBufferDesc.numMipLevels = 1;
            visBufferDesc.layers = 1;
            visBufferDesc.format = RenderCore::kTexFormatRGBA32Float;

            data.visBuffer = builder.Create<RenderSystem::FrameGraphTexture>("VisBuffer", visBufferDesc);
            data.visBuffer = builder.Write(data.visBuffer, static_cast<uint32_t>(RenderCore::ResourceAccessType::ComputeShaderWrite));
        },
        [&](const VisualizationPassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
            RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

            RenderCore::RCTexturePtr visBuffer64 = RenderSystem::GetTexture(resources, data.visBuffer64);
            RenderCore::RCTexturePtr visBuffer = RenderSystem::GetTexture(resources, data.visBuffer);

            ExecuteVisualizationPass(cmdBuffer, std::dynamic_pointer_cast<RenderCore::RCTexture2D>(visBuffer64),
                std::dynamic_pointer_cast<RenderCore::RCTexture2D>(visBuffer));
        }
    );

    // 获取visBuffer的资源ID，供后续pass使用
    RenderSystem::FrameGraphResource visBuffer = visualizationData.visBuffer;

    // 定义SwapChainPass（作为副作用，因为它直接渲染到swap chain）
    struct SwapChainPassData
    {
        RenderSystem::FrameGraphResource visBuffer;
    };

    frameGraph.AddPass<SwapChainPassData>(
        "SwapChain",
        [&](RenderSystem::FrameGraph::Builder& builder, SwapChainPassData& data)
        {
            data.visBuffer = builder.Read(visBuffer, static_cast<uint32_t>(RenderCore::ResourceAccessType::ShaderRead));
            builder.SetSideEffect();
        },
        [&](const SwapChainPassData& data, RenderSystem::FrameGraphPassResources& resources, void* context)
        {
            RenderSystem::FrameGraphExecuteContext* executeContext = static_cast<RenderSystem::FrameGraphExecuteContext*>(context);
            RenderCore::CommandBufferPtr cmdBuffer = executeContext->commandBuffer;

            RenderCore::RCTexturePtr visBuffer = RenderSystem::GetTexture(resources, data.visBuffer);
            RenderCore::RenderEncoderPtr renderEncoder = cmdBuffer->CreateDefaultRenderEncoder();

            ExecuteSwapChainPass(cmdBuffer, renderEncoder, std::dynamic_pointer_cast<RenderCore::RCTexture2D>(visBuffer));

            renderEncoder->EndEncode();
        }
    );

    // 编译FrameGraph
    frameGraph.Compile();

    // 创建执行上下文
    RenderSystem::FrameGraphExecuteContext executeContext;
    executeContext.commandBuffer = commandBuffer;

    // 执行FrameGraph
    frameGraph.Execute(&executeContext, mTransientResources);

    mTransientResources->Update(deltaTime);

    // 呈现到屏幕
    commandBuffer->PresentFrameBuffer();
}

void NaniteFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    
    GNXEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>(GNX_BIND_EVENT_FN(OnKeyUp));
}

RenderCore::RCBufferPtr NaniteFrameWork::InitNaniteMeshBuffer()
{
	// 加载nanitemesh
	std::string strDataFile = GetProjectAssetDir() + "Nanite/mitsuba.nanitemesh";
	std::vector<uint8_t> hBufferData = baselib::FileUtil::ReadBinaryFile(strDataFile);
	RenderCore::RCBufferDesc desc(static_cast<uint32_t>(hBufferData.size()),
		RenderCore::RCBufferUsage::StorageBuffer, RenderCore::StorageMode::StorageModePrivate);
	RenderCore::RCBufferPtr naniteMeshBuffer = mRenderDevice->CreateBuffer(desc, hBufferData.data());
	naniteMeshBuffer->SetName("Nanite.ClusterPageData");
	return naniteMeshBuffer;
}

bool NaniteFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    if (e.GetKeyCode() == GNXEngine::Up)
    {
        sCurrentMipLevelIndex ++;
    }
    else if (e.GetKeyCode() == GNXEngine::Down)
    {
        sCurrentMipLevelIndex --;
    }
    if (sCurrentMipLevelIndex < 0) 
    {
        sCurrentMipLevelIndex = 9;
    }
    else if (sCurrentMipLevelIndex > 9) 
    {
        sCurrentMipLevelIndex = 0;
    }
    
    mGlobalData.misc0[0] = mipLevels[sCurrentMipLevelIndex];
    mGlobalBuffer->SetData(&mGlobalData, 0, sizeof(GlobaleData));

    return true;
}
