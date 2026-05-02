//
//  TerrainFrameWork.cpp
//  terrain
//
//  Terrain Phase 1 Demo
//  - Procedural heightmap terrain mesh
//  - Height-based diffuse coloring (green -> brown -> gray -> white)
//  - PBR material through the deferred rendering pipeline
//  - Directional lighting + skybox
//

#include "TerrainFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include "Runtime/RenderSystem/include/SkyBox.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include "Runtime/RenderSystem/include/Material.h"
#include "Runtime/RenderSystem/include/terrain/TerrainGenerator.h"
#include "Runtime/RenderSystem/include/terrain/TerrainComponent.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <cmath>
#include <algorithm>

//=============================================================================
// Helper: Create a default gradient skybox
//=============================================================================

static RenderSystem::SkyBox* CreateDefaultSkybox()
{
    using namespace imagecodec;

    const uint32_t faceSize = 256;
    auto renderDevice = RenderCore::GetRenderDevice();

    // 根据 3D 方向向量计算天空颜色，所有面统一使用此函数
    // elevation = dir.y: -1(正下方) → 0(地平线) → +1(天顶)
    auto skyColor = [](Vector3f dir) -> Vector3f
    {
        dir.Normalize();
        float elevation = dir.y;

        if (elevation < 0.0f)
        {
            // 地平线以下：暗色地面
            float t = -elevation;
            return Vector3f(0.15f, 0.17f, 0.22f) * (1.0f - t)
                 + Vector3f(0.10f, 0.12f, 0.18f) * t;
        }
        else
        {
            // 地平线到天顶：亮蓝 → 深蓝
            float t = elevation;
            return Vector3f(0.70f, 0.82f, 0.95f) * (1.0f - t)
                 + Vector3f(0.25f, 0.45f, 0.80f) * t;
        }
    };

    std::vector<VImagePtr> faceImages(6);

    for (int face = 0; face < 6; ++face)
    {
        uint8_t* data = (uint8_t*)malloc(faceSize * faceSize * 4);

        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                float su = 2.0f * (float)x / (float)(faceSize - 1) - 1.0f;
                float sv = 2.0f * (float)y / (float)(faceSize - 1) - 1.0f;

                // Cubemap (face, u, v) → 3D 方向向量
                Vector3f dir;
                switch (face)
                {
                case 0: dir = Vector3f( 1, -sv, -su); break;  // +X
                case 1: dir = Vector3f(-1, -sv,  su); break;  // -X
                case 2: dir = Vector3f( su, 1,  -sv); break;  // +Y (top)
                case 3: dir = Vector3f( su, -1,  sv); break;  // -Y (bottom)
                case 4: dir = Vector3f( su, -sv,  1); break;  // +Z
                case 5: dir = Vector3f(-su, -sv, -1); break;  // -Z
                }

                Vector3f color = skyColor(dir);

                uint32_t idx = (y * faceSize + x) * 4;
                data[idx + 0] = (uint8_t)(std::min(std::max(color.x, 0.0f), 1.0f) * 255.0f);
                data[idx + 1] = (uint8_t)(std::min(std::max(color.y, 0.0f), 1.0f) * 255.0f);
                data[idx + 2] = (uint8_t)(std::min(std::max(color.z, 0.0f), 1.0f) * 255.0f);
                data[idx + 3] = 255;
            }
        }

        faceImages[face] = std::make_shared<VImage>();
        faceImages[face]->SetImageInfo(FORMAT_RGBA8, faceSize, faceSize, data, free);
    }

    return RenderSystem::SkyBox::create(renderDevice,
        faceImages[0], faceImages[1],
        faceImages[2], faceImages[3],
        faceImages[4], faceImages[5]);
}

//=============================================================================
// Terrain configuration
//=============================================================================

static const uint32_t kTerrainResolution = 512;
static const float   kTerrainWorldSize  = 163840.0f;
static const float   kTerrainHeightScale = 0.5f;
static const uint32_t kMaxQuadTreeLevel = 10;

//=============================================================================
// Implementation
//=============================================================================

TerrainFrameWork::TerrainFrameWork(const GNXEngine::WindowProps& props)
    : GNXEngine::AppFrameWork(props)
{
}

void TerrainFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void TerrainFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);

    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // ---- Camera ----
    RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
    if (!camera)
    {
        camera = sceneManager->CreateCamera("MainCamera");
    }
    camera->LookAt(
        Vector3f(0.0f, 70000.0f, 90000.0f),
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f)
    );
    // 远平面需要覆盖整个地形范围（对角线距离 + 相机高度余量）
    // 地形大小 163840，对角线约 231700，加上相机位置偏移，设为 300000
    camera->SetLens(60.0f, width, height, 1.0f, 300000.0f);

    // ---- Directional Light (sun) ----
    RenderSystem::DirectionLight* dirLight = static_cast<RenderSystem::DirectionLight*>(
        sceneManager->CreateLight("sun", RenderSystem::Light::DirectionLight));
    dirLight->setColor(Vector3f(1.0f, 1.0f, 1.f));
    dirLight->setDirection(Vector3f(0.0f, 1.0f, -0.0f).Normalize());
    dirLight->setStrength(Vector3f(2.0f, 2.0f, 2.0f));

    // ---- Skybox ----
    mSkyBox = CreateDefaultSkybox();
    if (mSkyBox)
    {
        sceneManager->GetSkyBox()->AttachSkyBoxObject(mSkyBox);
    }

    // ---- Terrain (QuadTree with adaptive LOD, TerrainComponent) ----
    std::string heightmapPath = GetProjectAssetDir() + "terrain/ps_height_1k.png";
    std::string texturePath   = GetProjectAssetDir() + "terrain/ps_texture_1k.png";

    LOG_INFO("Generating QuadTree terrain (worldSize=%.0f, heightScale=%.1f, maxLevel=%u)...",
             kTerrainWorldSize, kTerrainHeightScale, kMaxQuadTreeLevel);

    // Create TerrainComponent and initialize from heightmap
    mTerrainComponent = new RenderSystem::TerrainComponent();
    mTerrainComponent->InitFromHeightMap(
        heightmapPath.c_str(), kTerrainWorldSize, kTerrainHeightScale, kMaxQuadTreeLevel);

    if (!mTerrainComponent->IsInitialized())
    {
        LOG_WARN("Failed to load heightmap, falling back to procedural terrain");
        mTerrainComponent->InitProcedural(
            kTerrainResolution, kTerrainWorldSize, kTerrainHeightScale, kMaxQuadTreeLevel);
    }

    // Load diffuse texture from image, or generate procedural one as fallback
    RenderCore::RCTexture2DPtr diffuseTexture = RenderSystem::TerrainGenerator::LoadDiffuseTexture(
        texturePath.c_str());

    if (!diffuseTexture)
    {
        LOG_WARN("Failed to load terrain texture, falling back to procedural coloring");
        diffuseTexture = RenderSystem::TerrainGenerator::GenerateDiffuseTexture(
            kTerrainResolution, kTerrainWorldSize, kTerrainHeightScale);
    }

    // Create default PBR textures
    RenderCore::RCTexturePtr normalTexture   = RenderSystem::ImageTextureUtil::CreateNormalTexture();
    RenderCore::RCTexturePtr metalRoughTexture = RenderSystem::ImageTextureUtil::CreateMetalRoughTexture();
    RenderCore::RCTexturePtr emissiveTexture  = RenderSystem::ImageTextureUtil::CreateEmmisveTexture();
    RenderCore::RCTexturePtr aoTexture        = RenderSystem::ImageTextureUtil::CreateAOTexture();

    // Create PBR material and bind textures
    RenderSystem::MaterialPtr material = std::make_shared<RenderSystem::Material>();
    material->SetMaterialType(RenderSystem::Material::MaterialType::PBR);
    material->SetTexture("diffuseTexture", diffuseTexture);
    material->SetTexture("normalTexture", normalTexture);
    material->SetTexture("roughnessTexture", metalRoughTexture);
    material->SetTexture("emissiveTexture", emissiveTexture);
    material->SetTexture("ambientTexture", aoTexture);

    // Assign material to TerrainComponent
    mTerrainComponent->SetMaterial(material);
    mTerrainComponent->SetWireframe(true);
    mTerrainComponent->SetUseGPUCulling(false);

    // Add TerrainComponent to scene (NOT MeshRenderer)
    auto* terrainNode = sceneManager->GetRootNode()->CreateChildSceneNode("Terrain");
    terrainNode->AddComponent(mTerrainComponent);

    LOG_INFO("Terrain scene setup complete (TerrainComponent).");
}

void TerrainFrameWork::RenderFrame()
{
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;

    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // Update terrain LOD based on camera position and screen-space error
    if (mTerrainComponent && mTerrainComponent->IsInitialized())
    {
        RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
        if (camera)
        {
            mathutil::Vector3f camPos = camera->GetPosition();
            float fovY = camera->GetFOV();
            mathutil::Vector2i viewSize = camera->GetViewSize();
            mTerrainComponent->GetQuadTreeTerrain()->Update(camPos, fovY, (float)viewSize.y);
        }
    }

    sceneManager->Update(deltaTime);

    // Render via deferred pipeline (G-Buffer -> Deferred Lighting)
    sceneManager->Render(nullptr);
}

void TerrainFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);
}
