#include "EditorInputAdapter.h"
#include "EditorRenderHost.h"
#include "Runtime/GNXEngine/include/InputState.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
int EditorInputAdapter::MapButton(Qt::MouseButton b) { return b==Qt::LeftButton?0:b==Qt::RightButton?1:b==Qt::MiddleButton?2:-1; }
int EditorInputAdapter::MapKey(int k) {
  if (k>=Qt::Key_0&&k<=Qt::Key_9) return 48+(k-Qt::Key_0); if(k>=Qt::Key_A&&k<=Qt::Key_Z) return 65+(k-Qt::Key_A);
  switch(k) { case Qt::Key_Space:return 32; case Qt::Key_Escape:return 256; case Qt::Key_Return: case Qt::Key_Enter:return 257;
    case Qt::Key_Tab:return 258; case Qt::Key_Backspace:return 259; case Qt::Key_Insert:return 260; case Qt::Key_Delete:return 261;
    case Qt::Key_Right:return 262; case Qt::Key_Left:return 263; case Qt::Key_Down:return 264; case Qt::Key_Up:return 265;
    case Qt::Key_PageUp:return 266; case Qt::Key_PageDown:return 267; case Qt::Key_Home:return 268; case Qt::Key_End:return 269;
    case Qt::Key_Shift:return 340; case Qt::Key_Control:return 341; case Qt::Key_Alt:return 342; case Qt::Key_Meta:return 343; default:return -1; }
}
void EditorInputAdapter::KeyPress(QKeyEvent& q) { int k=MapKey(q.key()); if(k<0)return; mPressedKeys.insert(k); GNXEngine::InputState::GetInstance().SetKeyState(k,true); GNXEngine::KeyPressedEvent e(k,q.isAutoRepeat()); mHost.ForwardEvent(e); }
void EditorInputAdapter::KeyRelease(QKeyEvent& q) { int k=MapKey(q.key()); if(k<0)return; mPressedKeys.remove(k); GNXEngine::InputState::GetInstance().SetKeyState(k,false); GNXEngine::KeyReleasedEvent e(k); mHost.ForwardEvent(e); }
void EditorInputAdapter::MousePress(QMouseEvent& q) { int b=MapButton(q.button()); if(b<0)return; mPressedButtons.insert(b); GNXEngine::InputState::GetInstance().SetMouseButtonState(b,true); GNXEngine::MouseButtonPressedEvent e(b); mHost.ForwardEvent(e); }
void EditorInputAdapter::MouseRelease(QMouseEvent& q) { int b=MapButton(q.button()); if(b<0)return; mPressedButtons.remove(b); GNXEngine::InputState::GetInstance().SetMouseButtonState(b,false); GNXEngine::MouseButtonReleasedEvent e(b); mHost.ForwardEvent(e); }
void EditorInputAdapter::MouseMove(QMouseEvent& q) { const auto p=q.position(); GNXEngine::InputState::GetInstance().SetMousePosition(float(p.x()),float(p.y())); GNXEngine::MouseMovedEvent e(float(p.x()),float(p.y())); mHost.ForwardEvent(e); }
void EditorInputAdapter::Wheel(QWheelEvent& q) { auto d=q.angleDelta(); GNXEngine::InputState::GetInstance().UpdateMouseScroll(float(d.x()),float(d.y())); GNXEngine::MouseScrolledEvent e(float(d.x()),float(d.y())); mHost.ForwardEvent(e); }
void EditorInputAdapter::ClearState() { for(int k:mPressedKeys){GNXEngine::InputState::GetInstance().SetKeyState(k,false);GNXEngine::KeyReleasedEvent e(k);mHost.ForwardEvent(e);} for(int b:mPressedButtons){GNXEngine::InputState::GetInstance().SetMouseButtonState(b,false);GNXEngine::MouseButtonReleasedEvent e(b);mHost.ForwardEvent(e);} mPressedKeys.clear();mPressedButtons.clear(); }
