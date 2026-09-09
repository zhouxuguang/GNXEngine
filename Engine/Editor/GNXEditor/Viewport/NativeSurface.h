#pragma once
#include <QtGlobal>
class QWidget;
class NativeSurface final {
public:
  static void *Handle(QWidget &widget);
};
