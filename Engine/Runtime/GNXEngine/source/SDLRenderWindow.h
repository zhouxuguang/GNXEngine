//
//  SDLRenderWindow.h
//  GNXEngine
//
//  SDL2 窗口实现（Android / iOS）
//

#if GNX_WINDOW_SDL

#ifndef GNX_ENGINE_SDL_RENDER_WINDOW_INCLUDE_NFJS
#define GNX_ENGINE_SDL_RENDER_WINDOW_INCLUDE_NFJS

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

#include <SDL.h>

NAMESPACE_GNXENGINE_BEGIN

struct WindowData
{
    std::string title;
    uint32_t width;
    uint32_t height;
    bool VSync;
    RenderWindow::EventCallbackFunc eventCallback = nullptr;
};

class SDLRenderWindow : public RenderWindow
{
public:
    SDLRenderWindow(const WindowProps& props);
    ~SDLRenderWindow();

    virtual void OnUpdate() override;

    virtual bool ShouldClose() const override;

    virtual uint32_t GetWidth() const override { return mData.width; }
    virtual uint32_t GetHeight() const override { return mData.height; }

    virtual void SetEventCallback(const EventCallbackFunc& callback) override;
    virtual void SetVSync(bool enabled) override;
    virtual bool IsVSync() const override { return mData.VSync; }

    virtual void* GetNativeWindow() const override { return mWindow; }

    virtual void Resize(uint32_t width, uint32_t height) override;

    // 手动触发事件回调（用于外部事件系统）
    virtual void TriggerEventCallback(Event& event) override;

    void Shutdown();

private:
    void Init();
    void HandleSDLEvents();

    WindowData mData;
    SDL_Window* mWindow = nullptr;
    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
};

NAMESPACE_GNXENGINE_END

#endif /* GNX_ENGINE_SDL_RENDER_WINDOW_INCLUDE_NFJS */

#endif // GNX_WINDOW_SDL
