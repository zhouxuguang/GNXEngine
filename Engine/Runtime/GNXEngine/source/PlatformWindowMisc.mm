#if !GNX_WINDOW_SDL

#include "DefaultRenderWindow.h"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

NAMESPACE_GNXENGINE_BEGIN

void* GetPlatformWindow(GLFWwindow *window)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metalLayer = [CAMetalLayer layer];
    metalLayer.device = gpu;
    metalLayer.opaque = YES;
    metalLayer.contentsScale = 1.0;
    metalLayer.framebufferOnly = YES;
    metalLayer.displaySyncEnabled = YES;    // 默认开启垂直同步
    
    NSWindow *nsWindow = glfwGetCocoaWindow(window);
    nsWindow.contentView.layer = metalLayer;
    nsWindow.contentView.wantsLayer = YES;

    return (__bridge void*)metalLayer;
}

NAMESPACE_GNXENGINE_END

#endif // !GNX_WINDOW_SDL
