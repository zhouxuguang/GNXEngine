#pragma once
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

enum class EditorRenderState
{
    Detached,
    Attaching,
    Ready,
    SurfaceLost,
    DeviceLost,
    Failed
};

class EditorRenderHost final
{
  public:
    using WindowFactory =
        std::function<GNXEngine::RenderWindowPtr(const GNXEngine::WindowProps &, void *)>;
    explicit EditorRenderHost(WindowFactory windowFactory = {});
    bool Attach(void *nativeHandle, uint32_t width, uint32_t height);
    void Detach();
    void Resize(uint32_t width, uint32_t height);
    void Tick(float deltaSeconds);
    void ForwardEvent(GNXEngine::Event &event);
    bool IsAttached() const
    {
        return mState == EditorRenderState::Ready;
    }
    EditorRenderState State() const
    {
        return mState;
    }
    const std::string &LastError() const
    {
        return mLastError;
    }
    void MarkSurfaceLost();
    void MarkDeviceLost(const std::string &message);
    void SetErrorCallback(std::function<void(const std::string &)> callback)
    {
        mErrorCallback = std::move(callback);
    }
    GNXEngine::RenderWindowPtr RenderWindow() const
    {
        return mRenderWindow;
    }

  private:
    GNXEngine::RenderWindowPtr mRenderWindow;
    uint32_t mPendingWidth = 0, mPendingHeight = 0;
    bool mResizePending = false;
    EditorRenderState mState = EditorRenderState::Detached;
    std::string mLastError;
    std::function<void(const std::string &)> mErrorCallback;
    WindowFactory mWindowFactory;
};
