//
//  SDLRenderWindow.cpp
//  GNXEngine
//
//  SDL2 窗口实现（Android / iOS）
//  仅负责窗口创建 + 输入事件 + 渲染设备初始化
//

#if GNX_WINDOW_SDL

#include "SDLRenderWindow.h"

#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "InputState.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <SDL.h>
#include <SDL_syswm.h>

#if TARGET_OS_IOS
#include <SDL_metal.h>
#endif

NAMESPACE_GNXENGINE_BEGIN

// —— 将 SDL Keycode 映射到引擎 KeyCode ——
static KeyCode MapSDLKeyToKeyCode(SDL_Keycode sdlKey)
{
    // 字母/数字键直接映射（SDL Keycode 和 KeyCode 都用 ASCII 兼容值）
    if (sdlKey >= 'A' && sdlKey <= 'Z')   return static_cast<KeyCode>(sdlKey);
    if (sdlKey >= '0' && sdlKey <= '9')   return static_cast<KeyCode>(sdlKey);
    // 功能键
    switch (sdlKey)
    {
        case SDLK_RETURN:    return Enter;
        case SDLK_ESCAPE:    return Escape;
        case SDLK_BACKSPACE: return Backspace;
        case SDLK_TAB:       return Tab;
        case SDLK_SPACE:     return Space;
        case SDLK_LEFT:      return Left;
        case SDLK_RIGHT:     return Right;
        case SDLK_UP:        return Up;
        case SDLK_DOWN:      return Down;
        default:             return static_cast<KeyCode>(0);
    }
}

static MouseCode MapSDLButtonToMouseCode(uint8_t sdlButton)
{
    switch (sdlButton)
    {
        case SDL_BUTTON_LEFT:   return Button0;
        case SDL_BUTTON_MIDDLE: return Button2;
        case SDL_BUTTON_RIGHT:  return Button1;
        default:                return Button0;
    }
}

// ========================================================================
// 构造
// ========================================================================
SDLRenderWindow::SDLRenderWindow(const WindowProps& props)
{
    mData.width  = props.width;
    mData.height = props.height;
    mData.title  = props.title;

    // 初始化 SDL（仅 Video + Events 子系统）
    const uint32_t sdlInitFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    if (SDL_Init(sdlInitFlags) != 0)
    {
        LOG_ERROR("SDLRenderWindow: SDL_Init failed: %s", SDL_GetError());
        return;
    }

    // 窗口标志
    uint32_t windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN;

#if GNX_OS_IOS
    windowFlags |= SDL_WINDOW_METAL;
    // 代码层面控制方向（需与 Info.plist 的 UISupportedInterfaceOrientations 取交集）
    // 这里显式声明支持横屏左右 + 竖屏，避免仅凭 main.cpp 的横屏尺寸推断方向
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait");
#endif

    mWindow = SDL_CreateWindow(
        mData.title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(mData.width), static_cast<int>(mData.height),
        windowFlags);

    if (!mWindow)
    {
        LOG_ERROR("SDLRenderWindow: SDL_CreateWindow failed: %s", SDL_GetError());
        return;
    }
    
    int realW = 0, realH = 0;
    SDL_GetWindowSizeInPixels(mWindow, &realW, &realH);
    if (realW > 0 && realH > 0)
    {
        mData.width  = (uint32_t)realW;
        mData.height = (uint32_t)realH;
    }

    // —— 创建渲染设备 ——
    void* nativeWnd = nullptr;

#if GNX_OS_IOS
    // iOS: SDL_Metal_CreateView → CAMetalLayer
    SDL_MetalView metalView = SDL_Metal_CreateView(mWindow);
    if (metalView)
    {
        nativeWnd = SDL_Metal_GetLayer(metalView);
    }
    if (nativeWnd)
    {
        RenderCore::NativeWindow nw;
        nw.viewHandle = nativeWnd;
        mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, nw);
        if (mRenderDevice)
        {
            mRenderDevice->Resize(mData.width, mData.height);
        }
    }
#elif GNX_OS_ANDROID
    // Android通过 SDL_GetWindowWMInfo 获取 ANativeWindow
    struct SDL_SysWMinfo sysWMinfo;
    SDL_VERSION(&sysWMinfo.version);
    bool success = SDL_GetWindowWMInfo(mWindow, &sysWMinfo);
    nativeWnd = sysWMinfo.info.android.window;

    if (nativeWnd)
    {
        RenderCore::NativeWindow nw;
        nw.viewHandle = nativeWnd;
        mRenderDevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, nw);
        if (mRenderDevice)
        {
            mRenderDevice->Resize(mData.width, mData.height);
        }
    }
#endif

    SetVSync(false);
    Init();
}

// ========================================================================
// 析构
// ========================================================================
SDLRenderWindow::~SDLRenderWindow()
{
    Shutdown();
}

// ========================================================================
// 公共接口
// ========================================================================

void SDLRenderWindow::OnUpdate()
{
    HandleSDLEvents();
}

bool SDLRenderWindow::ShouldClose() const
{
    return mWindow == nullptr;  // 在处理 SDL_QUIT 时会置 null
}

void SDLRenderWindow::SetEventCallback(const EventCallbackFunc& callback)
{
    mData.eventCallback = callback;
}

void SDLRenderWindow::SetVSync(bool enabled)
{
    // SDL 本身没有直接的 VSync API，Metal 端控制
    mData.VSync = enabled;
    if (mRenderDevice)
    {
        mRenderDevice->SetVSync(enabled);
    }
}

void SDLRenderWindow::Resize(uint32_t width, uint32_t height)
{
    mData.width  = width;
    mData.height = height;

    if (mRenderDevice)
    {
        mRenderDevice->Resize(width, height);
    }
}

void SDLRenderWindow::Shutdown()
{
    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    SDL_Quit();
}

void SDLRenderWindow::TriggerEventCallback(Event& event)
{
    if (mData.eventCallback)
    {
        mData.eventCallback(event);
    }
}

// ========================================================================
// 初始化事件回调
// ========================================================================
void SDLRenderWindow::Init()
{
    // SDL 事件通过 HandleSDLEvents() 在 OnUpdate() 中轮询
}

// ========================================================================
// SDL 事件处理（在 OnUpdate 中每帧调用）
// ========================================================================
void SDLRenderWindow::HandleSDLEvents()
{
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        switch (sdlEvent.type)
        {
            // —— 键盘 ——
            case SDL_KEYDOWN:
            {
                if (!sdlEvent.key.repeat)
                {
                    KeyPressedEvent event(MapSDLKeyToKeyCode(sdlEvent.key.keysym.sym), 0);
                    mData.eventCallback(event);
                }
                break;
            }
            case SDL_KEYUP:
            {
                KeyReleasedEvent event(MapSDLKeyToKeyCode(sdlEvent.key.keysym.sym));
                mData.eventCallback(event);
                break;
            }

            // —— 触摸 / 鼠标按钮 ——
            case SDL_MOUSEBUTTONDOWN:
    #if TARGET_OS_IOS || defined(__ANDROID__)
            case SDL_FINGERDOWN:
    #endif
            {
                MouseButtonPressedEvent event(
                    (sdlEvent.type == SDL_FINGERDOWN) ? Button0
                                                      : MapSDLButtonToMouseCode(sdlEvent.button.button));
                mData.eventCallback(event);
                break;
            }
            case SDL_MOUSEBUTTONUP:
    #if TARGET_OS_IOS || defined(__ANDROID__)
            case SDL_FINGERUP:
    #endif
            {
                MouseButtonReleasedEvent event(
                    (sdlEvent.type == SDL_FINGERUP) ? Button0
                                                    : MapSDLButtonToMouseCode(sdlEvent.button.button));
                mData.eventCallback(event);
                break;
            }

            // —— 触摸/鼠标移动 ——
            case SDL_MOUSEMOTION:
            {
                MouseMovedEvent event(
                    static_cast<float>(sdlEvent.motion.x),
                    static_cast<float>(sdlEvent.motion.y));
                mData.eventCallback(event);
                break;
            }
    #if TARGET_OS_IOS || defined(__ANDROID__)
            case SDL_FINGERMOTION:
            {
                int winW, winH;
                SDL_GetWindowSize(mWindow, &winW, &winH);
                MouseMovedEvent event(
                    sdlEvent.tfinger.x * winW,
                    sdlEvent.tfinger.y * winH);
                mData.eventCallback(event);
                break;
            }
    #endif

            // —— 窗口大小变化 / 生命周期 ——
            case SDL_WINDOWEVENT:
            {
                if (sdlEvent.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    // 用像素尺寸（而非事件里的逻辑点），确保高分屏(Retina)下渲染分辨率正确
                    int pxW = 0, pxH = 0;
                    SDL_GetWindowSizeInPixels(mWindow, &pxW, &pxH);
                    if (pxW <= 0 || pxH <= 0)
                    {
                        // 回退到事件携带的尺寸（非高分屏场景）
                        pxW = sdlEvent.window.data1;
                        pxH = sdlEvent.window.data2;
                    }
                    mData.width  = static_cast<uint32_t>(pxW);
                    mData.height = static_cast<uint32_t>(pxH);
                    WindowResizeEvent event(mData.width, mData.height);
                    mData.eventCallback(event);
                }
                // —— 窗口进入后台 ——
                // 注意：MINIMIZED 时底层 Surface 还没销毁（真正销毁发生在 surfaceDestroyed），
                //       只需暂停渲染、等待 GPU 空闲即可，绝不能在这里重建 surface+swapchain。
                else if (sdlEvent.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    mAppActive = false;
                    if (mRenderDevice)
                    {
                        mRenderDevice->OnWindowMinimized();
                    }
                }
                // —— 窗口从后台恢复 ——
                // 恢复时底层 ANativeWindow 已被 surfaceCreated 重建，必须用新句柄
                // 重建 VkSurfaceKHR + swapchain（Android 核心路径）。
                else if (sdlEvent.window.event == SDL_WINDOWEVENT_RESTORED)
                {
                    mAppActive = true;
    #if defined(__ANDROID__)
                    // 重新获取最新的 ANativeWindow（SDL 内部已在 onNativeSurfaceCreated 更新）
                    struct SDL_SysWMinfo sysWMinfo;
                    SDL_VERSION(&sysWMinfo.version);
                    if (SDL_GetWindowWMInfo(mWindow, &sysWMinfo))
                    {
                        void* nativeWnd = sysWMinfo.info.android.window;
                        if (nativeWnd && mRenderDevice)
                        {
                            // 用新的 ANativeWindow 重建 surface + swapchain
                            RenderCore::NativeWindow nw;
                            nw.viewHandle = nativeWnd;
                            mRenderDevice->OnWindowRestored(nw);
                        }
                    }
    #else
                    if (mRenderDevice)
                    {
                        mRenderDevice->OnWindowRestored(RenderCore::NativeWindow());
                    }
    #endif
                }
                break;
            }

            // —— 应用生命周期事件（Android/iOS） ——
            case SDL_APP_WILLENTERBACKGROUND:
            {
                // 即将进入后台：暂停渲染
                mAppActive = false;
                if (mRenderDevice)
                {
                    mRenderDevice->OnWindowMinimized();
                }
                break;
            }
            case SDL_APP_DIDENTERBACKGROUND:
            {
                // 已进入后台：确保暂停
                mAppActive = false;
                break;
            }
            case SDL_APP_WILLENTERFOREGROUND:
            case SDL_APP_DIDENTERFOREGROUND:
            {
                // 回到前台：恢复渲染（surface 由 RESTORED 事件重建）
                mAppActive = true;
                break;
            }

            // —— 退出 ——
            case SDL_QUIT:
            {
                WindowCloseEvent event;
                mData.eventCallback(event);
                // 关闭窗口
                SDL_DestroyWindow(mWindow);
                mWindow = nullptr;
                break;
            }
        }
    }
}

NAMESPACE_GNXENGINE_END

#endif // GNX_WINDOW_SDL
