#include "mainwindow.h"
#include "Application/EditorContext.h"
#include "Application/EditorSettings.h"
#include "Assets/AssetImportService.h"
#include "Assets/ContentWidget.h"
#include "Documents/CommandHistory.h"
#include "Documents/DetailWidget.h"
#include "Documents/HierarchyWidget.h"
#include "Documents/SceneDocument.h"
#include "Documents/SelectionService.h"
#include "Project/EditorProjectService.h"
#include "Project/NewProjectDialog.h"
#include "Project/OpenProjectDialog.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Viewport/EditorRenderHost.h"
#include "Viewport/EditorViewportWidget.h"
#include "ui_mainwindow.h"
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
namespace
{
bool IsWithinDirectory(const QString &path, const QString &root)
{
    const QString normalizedPath =
        QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
    const QString normalizedRoot =
        QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(root).absoluteFilePath()));
#if defined(Q_OS_WIN)
    constexpr Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
    return normalizedPath.compare(normalizedRoot, pathCase) == 0 ||
           normalizedPath.startsWith(normalizedRoot + '/', pathCase);
}
} // namespace
MainWindow::MainWindow(EditorContext &context, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), mContext(context)
{
    ui->setupUi(this);
    SetupMenu();
    mViewport = new EditorViewportWidget(*mContext.renderHost, this);
    connect(mViewport, &EditorViewportWidget::RenderError, this,
            [this](const QString &message)
            {
                statusBar()->showMessage(tr("视口渲染已停止：%1").arg(message));
            });
    setCentralWidget(mViewport);
    SetupDock();
    connect(mContext.projectService.get(), &EditorProjectService::ProjectOpened, this,
            &MainWindow::OnProjectOpened);
    connect(mContext.projectService.get(), &EditorProjectService::ProjectClosed, this,
            &MainWindow::OnProjectClosed);
    connect(mContext.projectService.get(), &EditorProjectService::ProjectOperationFailed, this,
            &MainWindow::OnProjectError);
    connect(mContext.assetImportService.get(), &AssetImportService::ImportFailed, this,
            [this](const QString &f, const QString &e)
            {
                QMessageBox::warning(this, tr("资源导入失败"), f + "\n" + e);
            });
    connect(mContext.assetImportService.get(), &AssetImportService::ImportStarted, this,
            [this](const QString &file)
            {
                statusBar()->showMessage(tr("正在导入 %1").arg(QFileInfo(file).fileName()));
            });
    connect(mContext.assetImportService.get(), &AssetImportService::ImportQueueChanged, this,
            [this](int pending, bool running)
            {
                if (pending > 0 || running)
                    statusBar()->showMessage(
                        tr("资源导入任务：%1 个等待，%2")
                            .arg(pending)
                            .arg(running ? tr("1 个执行中") : tr("无执行任务")));
                else
                    statusBar()->showMessage(tr("资源导入队列已完成"), 3000);
            });
    if (!mContext.settings->WindowGeometry().isEmpty())
        restoreGeometry(mContext.settings->WindowGeometry());
    else
        setWindowState(Qt::WindowMaximized);
    if (!mContext.settings->WindowState().isEmpty())
        restoreState(mContext.settings->WindowState());
    UpdateProjectUi();
}
MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::SetupMenu()
{
    auto *menu = menuBar()->addMenu(tr("文件"));
    menu->addAction(tr("新建工程"), this, &MainWindow::OnNewProject, QKeySequence::New);
    menu->addAction(tr("打开工程"), this, &MainWindow::OnOpenProject, QKeySequence::Open);
    menu->addSeparator();
    mSaveAction =
        menu->addAction(tr("保存工程"), this, &MainWindow::OnSaveProject, QKeySequence::Save);
    mCloseAction = menu->addAction(tr("关闭工程"), this, &MainWindow::OnCloseProject);
    mSaveSceneAction =
        menu->addAction(tr("场景另存为"), this, &MainWindow::OnSaveSceneAs, QKeySequence::SaveAs);
    menu->addSeparator();
    menu->addAction(tr("退出"), this, &QWidget::close, QKeySequence::Quit);
    auto *editMenu = menuBar()->addMenu(tr("编辑"));
    auto *undoAction = editMenu->addAction(tr("撤销"), mContext.commandHistory.get(),
                                           &CommandHistory::Undo, QKeySequence::Undo);
    auto *redoAction = editMenu->addAction(tr("重做"), mContext.commandHistory.get(),
                                           &CommandHistory::Redo, QKeySequence::Redo);
    connect(mContext.commandHistory.get(), &CommandHistory::StateChanged, this,
            [this, undoAction, redoAction]
            {
                undoAction->setEnabled(mContext.commandHistory->CanUndo());
                redoAction->setEnabled(mContext.commandHistory->CanRedo());
            });
    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
}
void MainWindow::SetupDock()
{
    mSceneDockWidget = new QDockWidget(tr("Hierarchy"), this);
    mContentDockWidget = new QDockWidget(tr("Content Browser"), this);
    mDetailDockWidget = new QDockWidget(tr("Detail"), this);
    const QString root = mContext.projectService->HasOpenProject()
                             ? mContext.projectService->AssetRoot()
                             : QString();
    mContentWidget =
        new ContentWidget(mContentDockWidget, root, *mContext.assetImportService,
                          *mContext.assetRegistry, *mContext.thumbnailService, *mContext.settings);
    mSceneDockWidget->setWidget(
        new HierarchyWidget(*mContext.sceneDocument, *mContext.selectionService, mSceneDockWidget));
    mDetailDockWidget->setWidget(
        new DetailWidget(*mContext.sceneDocument, *mContext.selectionService, mDetailDockWidget));
    addDockWidget(Qt::LeftDockWidgetArea, mSceneDockWidget);
    addDockWidget(Qt::BottomDockWidgetArea, mContentDockWidget);
    addDockWidget(Qt::RightDockWidgetArea, mDetailDockWidget);
}
void MainWindow::OnNewProject()
{
    if (!MaybeSaveScene())
        return;
    NewProjectDialog d(this);
    if (d.exec() != QDialog::Accepted)
        return;
    ProjectCreateRequest r{d.GetProjectPath(), d.GetProjectName()};
    if (mContext.projectService->CreateProject(r))
    {
        mContext.settings->AddRecentProject(mContext.projectService->Context().projectFile);
        QMessageBox::information(this, tr("提示"), tr("工程创建成功！"));
    }
}
void MainWindow::OnOpenProject()
{
    if (!MaybeSaveScene())
        return;
    OpenProjectDialog d(*mContext.settings, this);
    if (d.exec() != QDialog::Accepted)
        return;
    if (mContext.projectService->OpenProject(d.GetSelectedProjectPath()))
        mContext.settings->AddRecentProject(d.GetSelectedProjectPath());
}
void MainWindow::OnSaveProject()
{
    if (!mContext.projectService->HasOpenProject())
    {
        QMessageBox::warning(this, tr("警告"), tr("当前没有打开的工程！"));
        return;
    }
    if (SaveScene() && mContext.projectService->SaveProject())
        QMessageBox::information(this, tr("成功"), tr("工程保存成功！"));
}
void MainWindow::OnCloseProject()
{
    if (!mContext.projectService->HasOpenProject())
        return;
    if (!MaybeSaveScene())
        return;
    if (QMessageBox::question(this, tr("确认"), tr("确定关闭当前工程吗？")) == QMessageBox::Yes)
        mContext.projectService->CloseProject();
}
void MainWindow::OnProjectOpened(const QString &)
{
    mContentWidget->SetRootPath(mContext.projectService->AssetRoot());
    const QString scenePath =
        QDir(mContext.projectService->Context().scenesRoot).filePath("Main.scene.json");
    QString error;
    if (QFileInfo::exists(scenePath))
    {
        if (!mContext.sceneDocument->Load(scenePath, &error))
            QMessageBox::warning(this, tr("场景加载失败"), error);
    }
    else
    {
        mContext.sceneDocument->NewDocument(scenePath);
    }
    UpdateProjectUi();
}
void MainWindow::OnProjectClosed()
{
    mContext.selectionService->Clear();
    mContext.sceneDocument->Close();
    mContentWidget->SetRootPath(QString());
    UpdateProjectUi();
}
void MainWindow::OnProjectError(const QString &m)
{
    QMessageBox::critical(this, tr("工程操作失败"), m);
}
void MainWindow::UpdateProjectUi()
{
    const bool open = mContext.projectService->HasOpenProject();
    mSaveAction->setEnabled(open);
    mCloseAction->setEnabled(open);
    mSaveSceneAction->setEnabled(open);
    setWindowTitle(open ? mContext.projectService->ProjectName() + " - GNXEngine"
                        : QStringLiteral("GNXEngine"));
}

bool MainWindow::SaveScene()
{
    if (!mContext.sceneDocument->IsOpen() || !mContext.sceneDocument->IsDirty())
        return true;
    QString error;
    if (mContext.sceneDocument->Save(&error))
        return true;
    QMessageBox::warning(this, tr("场景保存失败"), error);
    return false;
}

void MainWindow::OnSaveSceneAs()
{
    if (!mContext.sceneDocument->IsOpen())
        return;
    QFileDialog dialog(this, tr("场景另存为"), mContext.sceneDocument->Path(),
                       tr("场景 (*.scene.json)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return;
    const QString selectedPath = dialog.selectedFiles().constFirst();
    if (!IsWithinDirectory(selectedPath, mContext.projectService->Context().scenesRoot))
    {
        QMessageBox::warning(this, tr("场景保存失败"),
                             tr("场景必须保存在当前工程的 Scenes 目录中"));
        return;
    }
    QString error;
    if (!mContext.sceneDocument->SaveAs(selectedPath, &error))
        QMessageBox::warning(this, tr("场景保存失败"), error);
}

bool MainWindow::MaybeSaveScene()
{
    if (!mContext.sceneDocument->IsDirty())
        return true;
    const auto answer =
        QMessageBox::warning(this, tr("未保存的场景"), tr("当前场景已修改，是否保存？"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    return answer == QMessageBox::Discard || (answer == QMessageBox::Save && SaveScene());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MaybeSaveScene() ? event->accept() : event->ignore();
}
