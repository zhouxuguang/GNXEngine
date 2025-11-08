#ifndef GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF
#define GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF

#include "RenderWindow.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

NAMESPACE_GNXENGINE_BEGIN

// 应用程序框架类，子类继承，需要重写Initlize和RenderFrame方法
class AppFrameWork
{
public:
    AppFrameWork(const WindowProps& props);

    virtual ~AppFrameWork() {}
    
    void RunLoop();
    
    virtual void Initlize();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual void RenderFrame();

private:
    RenderWindowPtr mRenderWindow = nullptr;
    
    void OnEvent(Event& e);
    
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
    
    bool mMinimized = false;
};

NAMESPACE_GNXENGINE_END

#endif // GNXENGINE_APP_FRAMEWORK_INCLUDE_DSKGJJDF
