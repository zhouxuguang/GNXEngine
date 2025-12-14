//
//  NaniteFrameWork.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NaniteFrameWork.h"
#include "RasterClear.h"
#include "NodeAndClusterCull.h"
#include "SwapChain.h"
#include "HardwareRasterization.h"
#include "Visualization.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/LogService.h"

struct GlobaleData
{
    mathutil::Matrix4x4f modelMatrix;
    uint32_t misc0[4];
    float Nanite_ViewOrigin[4];
    float Nanite_ViewForward[4];
};

static int sCurrentMipLevelIndex = 0;
//0, 1, 2, 3, 4, 5, 6, 7, 8, 10
static uint32_t mipLevels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};

NaniteFrameWork::NaniteFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
}

void NaniteFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    
    mRenderDevice = RenderCore::GetRenderDevice();
    mHierarchyBuffer = InitHierarchyBuffer(mRenderDevice);
    mClusterSelectionArgs1 = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    mClusterSelectionArgs1->SetName("DebugBuffer");
    
    mRasterBinMeta = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    mRasterBinMeta->SetName("Nanite.RasterBinMeta");
    mRasterBinData = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    mRasterBinData->SetName("Nanite.RasterBinData");
    mMainAndPostNodeAndClusterBatches = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    mMainAndPostNodeAndClusterBatches->SetName("Nanite.MainAndPostNodeAndClusterBatches");
    mGlobalBuffer = mRenderDevice->CreateUniformBufferWithSize(sizeof(GlobaleData));
    
    mWorkArgs[0] = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024, RenderCore::StorageModeShared);
    mWorkArgs[0]->SetName("mWorkArgs0");
    mWorkArgs[1] = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    mWorkArgs[1]->SetName("mWorkArgs1");
    
    mQueueState = mRenderDevice->CreateComputeBuffer(8);
    mQueueState->SetName("QueueState");
    
    InitRasterClearPass(mRenderDevice);
    InitNodeAndClusterCullPass(mRenderDevice);

    mClusterPageData = InitNaniteMeshBuffer();
    mVisBuffer = InitVisualizeBuffer();
    mVisBuffer64 = InitVisBuffer64();

    InitHWRasterizePass(mRenderDevice);
    InitVisualizationPass(mRenderDevice);
    InitSwapChainPass(mRenderDevice);
}

void NaniteFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    
    RenderSystem::CameraPtr cameraPtr = sceneManager->getCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->createCamera("MainCamera");
    }

    cameraPtr->LookAt(mathutil::Vector3f(330.0f, 330.0f, -330.0f), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(width) / height, 0.1f, 1000.f);

    GlobaleData globalData;
    globalData.modelMatrix = mathutil::Matrix4x4f::CreateRotation(0, 1, 0, -90) * mathutil::Matrix4x4f::CreateRotation(1, 0, 0, 180);
    globalData.misc0[0] = mipLevels[sCurrentMipLevelIndex];

    mathutil::Matrix4x4f projectionMatrix = cameraPtr->GetProjectionMatrix();

    float lodScale = (0.5f * projectionMatrix[1][1] * float(width)) / 1.0f;
    float lodScaleHW = (0.5f * projectionMatrix[1][1] * float(height)) / 32.0f; // ue5 CVAR

    mathutil::Vector3f camPos = cameraPtr->GetPosition();
    globalData.Nanite_ViewOrigin[0] = camPos.x;
    globalData.Nanite_ViewOrigin[1] = camPos.y;
    globalData.Nanite_ViewOrigin[2] = camPos.z;
    globalData.Nanite_ViewOrigin[3] = lodScale;
    
    mathutil::Vector3f viewDirection = cameraPtr->GetViewDirection();
    globalData.Nanite_ViewForward[0] = viewDirection.x;
    globalData.Nanite_ViewForward[1] = viewDirection.y;
    globalData.Nanite_ViewForward[2] = viewDirection.z;
    globalData.Nanite_ViewForward[3] = lodScaleHW;

    mGlobalBuffer->SetData(&globalData, 0, sizeof(GlobaleData));
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
    
    RenderCore::CommandBufferPtr commandBuffer = mRenderDevice->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }
    
    ExecuteRasterClearPass(commandBuffer, mQueueState, mVisBuffer64);
    
    uint32_t* pData = (uint32_t*)mWorkArgs[0]->MapBufferData();
    pData[0] = 0;
    pData[1] = 0;
    pData[2] = 0;
    pData[3] = 0;
    pData[5] = 0;
    pData[6] = 1;
    mWorkArgs[0]->UnmapBufferData(pData);
    
    RenderCore::ComputeBufferPtr lastNodeAndClusterCullOutPut = nullptr;
    
    int inputArgIndex = 0;
    int outArgIndex = 1;
    for (int i = 0; i < 4; i++)
    {
        //select lod => clusters
        ExecuteNodeAndClusterCullPass(commandBuffer, i, mHierarchyBuffer, mWorkArgs[inputArgIndex],
                                mWorkArgs[outArgIndex], mQueueState, mMainAndPostNodeAndClusterBatches, mGlobalBuffer);
        
        lastNodeAndClusterCullOutPut = mWorkArgs[outArgIndex];
        inputArgIndex = (inputArgIndex + 1) % 2;
        outArgIndex = (outArgIndex + 1) % 2;
    }
    
#if 1

    //lod => hw + sw => args
    ExecuteHWRasterizePass(commandBuffer, mVisBuffer64, mClusterPageData, lastNodeAndClusterCullOutPut,
                           mMainAndPostNodeAndClusterBatches, mGlobalBuffer, 1400, 480);

    // => visBuffer64 (R32G32_UINT)
    ExecuteVisualizationPass(commandBuffer, mVisBuffer64, mVisBuffer); //visBuffer64 => visualize buffer(R32G32B32A32_FLOAT)
    
#endif

    //=> visualize buffer
    //swap chain
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    ExecuteSwapChainPass(commandBuffer, renderEncoder, mVisBuffer);
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

void NaniteFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    
    GNXEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>(GNX_BIND_EVENT_FN(OnKeyUp));
}

RenderCore::ComputeBufferPtr NaniteFrameWork::InitNaniteMeshBuffer()
{
	// 加载nanitemesh
	std::string strDataFile = GetProjectAssetDir() + "Nanite/mitsuba.nanitemesh";
	std::vector<uint8_t> hBufferData = baselib::FileUtil::ReadBinaryFile(strDataFile);
	RenderCore::ComputeBufferPtr naniteMeshBuffer = mRenderDevice->CreateComputeBuffer(hBufferData.data(), (uint32_t)hBufferData.size(),
		RenderCore::StorageMode::StorageModePrivate);
	naniteMeshBuffer->SetName("Nanite.ClusterPageData");
	return naniteMeshBuffer;
}

RenderCore::RCTexture2DPtr NaniteFrameWork::InitVisualizeBuffer()
{
    RenderCore::RCTexture2DPtr visBuffer = mRenderDevice->CreateTexture2D(RenderCore::kTexFormatRGBA32Float,
        RenderCore::TextureUsage(RenderCore::TextureUsageShaderRead | RenderCore::TextureUsageRenderTarget), 1400, 480, 1);

    visBuffer->SetName("Nanite.VisualizeBuffer");
    return visBuffer;
}

RenderCore::RCTexture2DPtr NaniteFrameWork::InitVisBuffer64()
{
    RenderCore::RCTexture2DPtr visBuffer64 = mRenderDevice->CreateTexture2D(RenderCore::kTexFormatRG32Uint,
        RenderCore::TextureUsage(RenderCore::TextureUsageShaderRead | RenderCore::TextureUsageRenderTarget), 1400, 480, 1);

    visBuffer64->SetName("Nanite.VisBuffer64");
    return visBuffer64;
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
    
    GlobaleData globalData;
    globalData.modelMatrix = mathutil::Matrix4x4f::CreateRotation(0, 1, 0, -90) * mathutil::Matrix4x4f::CreateRotation(1, 0, 0, 180);
    globalData.misc0[0] = mipLevels[sCurrentMipLevelIndex];
    mGlobalBuffer->SetData(&globalData, 0, sizeof(GlobaleData));

    return true;
}
