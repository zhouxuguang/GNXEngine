#include "EditorRenderHost.h"
#include "Runtime/GNXEngine/include/Events/ApplicationEvent.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
bool EditorRenderHost::Attach(void *handle, uint32_t width, uint32_t height) {
  if (mRenderWindow || !handle || width == 0 || height == 0)
    return mRenderWindow != nullptr;
  GNXEngine::WindowProps props("GNXEditor Viewport", width, height);
  mRenderWindow =
      GNXEngine::RenderWindow::CreateWithExternalWindow(props, handle);
  return mRenderWindow != nullptr;
}
void EditorRenderHost::Detach() {
  mRenderWindow.reset();
  mResizePending = false;
}
void EditorRenderHost::Resize(uint32_t width, uint32_t height) {
  if (!width || !height)
    return;
  mPendingWidth = width;
  mPendingHeight = height;
  mResizePending = true;
}
void EditorRenderHost::Tick(float dt) {
  if (!mRenderWindow)
    return;
  if (mResizePending) {
    mRenderWindow->Resize(mPendingWidth, mPendingHeight);
    GNXEngine::WindowResizeEvent e(mPendingWidth, mPendingHeight);
    ForwardEvent(e);
    mResizePending = false;
  }
  auto *scene = RenderSystem::SceneManager::GetInstance();
  scene->Update(dt);
  mRenderWindow->OnUpdate();
  scene->Render(nullptr);
}
void EditorRenderHost::ForwardEvent(GNXEngine::Event &event) {
  if (mRenderWindow)
    mRenderWindow->TriggerEventCallback(event);
}
