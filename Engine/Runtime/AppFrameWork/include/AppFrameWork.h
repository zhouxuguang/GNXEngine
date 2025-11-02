#ifndef GNXENGINE_APP_FRAMEWORK_INCLUDE_KJFVNHDFJ
#define GNXENGINE_APP_FRAMEWORK_INCLUDE_KJFVNHDFJ

#include "Runtime/RenderSystem/include/RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

class AppFrameWork
{
public:
    virtual void exec() = 0;
    
    virtual void renderFrame() = 0;

    static AppFrameWork* Create(uint32_t width, uint32_t height, const char* title);

protected:
	AppFrameWork() {}

    virtual ~AppFrameWork() {}
};

#endif // GNXENGINE_APP_FRAMEWORK_INCLUDE_KJFVNHDFJ