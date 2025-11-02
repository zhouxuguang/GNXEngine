#ifndef GNX_ENGINE_WIN32_GLFW_FRAME_WORK_INCLUDE_SJHGVJD
#define GNX_ENGINE_WIN32_GLFW_FRAME_WORK_INCLUDE_SJHGVJD

#include "Runtime/RenderSystem/include/RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "AppFrameWork.h"

class Win32GLFWFrameWork : public AppFrameWork
{
public:
    Win32GLFWFrameWork(uint32_t width, uint32_t height, const char* title);
    
    ~Win32GLFWFrameWork();

    virtual void exec();
    
    virtual void renderFrame();
    
private:
    GLFWwindow *mWindow = nullptr;
    RenderDevicePtr mRenderdevice;
};

#endif // GNX_ENGINE_WIN32_GLFW_FRAME_WORK_INCLUDE_SJHGVJD