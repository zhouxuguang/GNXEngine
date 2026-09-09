//
//  OpenProjectDialog.h
//  GNXEngine
//
//  打开工程对话框
//

#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class EditorSettings;
class OpenProjectDialog : public QDialog {
  Q_OBJECT

public:
  explicit OpenProjectDialog(EditorSettings &settings,
                             QWidget *parent = nullptr);
  ~OpenProjectDialog();

  // 获取选中的工程路径
  QString GetSelectedProjectPath() const;

private slots:
  void OnBrowseButtonClicked();
  void OnOpenButtonClicked();
  void OnCancelButtonClicked();
  void OnProjectSelectionChanged();

private:
  void SetupUI();
  void LoadRecentProjects();

  QListWidget *mRecentProjectsList = nullptr;
  QPushButton *mBrowseButton = nullptr;
  QPushButton *mOpenButton = nullptr;
  QPushButton *mCancelButton = nullptr;

  QLabel *mStatusLabel = nullptr;

  QString mSelectedProjectPath;
  EditorSettings &mSettings;
};
