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
    mRenderWindow->SetEventCallback(GNX_BIND_EVENT_FN(OnEventImpl));
}

void AppFrameWork::RunLoop()
{
    Initlize();
    Resize(mRenderWindow->GetWidth(), mRenderWindow->GetHeight());
    while (mRenderWindow && !mRenderWindow->ShouldClose())
    {
        mRenderWindow->OnUpdate();
        RenderFrame();
    }

    // Flush pipeline cache to disk before RenderWindow is destroyed
    // (VKRenderDevice's destructor may not be called in time due to static shared_ptr)
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    if (renderDevice)
    {
        renderDevice->FlushPipelineCache();
    }
}

void AppFrameWork::Initlize()
{
}

void AppFrameWork::Resize(uint32_t width, uint32_t height)
{
    mRenderWindow->Resize(width, height);
}

void AppFrameWork::RenderFrame()
{
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    // 从Graphics队列创建命令缓冲区
    RenderCore::CommandQueuePtr graphicsQueue = renderDevice->GetCommandQueue(RenderCore::QueueType::Graphics, 0);
    RenderCore::CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

void AppFrameWork::OnEvent(Event& e)
{
    LOG_INFO("%s", e.ToString().c_str());
    
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(GNX_BIND_EVENT_FN(OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(GNX_BIND_EVENT_FN(OnWindowResize));
}

void AppFrameWork::OnEventImpl(Event& e)
{
    OnEvent(e);
}

bool AppFrameWork::OnWindowClose(WindowCloseEvent& e)
{
    LOG_INFO("%s", e.ToString().c_str());
    return true;
}

bool AppFrameWork::OnWindowResize(WindowResizeEvent& e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        mMinimized = true;
        return false;
    }

    mMinimized = false;
    mRenderWindow->Resize(e.GetWidth(), e.GetHeight());

    return false;
}

NAMESPACE_GNXENGINE_END
