//
//  AtmosphereFrameWork.cpp
//  atmosphere demo
//

#include "AtmosphereFrameWork.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include "Runtime/RenderSystem/include/Light.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereRenderer.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereConstant.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Vector3.h"

#include <imgui.h>

#include <cmath>

using namespace mathutil;

namespace
{
constexpr float kDemoPi = 3.1415926f;
}

AtmosphereFrameWork::AtmosphereFrameWork(const GNXEngine::WindowProps& props)
    : GNXEngine::AppFrameWork(props)
{
}

void AtmosphereFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

// 启用 ImGui，并按窗口 DPI 缩放重建字体图集
void AtmosphereFrameWork::SetupImGui()
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
    imgui->InvalidateFontAtlas();   // 按新的 DPI 重建字体，保证文字清晰
}

void AtmosphereFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);

    if (!mSceneCreated)
    {
        CreateScene(width, height);
        mSceneCreated = true;
    }
    else
    {
        // 窗口尺寸变化时更新投影（保持宽高比正确）
        RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
        RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
        if (camera)
        {
            camera->SetLens(50.0f, width, height, 0.5f, 1000.0f);
        }
    }
}

void AtmosphereFrameWork::CreateScene(uint32_t width, uint32_t height)
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // ---- 相机（移植自参考实现 GodCamera）----
    RenderSystem::CameraPtr camera = sceneManager->CreateCamera("MainCamera");
    UpdateCamera();
    camera->SetLens(50.0f, width, height, 0.5f, 1000.0f);

    // ---- 太阳（方向光）：方向从地表指向太阳 ----
    RenderSystem::DirectionLight* dirLight = static_cast<RenderSystem::DirectionLight*>(
        sceneManager->CreateLight("sun", RenderSystem::Light::DirectionLight));
    dirLight->setColor(Vector3f(1.0f, 1.0f, 1.0f));
    dirLight->setStrength(Vector3f(1.0f, 1.0f, 1.0f));
    UpdateSun();

    // ---- 大气散射组件 ----
    RenderSystem::SceneNode* atmoNode = sceneManager->GetRootNode()->CreateChildSceneNode("Atmosphere");
    mAtmosphere = atmoNode->AddComponent<RenderSystem::AtmosphereComponent>();
    // 参考 Demo 的 exposure=10 是配合它自己的 1-exp(-L*exposure) 曲线；
    // 本引擎管线末端使用 ACES 曲线（对中间调提升更强），取 5.0 后整体亮度与参考图一致。
    mAtmosphere->SetExposure(5.0f);
    mAtmosphere->SetWhitePoint(Vector3f(1.0f, 1.0f, 1.0f));

    // 场景“额外几何体”（球体 + 地面）：来自参考 Demo 的演示物体。
    // 这里以米为单位给出，再换算成大气单位（1 大气单位 = kLengthUnitInMeters 米），
    // 通过参数传给着色器，避免把这些场景参数写死在 shader 里。
    {
        const float kLengthUnitInMeters = static_cast<float>(RenderSystem::Atmosphere::kLengthUnitInMeters);
        RenderSystem::Atmosphere::AtmosphereSceneGeometry geometry;
        geometry.sphereCenter = Vector3f(0.0f, 0.0f, 1000.0f / kLengthUnitInMeters); // 球心 (0,0,1000m)
        geometry.sphereRadius = 1000.0f / kLengthUnitInMeters;                       // 半径 1000m
        geometry.sphereAlbedo = Vector3f(0.8f, 0.8f, 0.8f);                          // 球体反照率
        geometry.groundAlbedo = Vector3f(0.0f, 0.0f, 0.04f);                         // 地面着色反照率
        mAtmosphere->SetSceneGeometry(geometry);
    }

    mAtmosphere->Initialize(5);   // 散射重数 = 5
    mScatteringOrders = 5;
    mPendingScatteringOrders = 5;

    // ---- 启用 ImGui（UI 优先消费输入，避免点击面板时同时操作场景）----
    SetupImGui();

    LOG_INFO("Atmosphere demo scene created");
}

// 依据 view_zenith / view_azimuth / view_distance 摆放相机
// 与参考实现 GodCamera 的基向量一致：
//   ux = (-sin a, cos a, 0)
//   uy = (-cos z cos a, -cos z sin a, sin z)
//   uz = ( sin z cos a,  sin z sin a, cos z)
// 相机位于 uz * distance，沿 -uz 方向观察，up = uy
void AtmosphereFrameWork::UpdateCamera()
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
    if (!camera)
    {
        return;
    }

    const float cosz = cosf(mViewZenith);
    const float sinz = sinf(mViewZenith);
    const float cosa = cosf(mViewAzimuth);
    const float sina = sinf(mViewAzimuth);

    Vector3f uz(sinz * cosa, sinz * sina, cosz);
    Vector3f uy(-cosz * cosa, -cosz * sina, sinz);

    Vector3f position = uz * mViewDistance;
    Vector3f target = position - uz;

    camera->LookAt(position, target, uy);
}

// 依据 sun_zenith / sun_azimuth 设置太阳方向
void AtmosphereFrameWork::UpdateSun()
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    RenderSystem::Light* light = sceneManager->GetLight("sun");
    if (!light)
    {
        return;
    }

    RenderSystem::DirectionLight* dirLight = static_cast<RenderSystem::DirectionLight*>(light);
    Vector3f sunDir(
        cosf(mSunAzimuth) * sinf(mSunZenith),
        sinf(mSunAzimuth) * sinf(mSunZenith),
        cosf(mSunZenith));

    dirLight->setDirection(sunDir.Normalize());
}

void AtmosphereFrameWork::RenderFrame()
{
    using namespace RenderCore;
    using namespace RenderSystem;

    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;

    // ---- ImGui：构建 UI 并结束帧（绘制由渲染管线的 Present Pass 完成）----
    BuildImGuiPanel();
    ImGui::Render();

    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    sceneManager->Render(nullptr);
}

void AtmosphereFrameWork::OnEvent(GNXEngine::Event& e)
{
    // AppFrameWork::OnEvent 内部会先把事件交给 ImGui；UI 捕获时会标记 e.handled
    GNXEngine::AppFrameWork::OnEvent(e);

    // 事件已被 UI 消费：不再触发 demo 自身的场景快捷键（3D 场景不受影响）
    if (e.handled)
    {
        return;
    }

    GNXEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<GNXEngine::KeyPressedEvent>(GNX_BIND_EVENT_FN(OnKeyPressed));
}

bool AtmosphereFrameWork::OnKeyPressed(GNXEngine::KeyPressedEvent& e)
{
    const float step = 0.02f;
    bool changedSun = false;

    switch (e.GetKeyCode())
    {
    // 方向键：移动太阳
    case GNXEngine::Left:
        mSunAzimuth -= step;
        changedSun = true;
        break;
    case GNXEngine::Right:
        mSunAzimuth += step;
        changedSun = true;
        break;
    case GNXEngine::Up:
        mSunZenith = std::min(kDemoPi, mSunZenith + step);
        changedSun = true;
        break;
    case GNXEngine::Down:
        mSunZenith = std::max(0.0f, mSunZenith - step);
        changedSun = true;
        break;
    // W/S：调整视线俯仰，A/D：调整视线方位
    case GNXEngine::W:
        mViewZenith = std::max(0.0f, mViewZenith - step);
        UpdateCamera();
        break;
    case GNXEngine::S:
        mViewZenith = std::min(kDemoPi * 0.5f, mViewZenith + step);
        UpdateCamera();
        break;
    case GNXEngine::A:
        mViewAzimuth -= step;
        UpdateCamera();
        break;
    case GNXEngine::D:
        mViewAzimuth += step;
        UpdateCamera();
        break;
    default:
        break;
    }

    if (changedSun)
    {
        UpdateSun();
    }

    return false;
}

// ---------------------------------------------------------------------------
// ImGui 调试面板：暴露大气散射的可调参数
// ---------------------------------------------------------------------------
void AtmosphereFrameWork::BuildImGuiPanel()
{
    if (!mAtmosphere)
    {
        return;
    }

    LOG_INFO("[demo] BuildImGuiPanel: ctx=%p", (void*)ImGui::GetCurrentContext());
    ImGuiIO& io = ImGui::GetIO();
    const float unit = static_cast<float>(RenderSystem::Atmosphere::kLengthUnitInMeters);

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.0f, 540.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("预计算大气散射", &mShowPanel))
    {
        ImGui::Text("FPS %.1f (%.2f ms)", io.Framerate,
                    io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
        ImGui::Text("输入捕获: 鼠标[%s] 键盘[%s]",
                    io.WantCaptureMouse ? "UI" : "场景",
                    io.WantCaptureKeyboard ? "UI" : "场景");
        ImGui::Separator();

        // ---- 曝光 ----
        if (ImGui::CollapsingHeader("曝光", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float exposure = mAtmosphere->GetExposure();
            if (ImGui::SliderFloat("exposure", &exposure, 0.1f, 20.0f, "%.2f"))
            {
                mAtmosphere->SetExposure(exposure);
            }
        }

        // ---- 太阳 ----
        if (ImGui::CollapsingHeader("太阳", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool changed = false;
            changed |= ImGui::SliderFloat("sun zenith", &mSunZenith, 0.0f, kDemoPi, "%.3f rad");
            changed |= ImGui::SliderFloat("sun azimuth", &mSunAzimuth, -kDemoPi, kDemoPi, "%.3f rad");
            if (changed)
            {
                UpdateSun();
            }
        }

        // ---- 相机 ----
        if (ImGui::CollapsingHeader("相机", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool changed = false;
            changed |= ImGui::SliderFloat("view zenith", &mViewZenith, 0.0f, kDemoPi * 0.5f, "%.3f rad");
            changed |= ImGui::SliderFloat("view azimuth", &mViewAzimuth, -kDemoPi, kDemoPi, "%.3f rad");
            changed |= ImGui::SliderFloat("view distance", &mViewDistance, 1.0f, 200.0f, "%.1f");
            if (changed)
            {
                UpdateCamera();
            }
        }

        // ---- 散射预计算 ----
        if (ImGui::CollapsingHeader("散射 / LUT 预计算"))
        {
            ImGui::SliderInt("scattering orders", &mPendingScatteringOrders, 1, 8);
            ImGui::TextWrapped("当前生效: %d 重", mScatteringOrders);
            if (ImGui::Button("重新预计算 LUT"))
            {
                mAtmosphere->Initialize((unsigned int)mPendingScatteringOrders);
                mScatteringOrders = mPendingScatteringOrders;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("重新生成 透射率 / 散射 / 辐照度 LUT（会短暂卡顿）");
            }
        }

        // ---- 场景几何体（参考 Demo 的球体与地面）----
        if (ImGui::CollapsingHeader("场景几何体"))
        {
            RenderSystem::Atmosphere::AtmosphereSceneGeometry geometry = mAtmosphere->GetSceneGeometry();

            float centerXY[2] = { geometry.sphereCenter.x * unit, geometry.sphereCenter.y * unit };
            float centerZ = geometry.sphereCenter.z * unit;
            float radiusMeters = geometry.sphereRadius * unit;
            float sphereAlbedo[3] = { geometry.sphereAlbedo.x, geometry.sphereAlbedo.y, geometry.sphereAlbedo.z };
            float groundAlbedo[3] = { geometry.groundAlbedo.x, geometry.groundAlbedo.y, geometry.groundAlbedo.z };

            bool changed = false;
            changed |= ImGui::DragFloat2("球心 XY (m)", centerXY, 10.0f, -5000.0f, 5000.0f);
            changed |= ImGui::DragFloat("球心 Z (m)", &centerZ, 10.0f, 0.0f, 5000.0f);
            changed |= ImGui::DragFloat("球半径 (m)", &radiusMeters, 10.0f, 10.0f, 5000.0f);
            changed |= ImGui::ColorEdit3("球体反照率", sphereAlbedo);
            changed |= ImGui::ColorEdit3("地面反照率", groundAlbedo);

            if (changed)
            {
                geometry.sphereCenter = Vector3f(centerXY[0] / unit, centerXY[1] / unit, centerZ / unit);
                geometry.sphereRadius = radiusMeters / unit;
                geometry.sphereAlbedo = Vector3f(sphereAlbedo[0], sphereAlbedo[1], sphereAlbedo[2]);
                geometry.groundAlbedo = Vector3f(groundAlbedo[0], groundAlbedo[1], groundAlbedo[2]);
                mAtmosphere->SetSceneGeometry(geometry);
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("快捷键: 方向键 = 太阳, W/S/A/D = 视线");
        ImGui::TextDisabled("点击面板时输入不会传给 3D 场景");
    }
    ImGui::End();
}
