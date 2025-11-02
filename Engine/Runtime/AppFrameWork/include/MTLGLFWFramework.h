#ifndef MTLGLFW_FRAMWWORK_INCLUDE_DHSFJSDHJ_J
#define MTLGLFW_FRAMWWORK_INCLUDE_DHSFJSDHJ_J

#include "Runtime/RenderSystem/include/RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

class MTLGLFWFramework
{
public:
    MTLGLFWFramework(uint32_t width, uint32_t height, const char* title);
    
    ~MTLGLFWFramework();

    void exec();
    
    virtual void renderFrame();
    
private:
    GLFWwindow *mWindow = nullptr;
    RenderDevicePtr mRenderdevice;
};

#endif
