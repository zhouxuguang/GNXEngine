//
//  AtmosphereFrameWork.h
//  atmosphere demo
//
//  预计算大气散射（Precomputed Atmospheric Scattering）演示
//  使用 GNXEngine 原生 Y-up 世界坐标和轨道相机控制器。
//

#ifndef AtmosphereFrameWork_h
#define AtmosphereFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/Atmosphere/AtmosphereComponent.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"

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
    void UpdateSun();
    void SetupImGui();
    void BuildImGuiPanel();

    bool OnKeyPressed(GNXEngine::KeyPressedEvent& e);
private:
    bool mSceneCreated = false;

    // 太阳参数
    float mSunZenith = 1.3f;
    float mSunAzimuth = 2.9f;

    // ImGui 面板参数
    bool  mShowPanel = true;
    int   mScatteringOrders = 5;                       // 当前生效的散射重数
    int   mPendingScatteringOrders = 5;                // UI 上待应用的值
    float mGroundAlbedoBlue = 0.04f;                   // 地面着色反照率（蓝通道）

    struct DemoGeometry
    {
        mathutil::Vector3f sphereCenter{0.0f, 1000.0f, 0.0f};
        float sphereRadius = 1000.0f;
        mathutil::Vector3f sphereAlbedo{0.8f, 0.8f, 0.8f};
        mathutil::Vector3f groundAlbedo{0.0f, 0.0f, 0.04f};
    } mGeometry;
    RenderSystem::Atmosphere::AtmosphereParameters mAtmosphereParameters{};
    RenderCore::UniformBufferPtr mDemoGeometryUBO;
    void UpdateDemoGeometryUBO();
    RenderSystem::AtmosphereComponent* mAtmosphere = nullptr;
};

#endif /* AtmosphereFrameWork_h */
