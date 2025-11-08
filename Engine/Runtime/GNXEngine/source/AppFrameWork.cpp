#include "AppFrameWork.h"

NAMESPACE_GNXENGINE_BEGIN

#define GNX_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

AppFrameWork::AppFrameWork(const WindowProps& props)
{
    mRenderWindow = RenderWindow::Create(props);
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
    //
}

NAMESPACE_GNXENGINE_END
