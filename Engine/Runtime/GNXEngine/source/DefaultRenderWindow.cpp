//
//  DefaultRenderWindow.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#include "DefaultRenderWindow.h"

NAMESPACE_GNXENGINE_BEGIN

extern void* GetPlatformWindow(GLFWwindow *window);

#if OS_WINDOWS
void* GetPlatformWindow(GLFWwindow *window)
{
    return glfwGetWin32Window(mWindow);
}
#endif

DefaultRenderWindow::DefaultRenderWindow(const WindowProps& props)
{
    mData.width = props.width;
    mData.height = props.height;
    mData.title = props.title;
    //mData.width = props.width;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    mWindow = glfwCreateWindow(mData.width, mData.height, mData.title.c_str(), NULL, NULL);

    void* nativeWnd = GetPlatformWindow(mWindow);

    // 在这里选择底层的渲染器类型，创建它
#if OS_WINDOWS
    mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, nativeWnd);
#elif OS_MACOS
    mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, nativeWnd);
#endif

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);
    mData.width = fbWidth;
    mData.height = fbHeight;
    
    mRenderDevice->Resize(fbWidth, fbHeight);
    SetVSync(true);
}

DefaultRenderWindow::~DefaultRenderWindow()
{
    Shutdown();
}

void DefaultRenderWindow::Shutdown()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

NAMESPACE_GNXENGINE_END
