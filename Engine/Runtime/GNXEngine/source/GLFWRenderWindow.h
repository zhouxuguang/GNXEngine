//
//  GLFWRenderWindow.h
//  GNXEngine
//
//  GLFW 窗口实现（Windows / macOS / Linux）
//

#if !GNX_WINDOW_SDL

#ifndef GNX_ENGINE_GLFW_RENDER_WINDOW_INCLUDE_NFJS
#define GNX_ENGINE_GLFW_RENDER_WINDOW_INCLUDE_NFJS

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

#define GLFW_INCLUDE_NONE
#if GNX_OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#elif GNX_OS_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#elif GNX_OS_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#endif

NAMESPACE_GNXENGINE_BEGIN

struct WindowData
{
    std::string title;
    uint32_t width;
    uint32_t height;
    bool VSync;
    RenderWindow::EventCallbackFunc eventCallback = nullptr;
};

class GLFWRenderWindow : public RenderWindow
{
public:
    // 默认构造函数，创建独立的窗口
    GLFWRenderWindow(const WindowProps& props);
    // 构造函数，使用外部窗口句柄（用于 Qt 嵌入）
    GLFWRenderWindow(const WindowProps& props, void* externalWindowHandle);
    ~GLFWRenderWindow();

    void OnUpdate() override
    {
        glfwPollEvents();
    }

    bool ShouldClose() const override;

    uint32_t GetWidth() const override
    {
        return mData.width;
    }

    uint32_t GetHeight() const override
    {
        return mData.height;
    }

    void SetEventCallback(const EventCallbackFunc& callback) override;

    void SetVSync(bool enabled) override;
    bool IsVSync() const override
    {
        return mData.VSync;
    }

    void* GetNativeWindow() const override
    {
        return mWindow;
    }

    void Resize(uint32_t width, uint32_t height) override;

    virtual void Shutdown();

    void Init();

    // 手动触发事件回调（用于 Qt 事件转发）
    void TriggerEventCallback(Event& event) override;

private:
    WindowData mData;
    GLFWwindow *mWindow = nullptr;
    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    bool mUseExternalWindow = false; // 是否使用外部窗口
};

NAMESPACE_GNXENGINE_END

#endif /* GNX_ENGINE_GLFW_RENDER_WINDOW_INCLUDE_NFJS */

#endif // !GNX_WINDOW_SDL
