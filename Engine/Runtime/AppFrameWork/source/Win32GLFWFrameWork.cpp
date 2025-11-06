#include "Win32GLFWFrameWork.h"

static void quit(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

Win32GLFWFrameWork::Win32GLFWFrameWork(uint32_t width, uint32_t height, const char* title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    float scaleX = 1.0, scaleY = 1.0;
	//glfwGetMonitorContentScale(monitor, &scaleX, &scaleY);

    mWindow = glfwCreateWindow(width * scaleX, height * scaleY, title, NULL, NULL);

    HWND hWnd = glfwGetWin32Window(mWindow);
    
    mRenderdevice = CreateRenderDevice(RenderCore::RenderDeviceType::VULKAN, hWnd);
    
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);
    
    mRenderdevice->Resize(fbWidth, fbHeight);
    glfwSetKeyCallback(mWindow, quit);
}

Win32GLFWFrameWork::~Win32GLFWFrameWork()
{
    //
}

void Win32GLFWFrameWork::exec()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
        renderFrame();
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void Win32GLFWFrameWork::renderFrame()
{
    CommandBufferPtr commandBuffer = mRenderdevice->CreateCommandBuffer();
    RenderEncoderPtr renderEncoder1 = commandBuffer->CreateDefaultRenderEncoder();
    renderEncoder1->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

AppFrameWork* AppFrameWork::Create(uint32_t width, uint32_t height, const char* title)
{
    return new Win32GLFWFrameWork(width, height, title);
}
