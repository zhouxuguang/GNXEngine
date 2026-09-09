#include "EditorViewportWidget.h"
#include "EditorInputAdapter.h"
#include "EditorRenderHost.h"
#include "NativeSurface.h"
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>
EditorViewportWidget::EditorViewportWidget(EditorRenderHost &host,
                                           QWidget *parent)
    : QWidget(parent), mHost(host),
      mInput(std::make_unique<EditorInputAdapter>(host)) {
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_DontCreateNativeAncestors, true);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_PaintOnScreen, true);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}
EditorViewportWidget::~EditorViewportWidget() = default;
void EditorViewportWidget::AttachIfReady() {
  if (!mHost.IsAttached() && winId()) {
    const qreal scale = devicePixelRatioF();
    mHost.Attach(NativeSurface::Handle(*this), uint32_t(width() * scale),
                 uint32_t(height() * scale));
  }
}
bool EditorViewportWidget::event(QEvent *e) {
  if (e->type() == QEvent::WinIdChange || e->type() == QEvent::Show)
    AttachIfReady();
  return QWidget::event(e);
}
void EditorViewportWidget::resizeEvent(QResizeEvent *e) {
  const qreal scale = devicePixelRatioF();
  mHost.Resize(uint32_t(e->size().width() * scale),
               uint32_t(e->size().height() * scale));
  QWidget::resizeEvent(e);
}
void EditorViewportWidget::keyPressEvent(QKeyEvent *e) {
  mInput->KeyPress(*e);
  QWidget::keyPressEvent(e);
}
void EditorViewportWidget::keyReleaseEvent(QKeyEvent *e) {
  mInput->KeyRelease(*e);
  QWidget::keyReleaseEvent(e);
}
void EditorViewportWidget::mousePressEvent(QMouseEvent *e) {
  setFocus();
  mInput->MousePress(*e);
  QWidget::mousePressEvent(e);
}
void EditorViewportWidget::mouseReleaseEvent(QMouseEvent *e) {
  mInput->MouseRelease(*e);
  QWidget::mouseReleaseEvent(e);
}
void EditorViewportWidget::mouseMoveEvent(QMouseEvent *e) {
  mInput->MouseMove(*e);
  QWidget::mouseMoveEvent(e);
}
void EditorViewportWidget::wheelEvent(QWheelEvent *e) {
  mInput->Wheel(*e);
  QWidget::wheelEvent(e);
}
void EditorViewportWidget::focusOutEvent(QFocusEvent *e) {
  mInput->ClearState();
  QWidget::focusOutEvent(e);
}
