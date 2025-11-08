//
//  DefaultRenderWindow.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#ifndef GNX_ENGINE_DEFAULT_RENDER_WINDOW_INCLUDE_NFJS
#define GNX_ENGINE_DEFAULT_RENDER_WINDOW_INCLUDE_NFJS

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

#define GLFW_INCLUDE_NONE
#if OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#elif OS_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
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

class DefaultRenderWindow : public RenderWindow
{
public:
    DefaultRenderWindow(const WindowProps& props);
    ~DefaultRenderWindow();

    virtual void OnUpdate()
    {
        glfwPollEvents();
    }

    virtual bool ShouldClose() const;

    virtual uint32_t GetWidth() const
    {
        return mData.width;
    }

    virtual uint32_t GetHeight() const
    {
        return mData.height;
    }

    virtual void SetEventCallback(const EventCallbackFunc& callback);
    
    virtual void SetVSync(bool enabled)
    {
        if (enabled)
        {
            glfwSwapInterval(1);
        }
		else
        {
            glfwSwapInterval(0);
        }

		mData.VSync = enabled;
    }
    virtual bool IsVSync() const
    {
        return mData.VSync;
    }

    virtual void* GetNativeWindow() const
    {
        return mWindow;
    }
    
    virtual void Resize(uint32_t width, uint32_t height);

    virtual void Shutdown();
    
    void Init();

private:
    WindowData mData;
    GLFWwindow *mWindow = nullptr;
    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
};

NAMESPACE_GNXENGINE_END

#endif /* GNX_ENGINE_DEFAULT_RENDER_WINDOW_INCLUDE_NFJS */
