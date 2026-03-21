//
//  SSAOFrameWork.cpp
//  ssao
//
//  SSAO (Screen Space Ambient Occlusion) Demo
//

#include "SSAOFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/MathUtil/include/MathUtil.h"

SSAOFrameWork::SSAOFrameWork(const GNXEngine::WindowProps& props) 
    : GNXEngine::AppFrameWork(props)
{
}

void SSAOFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void SSAOFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    // Update camera
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    cameraPtr->LookAt(
        mathutil::Vector3f(0.0f, 2.0f, 5.0f),
        mathutil::Vector3f(0.0f, 0.0f, 0.0f),
        mathutil::Vector3f(0.0f, 1.0f, 0.0f)
    );
    cameraPtr->SetLens(60, width, height, 0.1f, 100.0f);
    
    // Create a simple light
    RenderSystem::DirectionLight* dirLight = (RenderSystem::DirectionLight*)sceneManager->CreateLight(
        "mainLight", RenderSystem::Light::DirectionLight);
    dirLight->setColor(mathutil::Vector3f(1.0f, 1.0f, 1.0f));
    dirLight->setDirection(mathutil::Vector3f(-0.5f, -1.0f, -0.5f));
    dirLight->setStrength(mathutil::Vector3f(1.0f, 1.0f, 1.0f));
}

void SSAOFrameWork::RenderFrame()
{
    using namespace RenderCore;
    using namespace RenderSystem;
    
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    
    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    
    sceneManager->Render(nullptr);
}

void SSAOFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}

bool SSAOFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
