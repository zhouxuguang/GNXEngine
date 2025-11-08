#include "AppFrameWork.h"

NAMESPACE_GNXENGINE_BEGIN

AppFrameWork::AppFrameWork(const WindowProps& props)
{
    mRenderWindow = RenderWindow::Create(props);
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

NAMESPACE_GNXENGINE_END
