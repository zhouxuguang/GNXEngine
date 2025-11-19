//
//  NaniteFrameWork.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NaniteFrameWork.h"
#include "ClusterSelection.h"
#include "SwapChain.h"
#include "HardwareRasterization.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/LogService.h"

NaniteFrameWork::NaniteFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
    //
}

void NaniteFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    
    mRenderDevice = RenderCore::GetRenderDevice();
    mHierarchyBuffer = InitHierarchyBuffer(mRenderDevice);
    mClusterSelectionArgs1 = mRenderDevice->CreateComputeBuffer(4 * 1024 * 1024);
    InitClusterSelectionPass(mRenderDevice);

    mClusterPageData = InitNaniteMeshBuffer();
    mVisBuffer = InitVisualizeBuffer();

    InitHWRasterizePass(mRenderDevice, mClusterPageData, mVisBuffer);
    InitSwapChainPass(mRenderDevice);
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
    ExecuteClusterSelectionPass(commandBuffer, mHierarchyBuffer, mClusterSelectionArgs1);
    ExecuteHWRasterizePass(commandBuffer, nullptr, mVisBuffer);

    //=> visualize buffer
    //swap chain
    
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    ExecuteSwapChainPass(commandBuffer, renderEncoder);
    //sceneManager->Render(renderEncoder);
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
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
        RenderCore::TextureUsage(RenderCore::TextureUsageShaderRead | RenderCore::TextureUsageRenderTarget), 1399, 479, 1);

    //visBuffer->SetName("Nanite.VisualizeBuffer");
    return visBuffer;
}
