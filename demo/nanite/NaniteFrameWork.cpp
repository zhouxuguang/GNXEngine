//
//  NaniteFrameWork.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NaniteFrameWork.h"
#include "ClusterSelection.h"
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
    
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    sceneManager->Render(renderEncoder);
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}
