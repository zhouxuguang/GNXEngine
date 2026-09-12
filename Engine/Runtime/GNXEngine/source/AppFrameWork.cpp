#include "AppFrameWork.h"
#include "Runtime/BaseLib/include/DateTime.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include <tracy/Tracy.hpp>

NAMESPACE_GNXENGINE_BEGIN

RenderWindowPtr gRenderWindow = nullptr;

RenderWindowPtr GetRenderWindow()
{
    return gRenderWindow;
}

AppFrameWork::AppFrameWork(const WindowProps& props)
{
    // 初始化 AssetManager（统一管理 .gnxasset 资源，含预编译 shader）
    // 必须在 RenderWindow 创建之前，因为 GLFWRenderWindow 构造时会触发
    // SceneManager/PostProcessing 初始化 → LoadShaderAsset，需要 AssetManager 已就绪
    if (!AssetManager::AssetManager::GetInstance())
    {
        AssetManager::AssetManager::Initialize(GetProjectAssetDir());
    }

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
            UpdateImGuiFrame();
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

RenderSystem::ImGuiRendererPtr AppFrameWork::GetImGui()
{
    if (!mImGuiEnabled)
    {
        return nullptr;
    }
    return RenderSystem::SceneManager::GetInstance()->GetImGuiRenderer();
}

void AppFrameWork::UpdateImGuiFrame()
{
    if (!mImGuiEnabled || !mRenderWindow)
    {
        return;
    }

    const uint64_t now = baselib::GetTickNanoSeconds();
    if (mLastFrameTick != 0)
    {
        mDeltaTime = (float)(now - mLastFrameTick) * 1e-9f;
        // 夹取异常值（例如断点/后台切回）
        if (mDeltaTime <= 0.0f || mDeltaTime > 0.5f)
        {
            mDeltaTime = 1.0f / 60.0f;
        }
    }
    mLastFrameTick = now;

    if (RenderSystem::ImGuiRendererPtr imgui = GetImGui())
    {
        // 窗口尺寸为帧缓冲像素尺寸；DPI 缩放由 demo/平台层设置
        imgui->NewFrame(mDeltaTime, mRenderWindow->GetWidth(), mRenderWindow->GetHeight());
    }
}

void AppFrameWork::OnEvent(Event& e)
{
    LOG_INFO("%s", e.ToString().c_str());
    
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(GNX_BIND_EVENT_FN(OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(GNX_BIND_EVENT_FN(OnWindowResize));

    // ImGui 优先消费输入事件：当 UI 捕获鼠标/键盘时，事件不再传递给 3D 场景，
    // 避免「点击 UI 面板的同时也在操作相机」这类问题。
    if (mImGuiEnabled)
    {
        if (RenderSystem::ImGuiRendererPtr imgui = GetImGui())
        {
            if (imgui->OnEvent(e))
            {
                // 事件已被 UI 消费（e.handled 已置位），调用方据此跳过自身处理
                return;
            }
        }
    }
    
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
    // 调用虚函数 Resize()（会触发子类 TerrainFrameWork::Resize 更新相机纵横比），
    // 而不是只调 mRenderWindow->Resize() —— 否则横屏旋转后窗口/swapchain 尺寸
    // 更新了但相机 aspect 仍是旧的，画面比例不对（"横屏但内容像竖屏"）。
    Resize(e.GetWidth(), e.GetHeight());

    return false;
}

NAMESPACE_GNXENGINE_END
