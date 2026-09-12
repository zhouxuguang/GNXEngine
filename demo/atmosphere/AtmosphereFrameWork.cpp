//
//  AtmosphereFrameWork.cpp
//  atmosphere demo
//

#include "AtmosphereFrameWork.h"
#include "Runtime/RenderSystem/include/Light.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereRenderer.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereConstant.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/Vector3.h"
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
    mAtmosphere->Initialize(5);   // 散射重数 = 5

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

    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    sceneManager->Render(nullptr);
}

void AtmosphereFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);

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
