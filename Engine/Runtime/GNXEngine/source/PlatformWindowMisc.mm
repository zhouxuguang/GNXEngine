#include "DefaultRenderWindow.h"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

NAMESPACE_GNXENGINE_BEGIN

void* GetPlatformWindow(GLFWwindow *window)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *mMetalLayer = [CAMetalLayer layer];
    mMetalLayer.device = gpu;
    mMetalLayer.opaque = YES;
    mMetalLayer.contentsScale = 1.0;
    mMetalLayer.framebufferOnly = YES;
    
    NSWindow *nswindow = glfwGetCocoaWindow(window);
    nswindow.contentView.layer = mMetalLayer;
    nswindow.contentView.wantsLayer = YES;

    return (__bridge void*)mMetalLayer;
}

NAMESPACE_GNXENGINE_END
