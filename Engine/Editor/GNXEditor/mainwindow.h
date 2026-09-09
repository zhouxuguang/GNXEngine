#pragma once
#include <QMainWindow>
class QAction; class ContentWidget; class EditorContext; class EditorViewportWidget; class QDockWidget;
namespace Ui { class MainWindow; }
class MainWindow final : public QMainWindow {
  Q_OBJECT
public: explicit MainWindow(EditorContext& context,QWidget* parent=nullptr); ~MainWindow() override;
private slots: void OnNewProject(); void OnOpenProject(); void OnSaveProject(); void OnCloseProject();
  void OnProjectOpened(const QString& root); void OnProjectClosed(); void OnProjectError(const QString& message);
private: void SetupMenu(); void SetupDock(); void UpdateProjectUi();
  Ui::MainWindow* ui=nullptr; EditorContext& mContext; EditorViewportWidget* mViewport=nullptr;
  QDockWidget *mSceneDockWidget=nullptr,*mContentDockWidget=nullptr,*mDetailDockWidget=nullptr;
  ContentWidget* mContentWidget=nullptr; QAction *mSaveAction=nullptr,*mCloseAction=nullptr;
};
