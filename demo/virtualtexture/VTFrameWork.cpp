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

    auto fileSource = std::make_shared<FileVirtualTextureDataSource>("assets/virtualtexture/terrain");
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
    LOG_INFO("Mip levels: %u, Pinned: %u, Uploads/frame: %u",
             cfg.mipLevels, cfg.pinnedMipLevels, cfg.uploadsPerFrame);
    LOG_INFO("PageTable: %llu bytes, Atlas: %.2f MB",
             (unsigned long long)EstimatePageTableMemory(cfg),
             EstimateAtlasMemory(cfg) / (1024.0 * 1024.0));

    // 创建平面网格（VT 材质测试）
    auto CreatePlaneMesh = [](float width, float depth, int xdivs, int zdivs) -> MeshPtr
    {
        int nPoints = (xdivs + 1) * (zdivs + 1);
        std::vector<Vector3f> positions(nPoints);
        std::vector<Vector3f> normals(nPoints, Vector3f(0, 1, 0));
        std::vector<Vector4f> tangents(nPoints, Vector4f(1, 0, 0, 1));
        std::vector<Vector2f> texcoords(nPoints);
        std::vector<uint32_t> indices(xdivs * zdivs * 6);
        
        float halfW = width * 0.5f;
        float halfD = depth * 0.5f;
        
        int vidx = 0;
        for (int iz = 0; iz <= zdivs; ++iz)
        {
            float z = halfD - (float)iz / zdivs * depth;
            for (int ix = 0; ix <= xdivs; ++ix)
            {
                float x = -halfW + (float)ix / xdivs * width;
                positions[vidx] = Vector3f(x, 0.0f, z);
                texcoords[vidx] = Vector2f((float)ix / xdivs * 8.0f, (float)iz / zdivs * 8.0f);
                ++vidx;
            }
        }
        
        int iidx = 0;
        for (int iz = 0; iz < zdivs; ++iz)
        {
            int rowStart     = iz * (xdivs + 1);
            int nextRowStart = (iz + 1) * (xdivs + 1);
            for (int ix = 0; ix < xdivs; ++ix)
            {
                indices[iidx++] = rowStart + ix;
                indices[iidx++] = nextRowStart + ix;
                indices[iidx++] = nextRowStart + ix + 1;
                indices[iidx++] = rowStart + ix;
                indices[iidx++] = nextRowStart + ix + 1;
                indices[iidx++] = rowStart + ix + 1;
            }
        }
        
        auto mesh = std::make_shared<Mesh>();
        mesh->SetPositions(positions.data(), nPoints);
        mesh->SetNormals(normals.data(), nPoints);
        mesh->SetTangents(tangents.data(), nPoints);
        mesh->SetUv(0, texcoords.data(), nPoints);
        mesh->SetIndices(indices.data(), (int)indices.size());
        
        SubMeshInfo subMeshInfo;
        subMeshInfo.firstIndex  = 0;
        subMeshInfo.indexCount  = (int)indices.size();
        subMeshInfo.vertexCount = nPoints;
        subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
        mesh->AddSubMeshInfo(subMeshInfo);
        
        mesh->SetUpBuffer();
        return mesh;
    };

    // 创建 VT 材质
    MaterialPtr vtMaterial = std::make_shared<Material>();
    vtMaterial->SetMaterialType(Material::MaterialType::VirtualTexturePBR);
    vtMaterial->SetTexture("normalTexture", nullptr);
    vtMaterial->SetTexture("roughnessTexture", nullptr);
    vtMaterial->SetTexture("emissiveTexture", nullptr);
    vtMaterial->SetTexture("ambientTexture", nullptr);

    // 创建场景节点并挂载网格
    MeshPtr planeMesh = CreatePlaneMesh(20.0f, 20.0f, 2, 2);
    
    SceneNode* vtNode = sceneManager->GetRootNode()->CreateChildSceneNode("VTPlane");
    MeshRenderer* meshRenderer = new MeshRenderer();
    meshRenderer->SetSharedMesh(planeMesh);
    meshRenderer->AddMaterial(vtMaterial);
    vtNode->AddComponent(meshRenderer);
    
    LOG_INFO("VT demo scene setup complete.");
}

void VTFrameWork::Resize(uint32_t width, uint32_t height)
{
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
