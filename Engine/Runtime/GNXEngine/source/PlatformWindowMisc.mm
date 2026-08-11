#if !GNX_WINDOW_SDL

#include "DefaultRenderWindow.h"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

NAMESPACE_GNXENGINE_BEGIN

RenderCore::NativeWindow GetPlatformWindow(GLFWwindow *window)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metalLayer = [CAMetalLayer layer];
    metalLayer.device = gpu;
    metalLayer.opaque = YES;

    NSWindow *nsWindow = glfwGetCocoaWindow(window);

    // HiDPI 支持：让 Metal layer 的 contentsScale 跟随窗口所在屏幕的缩放因子，
    // 这样 drawableSize（物理像素）才能正确对应 Retina 分辨率
    CGFloat scale = nsWindow.screen ? nsWindow.screen.backingScaleFactor : 1.0;
    metalLayer.contentsScale = scale;

    metalLayer.framebufferOnly = YES;
    metalLayer.displaySyncEnabled = YES;    // 默认开启垂直同步
    
    nsWindow.contentView.layer = metalLayer;
    nsWindow.contentView.wantsLayer = YES;

    RenderCore::NativeWindow nw;
    nw.viewHandle = (__bridge void*)metalLayer;
    return nw;
}

NAMESPACE_GNXENGINE_END

#endif // !GNX_WINDOW_SDL
