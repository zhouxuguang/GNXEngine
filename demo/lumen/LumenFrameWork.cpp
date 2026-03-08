//
//  LumenFrameWork.cpp
//  lumen
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "LumenFrameWork.h"

LumenFrameWork::LumenFrameWork(const GNXEngine::WindowProps& props) : GNXEngine::AppFrameWork(props)
{
//    mWidth = props.width;
//    mHeight = props.height;
}

void LumenFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    
    //mRenderDevice = RenderCore::GetRenderDevice();
}

void LumenFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
}

void LumenFrameWork::RenderFrame()
{
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    //LOG_INFO("deltaTime = %f\n", deltaTime);

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
}

void LumenFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}

bool LumenFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
