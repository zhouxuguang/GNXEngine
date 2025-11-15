//
//  SDL2RenderWindow.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#ifndef GNX_ENGINE_SDL2_RENDER_WINDOW_INCLUDE_SGJDKF
#define GNX_ENGINE_SDL2_RENDER_WINDOW_INCLUDE_SGJDKF

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "SDL.h"

NAMESPACE_GNXENGINE_BEGIN

class SDL2RenderWindow : public RenderWindow
{
public:
    SDL2RenderWindow(const WindowProps& props);
    ~SDL2RenderWindow();

    virtual void OnUpdate();

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
    
    virtual void SetVSync(bool enabled);
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
    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    SDL_Window* mWindow = nullptr;
    bool mShoudClose = false;
};

NAMESPACE_GNXENGINE_END

#endif /* GNX_ENGINE_SDL2_RENDER_WINDOW_INCLUDE_SGJDKF */
