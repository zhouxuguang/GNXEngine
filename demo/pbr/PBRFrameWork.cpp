//
//  PBRFrameWork.cpp
//  pbr
//
//  PBR Demo - Demonstrates Physically Based Rendering with the asset pipeline:
//  1. Material textures are KTX textures packaged as .texture assets, loaded at runtime
//  2. Environment lighting (skybox / IBL irradiance + prefilter + BRDF LUT) uses the
//     existing 1.hdr baking outputs (1_cubemap.ktx / 1_irradiance.ktx / 1_prefilter.ktx,
//     BC6H_UFLOAT; Metal 后端映射到 BC6H_RGBUfloat 后可直接显示)
//  3. DamagedHelmet mesh is packaged as .meshasset (custom pb MeshMessage), loaded at runtime
//  4. IBL evaluated in DeferredLighting.shader (diffuse + specular + BRDF LUT)
//

#include "PBRFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include "Runtime/RenderSystem/include/TextureSlot.h"
#include "Runtime/RenderSystem/include/SkyBox.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/BaseLib/include/FileUtil.h"
#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include "Runtime/AssetManager/include/MeshMessageUtil.h"
#include <algorithm>
#include <cmath>

using namespace mathutil;

namespace
{
const char* kAssetRoot = "pbr/";

// ============================================================
// Helper: 加载 .meshasset（AssetFileHeader + MeshMessage pb）为 RenderSystem::Mesh
// ============================================================
RenderSystem::MeshPtr LoadMeshAsset(const std::string& filePath)
{
    // 读取文件（桌面：文件系统；移动端：包内资源由 AssetManager 处理）
    std::vector<uint8_t> fileData;
#if GNX_OS_IOS || GNX_OS_ANDROID
    if (!AssetManager::AssetManager::LoadResource(filePath, fileData))
    {
        LOG_ERROR("LoadMeshAsset: cannot load %s", filePath.c_str());
        return nullptr;
    }
#else
    fileData = baselib::FileUtil::ReadBinaryFile(filePath);
    if (fileData.empty())
    {
        LOG_ERROR("LoadMeshAsset: cannot read %s", filePath.c_str());
        return nullptr;
    }
#endif

    const size_t kHeaderSize = sizeof(AssetManager::AssetFileHeader);
    if (fileData.size() <= kHeaderSize)
    {
        LOG_ERROR("LoadMeshAsset: file too small: %s", filePath.c_str());
        return nullptr;
    }

    RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();
    if (!AssetManager::MeshMessageUtil::DecodeMeshMessage(fileData.data() + kHeaderSize,
                                                          (uint32_t)(fileData.size() - kHeaderSize),
                                                          mesh.get()))
    {
        LOG_ERROR("LoadMeshAsset: pb decode failed: %s", filePath.c_str());
        return nullptr;
    }

    // 创建 GPU 顶点/索引缓冲（渲染所需）
    mesh->SetUpBuffer();
    LOG_INFO("Loaded mesh asset: %s (%u verts, %zu indices, %u submesh)",
             filePath.c_str(), mesh->GetVertexCount(), mesh->GetIndices().size(), mesh->GetSubMeshCount());
    return mesh;
}

// ============================================================
// Helper: 生成纯色/占位 2D 纹理（回退用）
// ============================================================
RenderCore::RCTexture2DPtr CreateSolidTexture2D(float r, float g, float b, float a = 1.0f)
{
    uint8_t* pData = (uint8_t*)malloc(4);
    pData[0] = (uint8_t)(r * 255.0f);
    pData[1] = (uint8_t)(g * 255.0f);
    pData[2] = (uint8_t)(b * 255.0f);
    pData[3] = (uint8_t)(a * 255.0f);
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(imagecodec::FORMAT_RGBA8, 1, 1, pData, free);

    RenderCore::TextureDesc desc = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    auto tex = RenderCore::GetRenderDevice()->CreateTexture2D(
        desc.format, RenderCore::TextureUsage::TextureUsageShaderRead, 1, 1, 1);
    RenderCore::Rect2D rect(0, 0, 1, 1);
    tex->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return tex;
}

// ============================================================
// Helper: 默认采样器（线性过滤，repeat）
// ============================================================
RenderCore::SamplerDesc DefaultSamplerDesc()
{
    return RenderCore::SamplerDesc(
        RenderCore::MAG_LINEAR,
        RenderCore::MIN_LINEAR,
        RenderCore::REPEAT,
        RenderCore::REPEAT
    );
}
} // namespace

// ============================================================
// Implementation
// ============================================================

PBRFrameWork::PBRFrameWork(const GNXEngine::WindowProps& props) 
    : GNXEngine::AppFrameWork(props)
{
}

void PBRFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void PBRFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // 更新相机投影（保持响应窗口变化）
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    cameraPtr->SetLens(60, width, height, 0.1f, 100.0f);

    // 首次创建场景（避免 Resize 事件重复创建）
    if (!mSceneCreated)
    {
        CreateScene(width, height);
        mSceneCreated = true;
    }
}

void PBRFrameWork::CreateScene(uint32_t width, uint32_t height)
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // ---- Camera（保持原默认视角） ----
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    cameraPtr->LookAt(
        Vector3f(0.0f, 2.5f, 6.0f),
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f)
    );
    cameraPtr->SetLens(60, width, height, 0.1f, 100.0f);

    // ---- 主方向光（平行光 key light） ----
    RenderSystem::DirectionLight* dirLight = static_cast<RenderSystem::DirectionLight*>(
        sceneManager->CreateLight("sun", RenderSystem::Light::DirectionLight));
    dirLight->setColor(Vector3f(1.0f, 0.95f, 0.9f));
    dirLight->setDirection(Vector3f(-0.6f, -0.7f, -0.4f).Normalize());
    dirLight->setStrength(Vector3f(3.0f, 3.0f, 3.0f));

    // ---- IBL 环境贴图（1.hdr 烘焙产物）+ 天空盒 ----
    CreateIBL();
    CreateSkybox();

    // ---- DamagedHelmet：meshasset + 全 PBR 纹理资产 ----
    CreateHelmet();
}

// ============================================================
// IBL：从 1.hdr 对应的烘焙产物（原生 .ktx, BC6H）加载 irradiance / prefilter / brdfLUT
// ============================================================
void PBRFrameWork::CreateIBL()
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    std::string helmetDir = GetProjectAssetDir() + std::string(kAssetRoot) + "DamagedHelmet/";

    // 漫反射辐照度 cubemap（1_irradiance.ktx, BC6H_UFLOAT）
    mIrradianceMap = RenderSystem::ImageTextureUtil::LoadKTXCubemapTexture(
        (helmetDir + "1_irradiance.ktx").c_str());
    if (!mIrradianceMap)
    {
        LOG_WARN("Failed to load irradiance cubemap: %s1_irradiance.ktx", helmetDir.c_str());
    }

    // 预过滤镜面 cubemap（1_prefilter.ktx, BC6H_UFLOAT）
    mPrefilteredMap = RenderSystem::ImageTextureUtil::LoadKTXCubemapTexture(
        (helmetDir + "1_prefilter.ktx").c_str());
    if (!mPrefilteredMap)
    {
        LOG_WARN("Failed to load prefiltered cubemap: %s1_prefilter.ktx", helmetDir.c_str());
    }

    // BRDF LUT（顶层 pbr/brdfLUT.ktx, RG16F）
    std::string brdfPath = GetProjectAssetDir() + std::string(kAssetRoot) + "brdfLUT.ktx";
    mBRDFLUT = RenderSystem::ImageTextureUtil::LoadKTXTexture(brdfPath.c_str());
    if (!mBRDFLUT)
    {
        LOG_WARN("Failed to load BRDF LUT: %s; falling back to runtime generation", brdfPath.c_str());
        mBRDFLUT = RenderSystem::ImageTextureUtil::CreateBRDFLUTTexture(512, 1024);
    }

    sceneManager->SetIBLTextures(mIrradianceMap, mPrefilteredMap, mBRDFLUT);
    LOG_INFO("IBL textures bound: irradiance=%s prefiltered=%s brdf=%s",
             mIrradianceMap ? "OK" : "NULL",
             mPrefilteredMap ? "OK" : "NULL",
             mBRDFLUT ? "OK" : "NULL");
}

// ============================================================
// 天空盒：从 1.hdr 对应的环境 cubemap（1_cubemap.ktx, BC6H_UFLOAT）创建
// ============================================================
void PBRFrameWork::CreateSkybox()
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    std::string cubemapPath = GetProjectAssetDir() + std::string(kAssetRoot) + "DamagedHelmet/1_cubemap.ktx";

    RenderCore::RCTextureCubePtr cubemap =
        RenderSystem::ImageTextureUtil::LoadKTXCubemapTexture(cubemapPath.c_str());
    if (!cubemap)
    {
        LOG_WARN("Failed to load skybox cubemap: %s", cubemapPath.c_str());
        return;
    }

    mSkyBox = RenderSystem::SkyBox::createFromTexture(RenderCore::GetRenderDevice(), cubemap);
    if (mSkyBox)
    {
        sceneManager->GetSkyBox()->AttachSkyBoxObject(mSkyBox);
        LOG_INFO("Skybox created from cubemap: %s", cubemapPath.c_str());
    }
}

// ============================================================
// DamagedHelmet：meshasset + 5 张纹理资产
// ============================================================
void PBRFrameWork::CreateHelmet()
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    std::string base = GetProjectAssetDir() + std::string(kAssetRoot) + "DamagedHelmet/";

    // 从 .meshasset 加载网格
    RenderSystem::MeshPtr helmetMesh = LoadMeshAsset(base + "DamagedHelmet.meshasset");
    if (!helmetMesh)
    {
        LOG_ERROR("Failed to load helmet mesh asset");
        return;
    }

    // 从 .texture 资产加载 PBR 纹理
    RenderCore::RCTexture2DPtr albedo    = RenderSystem::ImageTextureUtil::LoadTextureAsset2D(base + "Default_albedo.texture");
    RenderCore::RCTexture2DPtr normal    = RenderSystem::ImageTextureUtil::LoadTextureAsset2D(base + "Default_normal.texture");
    RenderCore::RCTexture2DPtr metalRough= RenderSystem::ImageTextureUtil::LoadTextureAsset2D(base + "Default_metalRoughness.texture");
    RenderCore::RCTexture2DPtr ao        = RenderSystem::ImageTextureUtil::LoadTextureAsset2D(base + "Default_AO.texture");
    RenderCore::RCTexture2DPtr emissive  = RenderSystem::ImageTextureUtil::LoadTextureAsset2D(base + "Default_emissive.texture");

    if (!albedo)     albedo     = CreateSolidTexture2D(0.7f, 0.7f, 0.7f);
    if (!normal)     normal     = RenderSystem::ImageTextureUtil::CreateNormalTexture();
    if (!metalRough) metalRough = RenderSystem::ImageTextureUtil::CreateMetalRoughTexture();
    if (!ao)         ao         = RenderSystem::ImageTextureUtil::CreateAOTexture();
    if (!emissive)   emissive   = CreateSolidTexture2D(0.0f, 0.0f, 0.0f);

    RenderCore::SamplerDesc samp = DefaultSamplerDesc();

    // 材质：纹理槽命名与 MeshDrawUtil::DrawMeshBasePass / GBufferPBR.shader 约定一致
    RenderSystem::MaterialPtr mat = std::make_shared<RenderSystem::Material>();
    mat->SetName("DamagedHelmet_Material");
    mat->SetMaterialType(RenderSystem::Material::MaterialType::PBR);
    mat->SetTexture("diffuseTexture", albedo, samp);
    mat->SetTexture("normalTexture", normal, samp);
    mat->SetTexture("roughnessTexture", metalRough, samp);
    mat->SetTexture("ambientTexture", ao, samp);
    mat->SetTexture("emissiveTexture", emissive, samp);

    // 创建节点（meshasset 几何 + 资产材质）
    // 位姿保持与原有 glTF 加载一致：scale 1.8、绕 Y 轴 -45°
    Matrix4x4f modelMatrix = Matrix4x4f::CreateScale(1.8f, 1.8f, 1.8f) *
                             Matrix4x4f::CreateRotation(0, 1, 0, -45.0f);
    RenderSystem::Transform transform;
    transform.TransformFromMat4(modelMatrix);

    RenderSystem::SceneNode* node = sceneManager->GetRootNode()->CreateChildSceneNode(
        "DamagedHelmet", transform.position, transform.rotation, transform.scale);
    RenderSystem::MeshRenderer* renderer = node->AddComponent<RenderSystem::MeshRenderer>();
    renderer->SetSharedMesh(helmetMesh);
    renderer->AddMaterial(mat);

    LOG_INFO("Helmet added (meshasset + 5 texture assets)");
}

void PBRFrameWork::RenderFrame()
{
    using namespace RenderCore;
    using namespace RenderSystem;
    
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    
    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    
    // Render via DeferredSceneRenderer (G-Buffer → Deferred Lighting pipeline)
    sceneManager->Render(nullptr);
}

void PBRFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);
    
    GNXEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>(GNX_BIND_EVENT_FN(OnKeyUp));
}

bool PBRFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
