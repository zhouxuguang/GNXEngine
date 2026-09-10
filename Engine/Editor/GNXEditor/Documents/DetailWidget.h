#pragma once

#include <QWidget>
class QLabel;
class QLineEdit;
class SceneDocument;
class SelectionService;

class DetailWidget final : public QWidget
{
    Q_OBJECT
  public:
    DetailWidget(SceneDocument &document, SelectionService &selection, QWidget *parent = nullptr);

  private:
    void Refresh();
    SceneDocument &mDocument;
    SelectionService &mSelection;
    QLabel *mId = nullptr;
    QLineEdit *mName = nullptr;
    bool mRefreshing = false;
};
