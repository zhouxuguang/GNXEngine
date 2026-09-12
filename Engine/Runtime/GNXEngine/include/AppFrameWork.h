#ifndef GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF
#define GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/UI/ImGuiRenderer.h"

NAMESPACE_GNXENGINE_BEGIN

// 应用程序框架类，子类继承，需要重写Initlize、Resize和RenderFrame方法
class GNXENGINE_API AppFrameWork
{
public:
    AppFrameWork(const WindowProps& props);

    virtual ~AppFrameWork() {}

    void RunLoop();

    virtual void Initlize();

    virtual void Resize(uint32_t width, uint32_t height);

    virtual void RenderFrame();

    virtual void OnEvent(Event& e);

    void SetVSync(bool enable);
    bool IsVSync() const;

    // ==================== ImGui UI 层 ====================
    // 启用后：每帧由框架驱动 ImGui::NewFrame()，且事件分发时优先让 UI 消费输入
    // （UI 命中时不再传递给 3D 场景）。在 Initlize() 中调用即可。
    void SetImGuiEnabled(bool enabled) { mImGuiEnabled = enabled; }
    bool IsImGuiEnabled() const { return mImGuiEnabled; }

    // 获取 ImGui 层（未启用时返回 nullptr）
    RenderSystem::ImGuiRendererPtr GetImGui();

    // 每帧的开始时间（秒），供 ImGui 使用
    float GetDeltaTime() const { return mDeltaTime; }

private:
    RenderWindowPtr mRenderWindow = nullptr;

    void OnEventImpl(Event& e);
    void UpdateImGuiFrame();

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    bool mMinimized = false;

    bool     mImGuiEnabled = false;
    uint64_t mLastFrameTick = 0;
    float    mDeltaTime = 1.0f / 60.0f;
};

NAMESPACE_GNXENGINE_END

#endif // GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF
