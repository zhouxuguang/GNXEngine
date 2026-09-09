#include "mainwindow.h"
#include "AssetImportService.h"
#include "ContentWidget.h"
#include "EditorContext.h"
#include "EditorProjectService.h"
#include "EditorRenderHost.h"
#include "EditorSettings.h"
#include "EditorViewportWidget.h"
#include "NewProjectDialog.h"
#include "OpenProjectDialog.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "ui_mainwindow.h"
#include <QAction>
#include <QDockWidget>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
MainWindow::MainWindow(EditorContext &context, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), mContext(context) {
  ui->setupUi(this);
  SetupMenu();
  mViewport = new EditorViewportWidget(*mContext.renderHost, this);
  setCentralWidget(mViewport);
  SetupDock();
  connect(mContext.projectService.get(), &EditorProjectService::ProjectOpened,
          this, &MainWindow::OnProjectOpened);
  connect(mContext.projectService.get(), &EditorProjectService::ProjectClosed,
          this, &MainWindow::OnProjectClosed);
  connect(mContext.projectService.get(),
          &EditorProjectService::ProjectOperationFailed, this,
          &MainWindow::OnProjectError);
  connect(mContext.assetImportService.get(), &AssetImportService::ImportFailed,
          this, [this](const QString &f, const QString &e) {
            QMessageBox::warning(this, tr("资源导入失败"), f + "\n" + e);
          });
  if (!mContext.settings->WindowGeometry().isEmpty())
    restoreGeometry(mContext.settings->WindowGeometry());
  else
    setWindowState(Qt::WindowMaximized);
  if (!mContext.settings->WindowState().isEmpty())
    restoreState(mContext.settings->WindowState());
  UpdateProjectUi();
}
MainWindow::~MainWindow() { delete ui; }
void MainWindow::SetupMenu() {
  auto *menu = menuBar()->addMenu(tr("文件"));
  menu->addAction(tr("新建工程"), this, &MainWindow::OnNewProject,
                  QKeySequence::New);
  menu->addAction(tr("打开工程"), this, &MainWindow::OnOpenProject,
                  QKeySequence::Open);
  menu->addSeparator();
  mSaveAction = menu->addAction(tr("保存工程"), this,
                                &MainWindow::OnSaveProject, QKeySequence::Save);
  mCloseAction =
      menu->addAction(tr("关闭工程"), this, &MainWindow::OnCloseProject);
  menu->addSeparator();
  menu->addAction(tr("退出"), this, &QWidget::close, QKeySequence::Quit);
}
void MainWindow::SetupDock() {
  mSceneDockWidget = new QDockWidget(tr("Hierarchy"), this);
  mContentDockWidget = new QDockWidget(tr("Content Browser"), this);
  mDetailDockWidget = new QDockWidget(tr("Detail"), this);
  const QString root = mContext.projectService->HasOpenProject()
                           ? mContext.projectService->AssetRoot()
                           : QString();
  mContentWidget =
      new ContentWidget(mContentDockWidget, root, *mContext.assetImportService,
                        *mContext.settings);
  addDockWidget(Qt::LeftDockWidgetArea, mSceneDockWidget);
  addDockWidget(Qt::BottomDockWidgetArea, mContentDockWidget);
  addDockWidget(Qt::RightDockWidgetArea, mDetailDockWidget);
}
void MainWindow::OnNewProject() {
  NewProjectDialog d(this);
  if (d.exec() != QDialog::Accepted)
    return;
  ProjectCreateRequest r{d.GetProjectPath(), d.GetProjectName()};
  if (mContext.projectService->CreateProject(r))
    QMessageBox::information(this, tr("提示"), tr("工程创建成功！"));
}
void MainWindow::OnOpenProject() {
  OpenProjectDialog d(*mContext.settings, this);
  if (d.exec() != QDialog::Accepted)
    return;
  if (mContext.projectService->OpenProject(d.GetSelectedProjectPath()))
    mContext.settings->AddRecentProject(d.GetSelectedProjectPath());
}
void MainWindow::OnSaveProject() {
  if (!mContext.projectService->HasOpenProject()) {
    QMessageBox::warning(this, tr("警告"), tr("当前没有打开的工程！"));
    return;
  }
  if (mContext.projectService->SaveProject())
    QMessageBox::information(this, tr("成功"), tr("工程保存成功！"));
}
void MainWindow::OnCloseProject() {
  if (!mContext.projectService->HasOpenProject())
    return;
  if (QMessageBox::question(this, tr("确认"), tr("确定关闭当前工程吗？")) ==
      QMessageBox::Yes)
    mContext.projectService->CloseProject();
}
void MainWindow::OnProjectOpened(const QString &) {
  mContentWidget->SetRootPath(mContext.projectService->AssetRoot());
  UpdateProjectUi();
}
void MainWindow::OnProjectClosed() {
  mContentWidget->SetRootPath(QString());
  UpdateProjectUi();
}
void MainWindow::OnProjectError(const QString &m) {
  QMessageBox::critical(this, tr("工程操作失败"), m);
}
void MainWindow::UpdateProjectUi() {
  const bool open = mContext.projectService->HasOpenProject();
  mSaveAction->setEnabled(open);
  mCloseAction->setEnabled(open);
  setWindowTitle(open ? mContext.projectService->ProjectName() + " - GNXEngine"
                      : QStringLiteral("GNXEngine"));
}
