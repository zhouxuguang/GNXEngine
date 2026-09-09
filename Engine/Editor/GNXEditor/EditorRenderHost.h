#pragma once
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include <cstdint>
class EditorRenderHost final {
public:
  bool Attach(void* nativeHandle, uint32_t width, uint32_t height); void Detach();
  void Resize(uint32_t width, uint32_t height); void Tick(float deltaSeconds);
  void ForwardEvent(GNXEngine::Event& event); bool IsAttached() const { return mRenderWindow != nullptr; }
  GNXEngine::RenderWindowPtr RenderWindow() const { return mRenderWindow; }
private:
  GNXEngine::RenderWindowPtr mRenderWindow;
  uint32_t mPendingWidth=0, mPendingHeight=0; bool mResizePending=false;
};
