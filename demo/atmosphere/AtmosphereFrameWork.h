//
//  AtmosphereFrameWork.h
//  atmosphere demo
//
//  预计算大气散射（Precomputed Atmospheric Scattering）演示
//  相机 / 太阳参数移植自参考实现 Atmosphere（ZeusYang）：
//    view_zenith=1.47rad, view_azimuth=-0.1rad, view_distance=9000m
//    sun_zenith=1.3rad,  sun_azimuth=2.9rad,  exposure=10
//

#ifndef AtmosphereFrameWork_h
#define AtmosphereFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereComponent.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Vector3.h"

class AtmosphereFrameWork : public GNXEngine::AppFrameWork
{
public:
    AtmosphereFrameWork(const GNXEngine::WindowProps& props);

    virtual void Initlize() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void RenderFrame() override;
    virtual void OnEvent(GNXEngine::Event& e) override;

private:
    void CreateScene(uint32_t width, uint32_t height);
    void UpdateCamera();
    void UpdateSun();
    void SetupImGui();
    void BuildImGuiPanel();

    bool OnKeyPressed(GNXEngine::KeyPressedEvent& e);

private:
    bool mSceneCreated = false;

    // 视角参数（移植自参考实现 GodCamera，view_zenith=1.47）
    float mViewZenith = 1.47f;      // 视线天顶角（弧度）
    float mViewAzimuth = -0.1f;     // 视线方位角（弧度）
    float mViewDistance = 9.0f;     // 相机到原点的距离（大气单位 = 米/1000）

    // 太阳参数
    float mSunZenith = 1.3f;
    float mSunAzimuth = 2.9f;

    // ImGui 面板参数
    bool  mShowPanel = true;
    int   mScatteringOrders = 5;                       // 当前生效的散射重数
    int   mPendingScatteringOrders = 5;                // UI 上待应用的值
    float mGroundAlbedoBlue = 0.04f;                   // 地面着色反照率（蓝通道）
    bool  mDirtySceneGeometry = false;                 // 场景几何参数被 UI 修改

    RenderSystem::AtmosphereComponent* mAtmosphere = nullptr;
};

#endif /* AtmosphereFrameWork_h */
