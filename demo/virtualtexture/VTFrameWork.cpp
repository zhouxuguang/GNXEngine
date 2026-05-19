//
//  VTFrameWork.cpp
//  virtualtexture
//
//  Virtual Texture Demo
//  Demonstrates VT system initialization and per-frame tick pipeline.
//

#include "VTFrameWork.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/RenderSystem/include/VirtualTexture/FileVirtualTextureDataSource.h"

using namespace mathutil;

VTFrameWork::VTFrameWork(const GNXEngine::WindowProps& props)
    : AppFrameWork(props)
{
}

void VTFrameWork::Initlize()
{
    using namespace RenderSystem;

    SceneManager* sceneManager = SceneManager::GetInstance();

    // Camera
    auto cameraPtr = sceneManager->CreateCamera("MainCamera");
    if (cameraPtr)
    {
        cameraPtr->LookAt(Vector3f(0.0f, 5.0f, -8.0f),
                          Vector3f(0.0f, 0.0f, 0.0f),
                          Vector3f(0.0f, 1.0f, 0.0f));
        cameraPtr->SetLens(60.0f, 1280, 720, 0.1f, 100.0f);
    }

    // Direction light
    Light* light = sceneManager->CreateLight("MainLight", Light::DirectionLight);
    if (light)
    {
        light->setColor(Vector3f(1.0f, 0.95f, 0.9f));
        light->setStrength(Vector3f(1.0f, 1.0f, 1.0f));
    }

    // Virtual Texture Manager
    VirtualTextureConfig vtConfig;
    vtConfig.virtualWidth    = 8192;
    vtConfig.virtualHeight   = 8192;
    vtConfig.pageSize        = 128;
    vtConfig.pageBorder      = 4;
    vtConfig.atlasSlotsX     = 16;
    vtConfig.atlasSlotsY     = 16;
    vtConfig.pinnedMipLevels = 2;
    vtConfig.uploadsPerFrame = 4;

    mVTManager = std::make_shared<VirtualTextureManager>();
    auto fileSource = std::make_shared<FileVirtualTextureDataSource>("assets/virtualtexture/terrain");
    mVTManager->Initialize(vtConfig, fileSource);

    // Log VT info
    const auto& cfg = mVTManager->GetConfig();
    LOG_INFO("=== Virtual Texture Demo ===");
    LOG_INFO("VT: %ux%u, Page: %u, Atlas: %ux%u (%u x %u slots)",
             cfg.virtualWidth, cfg.virtualHeight,
             cfg.pageSize,
             cfg.atlasWidth, cfg.atlasHeight,
             cfg.atlasSlotsX, cfg.atlasSlotsY);
    LOG_INFO("Mip levels: %u, Pinned: %u, Uploads/frame: %u",
             cfg.mipLevels, cfg.pinnedMipLevels, cfg.uploadsPerFrame);
    LOG_INFO("PageTable: %llu bytes, Atlas: %.2f MB",
             (unsigned long long)EstimatePageTableMemory(cfg),
             EstimateAtlasMemory(cfg) / (1024.0 * 1024.0));
}

void VTFrameWork::Resize(uint32_t width, uint32_t height)
{
}

void VTFrameWork::RenderFrame()
{
    using namespace RenderSystem;

    SceneManager* sceneManager = SceneManager::GetInstance();

    if (mVTManager)
    {
        mVTManager->Tick();
    }

    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;

    sceneManager->Update(deltaTime);
    sceneManager->Render(nullptr);
}

void VTFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);
}