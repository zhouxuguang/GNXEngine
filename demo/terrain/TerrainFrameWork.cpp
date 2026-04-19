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

    std::vector<VImagePtr> faceImages(6);

    for (int face = 0; face < 6; ++face)
    {
        uint8_t* data = (uint8_t*)malloc(faceSize * faceSize * 4);

        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                float u = (float)x / (float)(faceSize - 1);
                float v = (float)y / (float)(faceSize - 1);

                Vector3f color;

                if (face == 2) // top
                {
                    color = Vector3f(0.40f, 0.60f, 0.90f) * (1.0f - v)
                          + Vector3f(0.55f, 0.72f, 0.92f) * v;
                }
                else if (face == 3) // bottom
                {
                    color = Vector3f(0.15f, 0.17f, 0.22f);
                }
                else // four sides
                {
                    float t = v;
                    if (t < 0.5f)
                    {
                        float s = t * 2.0f;
                        color = Vector3f(0.50f, 0.65f, 0.85f) * (1.0f - s)
                              + Vector3f(0.70f, 0.82f, 0.95f) * s;
                    }
                    else
                    {
                        float s = (t - 0.5f) * 2.0f;
                        color = Vector3f(0.70f, 0.82f, 0.95f) * (1.0f - s)
                              + Vector3f(0.45f, 0.62f, 0.88f) * s;
                    }
                }

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
static const uint32_t kPatchSize = 33;

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
    camera->SetLens(60.0f, width, height, 1.0f, 2000.0f);

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

    // ---- Terrain (GeoMipMapping with LOD, TerrainComponent) ----
    std::string heightmapPath = GetProjectAssetDir() + "terrain/ps_height_1k.png";
    std::string texturePath   = GetProjectAssetDir() + "terrain/ps_texture_1k.png";

    LOG_INFO("Generating GeoMipMapping terrain (worldSize=%.0f, heightScale=%.1f, patchSize=%u)...",
             kTerrainWorldSize, kTerrainHeightScale, kPatchSize);

    // Create TerrainComponent and initialize from heightmap
    mTerrainComponent = new RenderSystem::TerrainComponent();
    mTerrainComponent->InitFromHeightMap(
        heightmapPath.c_str(), kTerrainWorldSize, kTerrainHeightScale, kPatchSize);

    if (!mTerrainComponent->IsInitialized())
    {
        LOG_WARN("Failed to load heightmap, falling back to procedural terrain");
        mTerrainComponent->InitProcedural(
            kTerrainResolution, kTerrainWorldSize, kTerrainHeightScale, kPatchSize);
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

    // Update terrain LOD based on camera position
    if (mTerrainComponent && mTerrainComponent->IsInitialized())
    {
        RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
        if (camera)
        {
            mathutil::Vector3f camPos = camera->GetPosition();
            mTerrainComponent->GetGeoMipTerrain()->UpdateLOD(camPos);
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
