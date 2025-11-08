#include "AppFrameWork.h"
#include "Runtime/BaseLib/include/LogService.h"

NAMESPACE_GNXENGINE_BEGIN

RenderWindowPtr gRenderWindow = nullptr;

RenderWindowPtr GetRenderWindow()
{
    return gRenderWindow;
}

AppFrameWork::AppFrameWork(const WindowProps& props)
{
    mRenderWindow = RenderWindow::Create(props);
    gRenderWindow = mRenderWindow;
    mRenderWindow->SetEventCallback(GNX_BIND_EVENT_FN(OnEvent));
}

void AppFrameWork::RunLoop()
{
    Initlize();
    while (mRenderWindow && !mRenderWindow->ShouldClose())
    {
        mRenderWindow->OnUpdate();
        RenderFrame();
    }
}

void AppFrameWork::Initlize()
{
    //
}

void AppFrameWork::Resize(uint32_t width, uint32_t height)
{
    //
}

void AppFrameWork::RenderFrame()
{
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    RenderCore::CommandBufferPtr commandBuffer = renderDevice->CreateCommandBuffer();
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

void AppFrameWork::OnEvent(Event& e)
{
    LOG_INFO("%s", e.ToString().c_str());
}

NAMESPACE_GNXENGINE_END
