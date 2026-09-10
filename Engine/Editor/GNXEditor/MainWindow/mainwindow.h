#pragma once
#include <QMainWindow>
class QAction;
class ContentWidget;
class EditorContext;
class EditorViewportWidget;
class QDockWidget;
class QCloseEvent;
namespace Ui
{
class MainWindow;
}
class MainWindow final : public QMainWindow
{
    Q_OBJECT
  public:
    explicit MainWindow(EditorContext &context, QWidget *parent = nullptr);
    ~MainWindow() override;

  protected:
    void closeEvent(QCloseEvent *event) override;
  private slots:
    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnCloseProject();
    void OnProjectOpened(const QString &root);
    void OnProjectClosed();
    void OnProjectError(const QString &message);
    void OnSaveSceneAs();

  private:
    void SetupMenu();
    void SetupDock();
    void UpdateProjectUi();
    bool MaybeSaveScene();
    bool SaveScene();
    Ui::MainWindow *ui = nullptr;
    EditorContext &mContext;
    EditorViewportWidget *mViewport = nullptr;
    QDockWidget *mSceneDockWidget = nullptr, *mContentDockWidget = nullptr,
                *mDetailDockWidget = nullptr;
    ContentWidget *mContentWidget = nullptr;
    QAction *mSaveAction = nullptr, *mCloseAction = nullptr;
    QAction *mSaveSceneAction = nullptr;
};
