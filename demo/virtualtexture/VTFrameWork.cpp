//
//  VTFrameWork.cpp
//  virtualtexture
//
//  Virtual Texture Demo
//  Demonstrates VT system initialization and per-frame tick pipeline.
//

#include "VTFrameWork.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/RenderSystem/include/VirtualTexture/FileVirtualTextureDataSource.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/Material.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include <memory>

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
        cameraPtr->LookAt(Vector3f(0.0f, 30.0f, 80.0f),
                          Vector3f(0.0f, 0.0f, 0.0f),
                          Vector3f(0.0f, 1.0f, 0.0f));
        cameraPtr->SetLens(60.0f, 1280, 720, 1.0f, 500.0f);
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
    vtConfig.virtualWidth    = 4096;
    vtConfig.virtualHeight   = 4096;
    vtConfig.pageSize        = 512;
    vtConfig.pageBorder      = 2;
    vtConfig.atlasSlotsX     = 8;
    vtConfig.atlasSlotsY     = 8;
    vtConfig.pinnedMipLevels = 2;
    vtConfig.uploadsPerFrame = 4;

    auto fileSource = std::make_shared<FileVirtualTextureDataSource>("vt/pages", "png");
    mathutil::Vector2i viewSize(1280, 720);
    mVTIndex = sceneManager->AddVTManager(vtConfig, fileSource, viewSize, 16);

    // Log VT info
    auto vt = sceneManager->GetVTManager(mVTIndex);
    const auto& cfg = vt->GetConfig();
    LOG_INFO("=== Virtual Texture Demo ===");
    LOG_INFO("VT: %ux%u, Page: %u, Atlas: %ux%u (%u x %u slots)",
             cfg.virtualWidth, cfg.virtualHeight,
             cfg.pageSize,
             cfg.atlasWidth, cfg.atlasHeight,
             cfg.atlasSlotsX, cfg.atlasSlotsY);
    LOG_INFO("Mip levels: %u, Pinned: %u, Uploads/frame: %u", cfg.mipLevels, cfg.pinnedMipLevels, cfg.uploadsPerFrame);
    LOG_INFO("PageTable: %llu bytes, Atlas: %.2f MB", (unsigned long long)EstimatePageTableMemory(cfg),
             EstimateAtlasMemory(cfg) / (1024.0 * 1024.0));

    // ── 加载模型（普通模型） ──
    {
        std::string modelPath = GetProjectAssetDir() + "vt/snowy_mountain.obj";

        // 山体模型，放在原点，不缩放
        Transform transform;
        transform.position = Vector3f(0.0f, 0.0f, 0.0f);
        transform.rotation = Quaternionf();
        transform.scale    = Vector3f(1.0f, 1.0f, 1.0f);

        SceneNode* modelNode = sceneManager->GetRootNode()->CreateRendererNode(
            "VTModel", modelPath, transform.position, transform.rotation, transform.scale);

        if (modelNode)
        {
            // 将模型所有材质替换为 VirtualTexturePBR
            MeshRenderer* meshRender = modelNode->QueryComponentT<MeshRenderer>();
            if (meshRender)
            {
                const auto& materials = meshRender->GetMaterials();
                for (const auto& mat : materials)
                {
                    mat->SetMaterialType(Material::MaterialType::VirtualTexturePBR);
                    mat->SetTexture("normalTexture", nullptr);
                    mat->SetTexture("roughnessTexture", nullptr);
                    mat->SetTexture("emissiveTexture", nullptr);
                    mat->SetTexture("ambientTexture", nullptr);
                }
                LOG_INFO("Replaced %zu material(s) with VirtualTexturePBR", materials.size());
            }
        }
    }

    LOG_INFO("VT demo scene setup complete.");
}

void VTFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
}

void VTFrameWork::RenderFrame()
{
    using namespace RenderSystem;

    SceneManager* sceneManager = SceneManager::GetInstance();

    // VT Tick 已由 SceneManager::Update() 内部统一处理

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
