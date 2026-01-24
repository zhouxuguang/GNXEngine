//
//  RenderWindow.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/20.
//

#ifndef GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE
#define GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE

#include "PreDefine.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

NAMESPACE_GNXENGINE_BEGIN

// 窗口属性
struct WindowProps
{
    std::string title;
    uint32_t width;
    uint32_t height;

    WindowProps(const std::string& Title = "GNXEngine",
                uint32_t Width = 800,
                uint32_t Height = 600)
        : title(Title), width(Width), height(Height)
    {
    }
};

//渲染窗口
class GNXENGINE_API RenderWindow
{
public:
    using EventCallbackFunc = std::function<void(Event&)>;

    RenderWindow() = default;

    virtual ~RenderWindow() = default;

    virtual void OnUpdate() = 0;

    virtual bool ShouldClose() const = 0;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;

    virtual void SetEventCallback(const EventCallbackFunc& callback) = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual bool IsVSync() const = 0;

    virtual void* GetNativeWindow() const = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // 手动触发事件回调（用于外部事件系统如 Qt 的事件转发）
    virtual void TriggerEventCallback(Event& event) = 0;

    static std::shared_ptr<RenderWindow> Create(const WindowProps& props = WindowProps());
    // 创建使用外部窗口句柄的 RenderWindow（用于 Qt 嵌入）
    static std::shared_ptr<RenderWindow> CreateWithExternalWindow(const WindowProps& props, void* externalWindowHandle);
};

typedef std::shared_ptr<RenderWindow> RenderWindowPtr;

RenderWindowPtr GetRenderWindow();

NAMESPACE_GNXENGINE_END


#endif //GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE
