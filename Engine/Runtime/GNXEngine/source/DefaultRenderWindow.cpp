//
//  DefaultRenderWindow.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#include "DefaultRenderWindow.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

NAMESPACE_GNXENGINE_BEGIN

extern void* GetPlatformWindow(GLFWwindow *window);

#if OS_WINDOWS
void* GetPlatformWindow(GLFWwindow *window)
{
    return glfwGetWin32Window(window);
}
#endif

DefaultRenderWindow::DefaultRenderWindow(const WindowProps& props)
{
    mData.width = props.width;
    mData.height = props.height;
    mData.title = props.title;
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
    
    mRenderDevice->Resize(mData.width, mData.height);
    SetVSync(false);
    Init();

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    
    //初始化相机
    RenderSystem::CameraPtr cameraPtr = sceneManager->createCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(mData.width) / mData.height, 0.1f, 1000.f);
}

DefaultRenderWindow::~DefaultRenderWindow()
{
    Shutdown();
}

void DefaultRenderWindow::SetEventCallback(const EventCallbackFunc& callback)
{
    mData.eventCallback = callback;
}

inline void DefaultRenderWindow::SetVSync(bool enabled)
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

void DefaultRenderWindow::Resize(uint32_t width, uint32_t height)
{
    mData.width = width;
    mData.height = height;
    
    mRenderDevice->Resize(width, height);
}

void DefaultRenderWindow::Shutdown()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

bool DefaultRenderWindow::ShouldClose() const
{
    return glfwWindowShouldClose(mWindow);
}

void DefaultRenderWindow::Init()
{
    glfwSetWindowUserPointer(mWindow, &mData);
    
    // Set GLFW callbacks
    glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.width = width;
        data.height = height;

        WindowResizeEvent event(width, height);
        data.eventCallback(event);
    });
    
    glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        WindowCloseEvent event;
        data.eventCallback(event);
    });
    
    glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action)
        {
            case GLFW_PRESS:
            {
                KeyPressedEvent event(key, 0);
                data.eventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event(key);
                data.eventCallback(event);
                break;
            }
            case GLFW_REPEAT:
            {
                KeyPressedEvent event(key, true);
                data.eventCallback(event);
                break;
            }
        }
    });
    
    glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int button, int action, int mods)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action)
        {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event(button);
                data.eventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event(button);
                data.eventCallback(event);
                break;
            }
        }
    });
    
    glfwSetScrollCallback(mWindow, [](GLFWwindow* window, double xOffset, double yOffset)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        
        // 标准化滚轮的数值
        if (yOffset < 0)
        {
            yOffset = -120;
        }
        else if (yOffset > 0)
        {
            yOffset = 120;
        }

        MouseScrolledEvent event((float)xOffset, (float)yOffset);
        data.eventCallback(event);
    });
    
    glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, double xPos, double yPos)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        MouseMovedEvent event((float)xPos, (float)yPos);
        data.eventCallback(event);
    });
}

NAMESPACE_GNXENGINE_END
