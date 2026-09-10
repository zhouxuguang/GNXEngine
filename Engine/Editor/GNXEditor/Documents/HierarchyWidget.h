#pragma once

#include <QWidget>
class QListWidget;
class SceneDocument;
class SelectionService;

class HierarchyWidget final : public QWidget
{
    Q_OBJECT
  public:
    HierarchyWidget(SceneDocument &document, SelectionService &selection,
                    QWidget *parent = nullptr);

  private:
    void Refresh();
    SceneDocument &mDocument;
    SelectionService &mSelection;
    QListWidget *mList = nullptr;
};
