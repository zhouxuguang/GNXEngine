//
//  DefaultRenderWindow.cpp
//  GNXEngine
//
//  GLFW 窗口实现（Windows / macOS / Linux）
//

#if !GNX_WINDOW_SDL

#include "DefaultRenderWindow.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "InputState.h"

NAMESPACE_GNXENGINE_BEGIN

extern void* GetPlatformWindow(GLFWwindow *window);

#if GNX_OS_WINDOWS
void* GetPlatformWindow(GLFWwindow *window)
{
    return glfwGetWin32Window(window);
}
#endif

#if GNX_OS_LINUX
#include <X11/Xlib.h>
void* GetPlatformWindow(GLFWwindow *window)
{
    RenderCore::X11ViewHandle* h = new RenderCore::X11ViewHandle();
    h->display = glfwGetX11Display();
    h->window = (void*)(uintptr_t)glfwGetX11Window(window);
    return (void*)h;
}
#endif

DefaultRenderWindow::DefaultRenderWindow(const WindowProps& props)
{
    mData.width = props.width;
    mData.height = props.height;
    mData.title = props.title;
    // 固定 GLFW 后端平台，避免运行时自动探测
    // Windows→Win32 / macOS→Cocoa / Linux→X11（项目使用 glfwGetX11Display/Win32 原生接口）
#if GNX_OS_MACOS
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
#elif GNX_OS_WINDOWS
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
#elif GNX_OS_LINUX
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    mWindow = glfwCreateWindow(mData.width, mData.height, mData.title.c_str(), NULL, NULL);

    void* nativeWnd = GetPlatformWindow(mWindow);

    // 在这里选择底层的渲染器类型，创建它
#if GNX_OS_WINDOWS | GNX_OS_LINUX
    mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, nativeWnd);
#elif GNX_OS_MACOS
    mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, nativeWnd);
#endif

    // HiDPI 支持：使用 framebuffer 的物理像素尺寸创建交换链，
    // 而不是窗口的逻辑尺寸（高 DPI 下两者不同）
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);
    mData.width = static_cast<uint32_t>(fbWidth);
    mData.height = static_cast<uint32_t>(fbHeight);

    mRenderDevice->Resize(mData.width, mData.height);
    SetVSync(false);
    Init();

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();

    //初始化相机
    RenderSystem::CameraPtr cameraPtr = sceneManager->CreateCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, mData.width, mData.height, 0.1f, 1000.f);
}

DefaultRenderWindow::DefaultRenderWindow(const WindowProps& props, void* externalWindowHandle)
{
    mData.width = props.width;
    mData.height = props.height;
    mData.title = props.title;
    mUseExternalWindow = (externalWindowHandle != nullptr);

    if (mUseExternalWindow)
    {
        // 使用外部窗口（如 Qt）
        // 设置输入模式为 Event，由 Qt 事件驱动输入状态
        InputState::GetInstance().SetMode(InputMode::Event);

        // 固定 GLFW 后端平台（与独立窗口保持一致）
#if GNX_OS_MACOS
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
#elif GNX_OS_WINDOWS
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
#elif GNX_OS_LINUX
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
        glfwInit();

        // 创建一个隐藏的 GLFW 窗口用于事件轮询
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        mWindow = glfwCreateWindow(1, 1, "Hidden GLFW Window", NULL, NULL);

        // 使用外部窗口句柄创建 RenderDevice
#if GNX_OS_WINDOWS
        mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, externalWindowHandle);
#elif GNX_OS_MACOS
        mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, externalWindowHandle);
#elif GNX_OS_LINUX
		mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, externalWindowHandle);
#endif

        mRenderDevice->Resize(mData.width, mData.height);
        SetVSync(false);

        // 不需要初始化 GLFW 回调，因为事件由 Qt 处理
        RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();

        //初始化相机
        RenderSystem::CameraPtr cameraPtr = sceneManager->CreateCamera("MainCamera");
        cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
        cameraPtr->SetLens(60, mData.width, mData.height, 0.1f, 1000.f);
    }
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
    // 只在使用 GLFW 窗口时才设置回调
    if (!mUseExternalWindow)
    {
        glfwSetWindowUserPointer(mWindow, &mData);

        // Set GLFW callbacks
        // HiDPI 支持：使用 framebuffer 尺寸回调（物理像素），
        // 而不是窗口尺寸回调（逻辑像素），确保高 DPI 下渲染分辨率正确
        glfwSetFramebufferSizeCallback(mWindow, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.width = static_cast<uint32_t>(width);
            data.height = static_cast<uint32_t>(height);

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
}

void DefaultRenderWindow::TriggerEventCallback(Event& event)
{
    if (mData.eventCallback) 
    {
        mData.eventCallback(event);
    }
}

NAMESPACE_GNXENGINE_END

#endif // !GNX_WINDOW_SDL
