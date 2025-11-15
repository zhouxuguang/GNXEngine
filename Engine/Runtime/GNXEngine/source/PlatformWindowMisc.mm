#include "DefaultRenderWindow.h"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

#include "SDL_syswm.h"

NAMESPACE_GNXENGINE_BEGIN

void* GetPlatformWindow(GLFWwindow *window)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metalLayer = [CAMetalLayer layer];
    metalLayer.device = gpu;
    metalLayer.opaque = YES;
    metalLayer.contentsScale = 1.0;
    metalLayer.framebufferOnly = YES;
    
    NSWindow *nsWindow = glfwGetCocoaWindow(window);
    nsWindow.contentView.layer = metalLayer;
    nsWindow.contentView.wantsLayer = YES;

    return (__bridge void*)metalLayer;
}

void* GetSDL2PlatformWindow(const SDL_SysWMinfo& wmInfo)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metalLayer = [CAMetalLayer layer];
    metalLayer.device = gpu;
    metalLayer.opaque = YES;
    metalLayer.contentsScale = 1.0;
    metalLayer.framebufferOnly = YES;
    
    NSWindow* nsWindow = wmInfo.info.cocoa.window;
    nsWindow.contentView.layer = metalLayer;
    nsWindow.contentView.wantsLayer = YES;

    return (__bridge void*)metalLayer;
}

NAMESPACE_GNXENGINE_END
