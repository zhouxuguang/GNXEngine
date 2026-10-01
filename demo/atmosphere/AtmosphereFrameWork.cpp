//
//  AtmosphereFrameWork.cpp
//  atmosphere demo
//

#include "AtmosphereFrameWork.h"
#include "AtmospherePreset.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/EditorCameraController.h"
#include "Runtime/RenderSystem/include/Light.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereRenderer.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereConstant.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Vector3.h"

#include <imgui.h>

#include <algorithm>
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

// 启用 ImGui；窗口与显示器缩放由 AppFrameWork 每帧同步
void AtmosphereFrameWork::SetupImGui()
{
    SetImGuiEnabled(true);
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
            camera->SetLens(50.0f, width, height, 500.0f, 1000000.0f);
        }
    }
}

void AtmosphereFrameWork::CreateScene(uint32_t width, uint32_t height)
{
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();

    // ---- 相机：使用引擎原生 Y-up 坐标与 EditorCameraController ----
    // 相机由 demo 创建；CreateCamera 会自动挂接引擎轨道相机控制器。
    // 先 GetCamera 再创建：CreateCamera 不去重，重复创建会留下第二个同名相机。
    RenderSystem::CameraPtr camera = sceneManager->GetCamera("MainCamera");
    if (!camera)
    {
        camera = sceneManager->CreateCamera("MainCamera");
    }
    // 参考视角旋转到引擎坐标后的位置。目标固定为地表原点，使控制器首次同步时
    // 获得正确的焦点和 9km 轨道距离，后续拖拽、平移和缩放都不会跳变。
    camera->LookAt(Vector3f(8910.0f, 905.0f, 893.0f),
                   Vector3f(0.0f, 0.0f, 0.0f),
                   Vector3f(0.0f, 1.0f, 0.0f));
    camera->SetLens(50.0f, width, height, 500.0f, 1000000.0f);

    // 参考实现的相机天顶角被夹在 [0, π/2]，从而保证 |camera - earth_center| >= bottom_radius。
    // 这里给轨道相机加同样的约束：焦点在地表，若不限制，往上拖鼠标会让相机沉到地面以下，
    // 那时大气 LUT 查询会退化（rho = sqrt(r²-R²) 被截断为 0），地面会消失并渲染成黑/雾。
    if (auto* orbitController =
            dynamic_cast<RenderSystem::EditorCameraController*>(sceneManager->GetCameraController()))
    {
        orbitController->SetMinCameraY(0.0f);
        orbitController->SetMoveSpeed(10000.0f);
        orbitController->SetMinDistance(100.0f);
        orbitController->SetMaxDistance(100000000.0f);
    }

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

    mAtmosphereParameters = CreateDemoAtmosphereParameters();
    mAtmosphere->SetPlanetCenter(Vector3d(0.0, -mAtmosphereParameters.bottom_radius, 0.0));
    mDemoGeometryUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(48);
    UpdateDemoGeometryUBO();
    mAtmosphere->SetSkyShaderAsset("AtmosphereDemo/Sky");
    mAtmosphere->SetSkyExtraUniformBuffer("AtmosphereDemoCB", mDemoGeometryUBO);
    mAtmosphere->Initialize(mAtmosphereParameters, 5);
    mScatteringOrders = 5;
    mPendingScatteringOrders = 5;

    // ---- 启用 ImGui（UI 优先消费输入，避免点击面板时同时操作场景）----
    SetupImGui();

    LOG_INFO("Atmosphere demo scene created");
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
    // 参考场景是 Z-up；与相机使用相同的保手性旋转
    // (oldX, oldY, oldZ) -> (oldX, oldZ, -oldY)，转换到引擎 Y-up。
    // 这样不仅竖直轴正确，也能保留太阳相对默认视角的正前方方位。
    Vector3f sunDir(
        cosf(mSunAzimuth) * sinf(mSunZenith),
        cosf(mSunZenith),
        -sinf(mSunAzimuth) * sinf(mSunZenith));

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
    if (IsImGuiEnabled())
    {
        BuildImGuiPanel();
        ImGui::Render();
    }

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

    // 相机输入已经由 AppFrameWork 转发给引擎轨道控制器；这里只处理太阳快捷键。
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

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 560.0f), ImGuiCond_FirstUseEver);

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

        // ---- 散射预计算 ----
        if (ImGui::CollapsingHeader("散射 / LUT 预计算"))
        {
            ImGui::SliderInt("scattering orders", &mPendingScatteringOrders, 1, 8);
            ImGui::TextWrapped("当前生效: %d 重", mScatteringOrders);
            if (ImGui::Button("重新预计算 LUT"))
            {
                mAtmosphere->Initialize(mAtmosphereParameters, (unsigned int)mPendingScatteringOrders);
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
            DemoGeometry geometry = mGeometry;

            float centerXZ[2] = { geometry.sphereCenter.x, geometry.sphereCenter.z };
            float centerY = geometry.sphereCenter.y;
            float radiusMeters = geometry.sphereRadius;
            float sphereAlbedo[3] = { geometry.sphereAlbedo.x, geometry.sphereAlbedo.y, geometry.sphereAlbedo.z };
            float groundAlbedo[3] = { geometry.groundAlbedo.x, geometry.groundAlbedo.y, geometry.groundAlbedo.z };

            bool changed = false;
            changed |= ImGui::DragFloat2("球心 XZ (m)", centerXZ, 10.0f, -5000.0f, 5000.0f);
            changed |= ImGui::DragFloat("球心 Y / 高度 (m)", &centerY, 10.0f, 0.0f, 5000.0f);
            changed |= ImGui::DragFloat("球半径 (m)", &radiusMeters, 10.0f, 10.0f, 5000.0f);
            changed |= ImGui::ColorEdit3("球体反照率", sphereAlbedo);
            changed |= ImGui::ColorEdit3("地面反照率", groundAlbedo);

            if (changed)
            {
                geometry.sphereCenter = Vector3f(centerXZ[0], centerY, centerXZ[1]);
                geometry.sphereRadius = radiusMeters;
                geometry.sphereAlbedo = Vector3f(sphereAlbedo[0], sphereAlbedo[1], sphereAlbedo[2]);
                geometry.groundAlbedo = Vector3f(groundAlbedo[0], groundAlbedo[1], groundAlbedo[2]);
                mGeometry = geometry;
                UpdateDemoGeometryUBO();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("坐标系: Y-up（GNXEngine 世界坐标）");
        ImGui::TextDisabled("快捷键: 方向键 = 太阳, W/A/S/D/Q/E = 平移相机");
        ImGui::TextDisabled("鼠标: 左/右键拖拽 = 轨道旋转, 中键 = 平移, 滚轮 = 缩放");
        ImGui::TextDisabled("点击面板时输入不会传给 3D 场景");
    }
    ImGui::End();
}

void AtmosphereFrameWork::UpdateDemoGeometryUBO()
{
    if (!mDemoGeometryUBO) return;
    struct alignas(16) DemoCB
    {
        simd_float4 sphereCenterRadius;
        simd_float4 sphereAlbedo;
        simd_float4 groundAlbedo;
    } data;
    static_assert(sizeof(DemoCB) == 48);
    const Vector3d center = mAtmosphere->GetPlanetCenter();
    data.sphereCenterRadius = {
        static_cast<float>(static_cast<double>(mGeometry.sphereCenter.x) - center.x),
        static_cast<float>(static_cast<double>(mGeometry.sphereCenter.y) - center.y),
        static_cast<float>(static_cast<double>(mGeometry.sphereCenter.z) - center.z),
        mGeometry.sphereRadius};
    data.sphereAlbedo = {mGeometry.sphereAlbedo.x, mGeometry.sphereAlbedo.y,
                                        mGeometry.sphereAlbedo.z, 0.0f};
    data.groundAlbedo = {mGeometry.groundAlbedo.x, mGeometry.groundAlbedo.y,
                                        mGeometry.groundAlbedo.z, 0.0f};
    mDemoGeometryUBO->SetData(&data, 0, sizeof(data));
}
