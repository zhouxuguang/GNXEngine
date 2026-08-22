#include "AppFrameWork.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include <tracy/Tracy.hpp>

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

        // 移动端后台时暂停渲染（SDL_APP_DIDENTERBACKGROUND 时 IsAppActive() == false），
        // 避免在失效的 swapchain / surface 上执行 vkAcquireNextImageKHR / vkQueuePresentKHR
        if (mRenderWindow->IsAppActive())
        {
            RenderFrame();
        }
        FrameMark;
    }

    // Flush pipeline cache to disk before RenderWindow is destroyed
    // (VKRenderDevice's destructor may not be called in time due to static shared_ptr)
    RenderCore::DestroyRenderDevice();

    // 释放持有 RenderDevice 引用的对象，确保 VKRenderDevice refcount 归零
    mRenderWindow.reset();
    gRenderWindow.reset();
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
    if (!renderDevice)
    {
        return;
    }
    // 从Graphics队列创建命令缓冲区
    RenderCore::CommandQueuePtr graphicsQueue = renderDevice->GetCommandQueue(RenderCore::QueueType::Graphics, 0);
    if (!graphicsQueue)
    {
        return;
    }
    RenderCore::CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        // Android 前后台切换时 swapchain/surface 未就绪，本帧跳过（等 OnWindowRestored）
        return;
    }
    RenderCore::RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    if (renderEncoder)
    {
        renderEncoder->EndEncode();
    }
    commandBuffer->PresentFrameBuffer();
}

void AppFrameWork::OnEvent(Event& e)
{
    LOG_INFO("%s", e.ToString().c_str());
    
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(GNX_BIND_EVENT_FN(OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(GNX_BIND_EVENT_FN(OnWindowResize));
    
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);
}

void AppFrameWork::OnEventImpl(Event& e)
{
    OnEvent(e);
}

void AppFrameWork::SetVSync(bool enable)
{
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    if (renderDevice)
    {
        renderDevice->SetVSync(enable);
    }
}

bool AppFrameWork::IsVSync() const
{
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    if (renderDevice)
    {
        return renderDevice->IsVSync();
    }
    return false;
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
