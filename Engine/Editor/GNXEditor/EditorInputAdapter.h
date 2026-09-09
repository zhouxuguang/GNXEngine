#pragma once
#include <QSet>
#include <Qt>
class EditorRenderHost;
class QKeyEvent; class QMouseEvent; class QWheelEvent;
class EditorInputAdapter final {
public:
  explicit EditorInputAdapter(EditorRenderHost& host) : mHost(host) {}
  void KeyPress(QKeyEvent&); void KeyRelease(QKeyEvent&); void MousePress(QMouseEvent&);
  void MouseRelease(QMouseEvent&); void MouseMove(QMouseEvent&); void Wheel(QWheelEvent&); void ClearState();
private:
  static int MapKey(int key); static int MapButton(Qt::MouseButton button); EditorRenderHost& mHost;
  QSet<int> mPressedKeys, mPressedButtons;
};
