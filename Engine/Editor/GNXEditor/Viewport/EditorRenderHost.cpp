#include "EditorRenderHost.h"
#include "Runtime/GNXEngine/include/Events/ApplicationEvent.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include <exception>
EditorRenderHost::EditorRenderHost(WindowFactory windowFactory)
    : mWindowFactory(std::move(windowFactory))
{
    if (!mWindowFactory)
        mWindowFactory = [](const GNXEngine::WindowProps &props, void *handle)
        {
            return GNXEngine::RenderWindow::CreateWithExternalWindow(props, handle);
        };
}
bool EditorRenderHost::Attach(void *handle, uint32_t width, uint32_t height)
{
    if (!handle || width == 0 || height == 0)
        return false;
    Detach();
    mState = EditorRenderState::Attaching;
    try
    {
        GNXEngine::WindowProps props("GNXEditor Viewport", width, height);
        mRenderWindow = mWindowFactory(props, handle);
        if (!mRenderWindow)
        {
            mState = EditorRenderState::Failed;
            mLastError = "RenderWindow attachment failed";
            if (mErrorCallback)
                mErrorCallback(mLastError);
            return false;
        }
        mLastError.clear();
        mState = EditorRenderState::Ready;
        EnsureSceneCamera(width, height);
        return true;
    }
    catch (const std::exception &error)
    {
        mRenderWindow.reset();
        mState = EditorRenderState::Failed;
        mLastError = error.what();
        if (mErrorCallback)
            mErrorCallback(mLastError);
        return false;
    }
    catch (...)
    {
        mRenderWindow.reset();
        mState = EditorRenderState::Failed;
        mLastError = "Unknown render attachment error";
        if (mErrorCallback)
            mErrorCallback(mLastError);
        return false;
    }
}
void EditorRenderHost::Detach()
{
    mRenderWindow.reset();
    mResizePending = false;
    mState = EditorRenderState::Detached;
}
void EditorRenderHost::MarkSurfaceLost()
{
    mRenderWindow.reset();
    mResizePending = false;
    mState = EditorRenderState::SurfaceLost;
}
void EditorRenderHost::MarkDeviceLost(const std::string &message)
{
    mRenderWindow.reset();
    mResizePending = false;
    mLastError = message;
    mState = EditorRenderState::DeviceLost;
    if (mErrorCallback)
        mErrorCallback(mLastError);
}
void EditorRenderHost::Resize(uint32_t width, uint32_t height)
{
    if (!width || !height)
        return;
    mPendingWidth = width;
    mPendingHeight = height;
    mResizePending = true;
}
void EditorRenderHost::Tick(float dt)
{
    if (!mRenderWindow || mState != EditorRenderState::Ready)
        return;
    try
    {
        if (mResizePending)
        {
            mRenderWindow->Resize(mPendingWidth, mPendingHeight);
            GNXEngine::WindowResizeEvent e(mPendingWidth, mPendingHeight);
            ForwardEvent(e);
            EnsureSceneCamera(mPendingWidth, mPendingHeight);
            mResizePending = false;
        }
        auto *scene = RenderSystem::SceneManager::GetInstance();
        if (!scene)
            return;
        scene->Update(dt);
        mRenderWindow->OnUpdate();
        scene->Render(nullptr);
    }
    catch (const std::exception &error)
    {
        MarkDeviceLost(error.what());
    }
    catch (...)
    {
        MarkDeviceLost("Unknown render error");
    }
}
void EditorRenderHost::ForwardEvent(GNXEngine::Event &event)
{
    if (mRenderWindow)
        mRenderWindow->TriggerEventCallback(event);
}

void EditorRenderHost::EnsureSceneCamera(uint32_t width, uint32_t height)
{
    auto *scene = RenderSystem::SceneManager::GetInstance();
    if (!scene)
        return;

    RenderSystem::CameraPtr camera = scene->GetCamera("MainCamera");
    if (!camera)
    {
        // 与原先 GLFWRenderWindow 外部窗口分支里的默认相机保持一致
        camera = scene->CreateCamera("MainCamera");
        camera->LookAt(mathutil::Vector3f(0.0f, 0.0f, 5.0f),
                       mathutil::Vector3f(0.0f, 0.0f, 0.0f),
                       mathutil::Vector3f(0.0f, 1.0f, 0.0f));
    }

    if (width > 0 && height > 0)
    {
        camera->SetLens(60.0f, width, height, 0.1f, 1000.0f);
    }
}
