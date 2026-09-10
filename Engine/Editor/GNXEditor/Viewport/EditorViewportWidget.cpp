#include "EditorViewportWidget.h"
#include "EditorInputAdapter.h"
#include "EditorRenderHost.h"
#include "NativeSurface.h"
#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QWheelEvent>
EditorViewportWidget::EditorViewportWidget(EditorRenderHost &host, QWidget *parent)
    : QWidget(parent), mHost(host), mInput(std::make_unique<EditorInputAdapter>(host))
{
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    qApp->installEventFilter(this);
    mHost.SetErrorCallback(
        [this](const std::string &message)
        {
            emit RenderError(QString::fromStdString(message));
        });
}
EditorViewportWidget::~EditorViewportWidget()
{
    qApp->removeEventFilter(this);
    mHost.SetErrorCallback({});
    mInput->ClearState();
    mHost.Detach();
}
void EditorViewportWidget::AttachIfReady()
{
    if (mAttachInProgress || !winId() || width() <= 0 || height() <= 0)
        return;
    void *handle = NativeSurface::Handle(*this);
    const qreal scale = devicePixelRatioF();
    mInput->SetDevicePixelRatio(scale);
    const EditorRenderState state = mHost.State();
    const bool surfaceChanged = handle != mNativeHandle;
    const bool canAttach =
        state == EditorRenderState::Detached || state == EditorRenderState::SurfaceLost;
    if (surfaceChanged || canAttach)
    {
        mAttachInProgress = true;
        mInput->ClearState();
        mHost.Detach();
        mNativeHandle = handle;
        mHost.Attach(handle, uint32_t(width() * scale), uint32_t(height() * scale));
        mAttachInProgress = false;
    }
}
bool EditorViewportWidget::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface)
    {
        auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(e);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
        {
            mInput->ClearState();
            mHost.MarkSurfaceLost();
            mNativeHandle = nullptr;
        }
        else if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated)
            AttachIfReady();
    }
    if (e->type() == QEvent::WinIdChange || e->type() == QEvent::Show ||
        e->type() == QEvent::ScreenChangeInternal)
        AttachIfReady();
    if (e->type() == QEvent::WindowDeactivate)
        mInput->ClearState();
    return QWidget::event(e);
}
bool EditorViewportWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && event->type() == QEvent::ApplicationDeactivate)
        mInput->ClearState();
    return QWidget::eventFilter(watched, event);
}
void EditorViewportWidget::resizeEvent(QResizeEvent *e)
{
    const qreal scale = devicePixelRatioF();
    mHost.Resize(uint32_t(e->size().width() * scale), uint32_t(e->size().height() * scale));
    QWidget::resizeEvent(e);
}
void EditorViewportWidget::keyPressEvent(QKeyEvent *e)
{
    mInput->KeyPress(*e);
    QWidget::keyPressEvent(e);
}
void EditorViewportWidget::keyReleaseEvent(QKeyEvent *e)
{
    mInput->KeyRelease(*e);
    QWidget::keyReleaseEvent(e);
}
void EditorViewportWidget::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    mInput->MousePress(*e);
    QWidget::mousePressEvent(e);
}
void EditorViewportWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mInput->MouseRelease(*e);
    QWidget::mouseReleaseEvent(e);
}
void EditorViewportWidget::mouseMoveEvent(QMouseEvent *e)
{
    mInput->MouseMove(*e);
    QWidget::mouseMoveEvent(e);
}
void EditorViewportWidget::wheelEvent(QWheelEvent *e)
{
    mInput->Wheel(*e);
    QWidget::wheelEvent(e);
}
void EditorViewportWidget::focusOutEvent(QFocusEvent *e)
{
    mInput->ClearState();
    QWidget::focusOutEvent(e);
}
void EditorViewportWidget::leaveEvent(QEvent *e)
{
    if (!(QApplication::mouseButtons()))
        mInput->ClearState();
    QWidget::leaveEvent(e);
}
void EditorViewportWidget::hideEvent(QHideEvent *e)
{
    mInput->ClearState();
    QWidget::hideEvent(e);
}
