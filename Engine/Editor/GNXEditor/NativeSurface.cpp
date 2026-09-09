#include "NativeSurface.h"
#include <QWidget>
#if defined(Q_OS_MAC)
#include "macUtils.h"
#endif
void* NativeSurface::Handle(QWidget& widget) {
#if defined(Q_OS_WIN)
  return reinterpret_cast<void*>(widget.winId());
#elif defined(Q_OS_MAC)
  return GetMetalLayer(widget.winId());
#else
  Q_UNUSED(widget); return nullptr;
#endif
}
