//
//  VTFrameWork.cpp
//  virtualtexture
//
//  Virtual Texture Demo
//  演示 VT 系统：反馈 pass → page 请求 → 异步加载 → atlas/page table 更新，
//  并用 ImGui 移植参考 demo 的 atlas minimap overlay，实时查看 page 布局变化。
//

#include "VTFrameWork.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/Light.h"
#include "Runtime/RenderSystem/include/SceneNode.h"
#include "Runtime/RenderSystem/include/Material.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include "Runtime/RenderSystem/include/VirtualTexture/FileVirtualTextureDataSource.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include "Runtime/AssetManager/include/MeshMessageUtil.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Quaternion.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/FileUtil.h"

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

using namespace mathutil;

namespace
{
// 加载引擎自有网格资产（.meshasset：AssetFileHeader + MeshMessage pb），与 pbr demo 一致。
RenderSystem::MeshPtr LoadMeshAsset(const std::string& filePath)
{
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

    // 创建 GPU 顶点/索引缓冲
    mesh->SetUpBuffer();
    LOG_INFO("Loaded mesh asset: %s (%u verts, %zu indices, %u submesh)",
             filePath.c_str(), mesh->GetVertexCount(), mesh->GetIndices().size(), mesh->GetSubMeshCount());
    return mesh;
}
} // namespace

VTFrameWork::VTFrameWork(const GNXEngine::WindowProps& props)
    : AppFrameWork(props)
{
}

void VTFrameWork::SetupImGui()
{
    SetImGuiEnabled(true);

    RenderSystem::ImGuiRendererPtr imgui = GetImGui();
    if (!imgui)
    {
        return;
    }

    float dpiScale = 1.0f;
    if (GNXEngine::RenderWindowPtr window = GNXEngine::GetRenderWindow())
    {
        dpiScale = window->GetDPIScale();
    }
    imgui->SetDPIScale(dpiScale);
}

void VTFrameWork::UpdateCameraLens(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    RenderSystem::CameraPtr camera = RenderSystem::SceneManager::GetInstance()->GetCamera("MainCamera");
    if (camera)
    {
        camera->SetLens(60.0f, (float)width, (float)height, 1.0f, 5000.0f);
    }
}

void VTFrameWork::Initlize()
{
    if (GNXEngine::RenderWindowPtr window = GNXEngine::GetRenderWindow())
    {
        mWindowWidth = window->GetWidth();
        mWindowHeight = window->GetHeight();
    }

    SetupImGui();
    SetupScene();

    UpdateCameraLens(mWindowWidth, mWindowHeight);
}

void VTFrameWork::SetupScene()
{
    using namespace RenderSystem;

    SceneManager* sceneManager = SceneManager::GetInstance();

    // ── Camera：参考模型和 GNXEngine 都使用右手、Y-up 世界坐标，模型无需轴旋转。──
    // 屏幕坐标则交给引擎 Camera/ImGui 转换：3D 为左下 NDC，UI 为左上像素坐标。
    // 模型缩放后约 600x600，相机抬高俯视整片地形。
    // 先 GetCamera 再创建：CreateCamera 不去重，直接调用会得到第二个同名相机，
    // 而 GetCamera 永远返回第一个，后面的摆位就落到没人用的相机上。
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    if (cameraPtr)
    {
        cameraPtr->LookAt(Vector3f(0.0f, 250.0f, 600.0f),
                          Vector3f(0.0f, 20.0f, 0.0f),
                          Vector3f(0.0f, 1.0f, 0.0f));
        cameraPtr->SetLens(60.0f, (float)mWindowWidth, (float)mWindowHeight, 1.0f, 5000.0f);
    }

    // ── Direction light ──
    // 注意：getDirection() 的语义是 "表面指向光源"；必须设置方向，
    // 否则延迟光照里 normalize(0,0,0) 会产生 NaN，地形全黑。
    DirectionLight* dirLight = static_cast<DirectionLight*>(
        sceneManager->CreateLight("MainLight", Light::DirectionLight));
    if (dirLight)
    {
        dirLight->setColor(Vector3f(1.0f, 0.98f, 0.95f));
        dirLight->setStrength(Vector3f(3.0f, 3.0f, 3.0f));
        dirLight->setDirection(Vector3f(0.45f, 0.70f, 0.55f).Normalize());
    }

    // ── Virtual Texture Manager ──
    // 与参考 demo / 离线切图资产 (assets/pages) 完全对齐：
    //   虚拟尺寸 8192，page 512 → mip0 为 16x16 网格，共 5 级 mip (16/8/4/2/1)
    //   atlas 8x8 slot，每个 slot 512 + 2*2 padding = 516
    VirtualTextureConfig vtConfig;
    vtConfig.virtualWidth    = 8192;
    vtConfig.virtualHeight   = 8192;
    vtConfig.pageSize        = 512;
    vtConfig.pageBorder      = 2;
    vtConfig.atlasSlotsX     = 8;
    vtConfig.atlasSlotsY     = 8;
    vtConfig.pinnedMipLevels = 2;   // 常驻最粗糙的 2 级 mip，作为回退兜底
    vtConfig.uploadsPerFrame = mUploadsPerFrame;

    const std::string tilePath = GetProjectAssetDir() + "vt/pages";
    auto fileSource = std::make_shared<FileVirtualTextureDataSource>(tilePath, ".png");

    Vector2i viewSize((int)mWindowWidth, (int)mWindowHeight);
    mVTIndex = sceneManager->AddVTManager(vtConfig, fileSource, viewSize, 16);
    mVTManager = sceneManager->GetVTManager(mVTIndex);

    if (mVTManager)
    {
        mAtlasTexture = mVTManager->GetAtlasTexture();
    }

    // Log VT info
    if (mVTManager)
    {
        const auto& cfg = mVTManager->GetConfig();
        LOG_INFO("=== Virtual Texture Demo ===");
        LOG_INFO("VT: %ux%u, Page: %u, Atlas: %ux%u (%u x %u slots)",
                 cfg.virtualWidth, cfg.virtualHeight,
                 cfg.pageSize,
                 cfg.atlasWidth, cfg.atlasHeight,
                 cfg.atlasSlotsX, cfg.atlasSlotsY);
        LOG_INFO("Mip levels: %u, Pinned: %u, Uploads/frame: %u", cfg.mipLevels, cfg.pinnedMipLevels, cfg.uploadsPerFrame);
        LOG_INFO("PageTable: %llu bytes, Atlas: %.2f MB", (unsigned long long)EstimatePageTableMemory(cfg),
                 EstimateAtlasMemory(cfg) / (1024.0 * 1024.0));
        LOG_INFO("Tile source: %s (0_0_0.png ...)", tilePath.c_str());
    }

    // ── 加载模型（.meshasset），材质设为 VirtualTexturePBR ──
    {
        const std::string modelPath = GetProjectAssetDir() + "vt/snowy_mountain.meshasset";

        RenderSystem::MeshPtr mesh = LoadMeshAsset(modelPath);
        if (!mesh)
        {
            LOG_ERROR("Failed to load VT model asset: %s", modelPath.c_str());
        }
        else
        {
            Transform transform;
            transform.position = Vector3f(0.0f, 0.0f, 0.0f);
            transform.rotation = Quaternionf();
            transform.scale    = Vector3f(30.0f, 30.0f, 30.0f);  // 与参考 demo 一致

            SceneNode* modelNode = sceneManager->GetRootNode()->CreateChildSceneNode(
                "VTModel", transform.position, transform.rotation, transform.scale);

            MeshRenderer* meshRender = modelNode->AddComponent<MeshRenderer>();
            meshRender->SetSharedMesh(mesh);

            // 非 base color 的贴图用 1x1 常量纹理兜底（模型本身不带贴图）
            RCTexture2DPtr normalTex   = ImageTextureUtil::CreateNormalTexture();
            RCTexture2DPtr roughTex    = ImageTextureUtil::CreateDiffuseTexture(0.0f, 0.85f, 0.0f); // G=rough, B=metal
            RCTexture2DPtr ambientTex  = ImageTextureUtil::CreateDiffuseTexture(1.0f, 1.0f, 1.0f);   // AO=1
            RCTexture2DPtr emissiveTex = ImageTextureUtil::CreateEmmisveTexture();                    // 无自发光

            // 单材质即可：多 submesh 共用该材质
            MaterialPtr mat = std::make_shared<Material>();
            mat->SetName("VTModel_Material");
            mat->SetMaterialType(Material::MaterialType::VirtualTexturePBR);
            mat->SetTexture("normalTexture", normalTex);
            mat->SetTexture("roughnessTexture", roughTex);
            mat->SetTexture("ambientTexture", ambientTex);
            mat->SetTexture("emissiveTexture", emissiveTex);
            meshRender->AddMaterial(mat);

            LOG_INFO("Loaded VT model asset: %s (%u submesh), material = VirtualTexturePBR",
                     modelPath.c_str(), mesh->GetSubMeshCount());
        }
    }

    LOG_INFO("VT demo scene setup complete.");
}

void VTFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);

    mWindowWidth = width;
    mWindowHeight = height;
    UpdateCameraLens(width, height);
    if (mVTManager)
    {
        mVTManager->Resize(Vector2i((int)width, (int)height));
    }
}

void VTFrameWork::RenderFrame()
{
    using namespace RenderSystem;

    SceneManager* sceneManager = SceneManager::GetInstance();

    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    if (deltaTime <= 0.0f || deltaTime > 0.5f)
    {
        deltaTime = 1.0f / 60.0f;
    }

    // ImGui：构建 UI 并结束帧（绘制由 Present Pass 完成）
    if (IsImGuiEnabled())
    {
        BuildImGuiPanel();
        ImGui::Render();
    }

    // VT Tick 由 SceneManager::Update() 内部统一处理
    sceneManager->Update(deltaTime);
    sceneManager->Render(nullptr);
}

void VTFrameWork::BuildImGuiPanel()
{
    if (!mShowPanel)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(370.0f, 470.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("虚拟纹理 (Virtual Texture)", &mShowPanel))
    {
        ImGui::Text("FPS %.1f (%.2f ms)", io.Framerate,
                    io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
        ImGui::Separator();

        if (mVTManager)
        {
            const auto& cfg = mVTManager->GetConfig();
            ImGui::Text("虚拟纹理: %u x %u", cfg.virtualWidth, cfg.virtualHeight);
            ImGui::Text("Page: %u px  Border: %u  Slot: %u px", cfg.pageSize, cfg.pageBorder, cfg.slotSize);
            ImGui::Text("Atlas: %u x %u (%u x %u slots)", cfg.atlasWidth, cfg.atlasHeight,
                        cfg.atlasSlotsX, cfg.atlasSlotsY);
            ImGui::Text("Mip 层级: %u  常驻层级: %u", cfg.mipLevels, cfg.pinnedMipLevels);
            ImGui::Separator();

            // 实时反映 page streaming 状态
            ImGui::Text("常驻 Page: %u / %u",
                        mVTManager->GetResidentPageCount(),
                        mVTManager->GetAtlasSlotCapacity());
            ImGui::Text("加载中: %u   待加载: %u",
                        mVTManager->GetPendingLoadCount(),
                        mVTManager->GetPendingRequestCount());
            ImGui::Text("本帧反馈: %u   累计上传: %llu",
                        mVTManager->GetLastFeedbackPageCount(),
                        (unsigned long long)mVTManager->GetTotalUploadedPageCount());
            if (mVTManager->GetFailedPageLoadCount() > 0)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.25f, 1.0f), "加载失败: %llu",
                                   (unsigned long long)mVTManager->GetFailedPageLoadCount());
            }

            int uploads = (int)mUploadsPerFrame;
            if (ImGui::SliderInt("每帧上传数", &uploads, 1, 32))
            {
                mUploadsPerFrame = (uint32_t)uploads;
                mVTManager->SetUploadsPerFrame(mUploadsPerFrame);
            }
        }

        ImGui::Separator();
        ImGui::Checkbox("显示物理 Atlas 预览", &mShowAtlasPreview);
        if (mShowAtlasPreview)
        {
            ImGui::SliderFloat("预览尺寸", &mAtlasPreviewSize, 128.0f, 400.0f, "%.0f px");
        }

        ImGui::Separator();
        ImGui::TextWrapped("操作: 左键拖拽旋转 / 中键平移 / 滚轮缩放 / WASD 移动");
        ImGui::TextWrapped("提示: 移动相机时 page 按需流式加载，观察常驻 Page 数与 Atlas 预览的变化。");
    }
    ImGui::End();

    // 参考 demo 的 overlay：把物理 atlas 作为 minimap 显示，直观看到 page 的分配/淘汰。
    // 默认放在右下角，避免遮挡地形。
    if (mShowAtlasPreview && mAtlasTexture)
    {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const ImVec2 previewExtent(mAtlasPreviewSize + 24.0f, mAtlasPreviewSize + 48.0f);
        ImGui::SetNextWindowPos(ImVec2(display.x - previewExtent.x - 12.0f,
                                       display.y - previewExtent.y - 12.0f),
                                ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(previewExtent, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("物理 Atlas (page 布局)"))
        {
            ImGui::Image((ImTextureID)(uintptr_t)mAtlasTexture.get(),
                         ImVec2(mAtlasPreviewSize, mAtlasPreviewSize));
            ImGui::TextDisabled("%u x %u px", mAtlasTexture->GetWidth(), mAtlasTexture->GetHeight());
        }
        ImGui::End();
    }
}

