#include "MTLGLFWFramework.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

static void quit(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

MTLGLFWFramework::MTLGLFWFramework(uint32_t width, uint32_t height, const char* title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(width, height, title, NULL, NULL);
    
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *mMetalLayer = [CAMetalLayer layer];
    mMetalLayer.device = gpu;
    mMetalLayer.opaque = YES;
    mMetalLayer.contentsScale = 1.0;
    mMetalLayer.framebufferOnly = YES;
    
    NSWindow *nswindow = glfwGetCocoaWindow(mWindow);
    nswindow.contentView.layer = mMetalLayer;
    nswindow.contentView.wantsLayer = YES;
    
    mRenderdevice = CreateRenderDevice(RenderCore::RenderDeviceType::METAL, (__bridge void*)mMetalLayer);
    
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);
    
    mRenderdevice->Resize(fbWidth, fbHeight);
    glfwSetKeyCallback(mWindow, quit);
}

MTLGLFWFramework::~MTLGLFWFramework()
{
    //
}

void MTLGLFWFramework::exec()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
        renderFrame();
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void MTLGLFWFramework::renderFrame()
{
    CommandBufferPtr commandBuffer = mRenderdevice->CreateCommandBuffer();
    RenderEncoderPtr renderEncoder1 = commandBuffer->CreateDefaultRenderEncoder();
    renderEncoder1->EndEncode();
    commandBuffer->PresentFrameBuffer();
}
