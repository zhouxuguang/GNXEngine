#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QFileSystemModel>
#include <QListView>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QIcon>
#include "View.h"
#include "RenderWindowWidget.h"
#include "NewProjectDialog.h"
#include "OpenProjectDialog.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/GNXEngine/include/Events/Event.h"
#include "Runtime/GNXEngine/include/Events/ApplicationEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QString::fromLocal8Bit("GNXEngine"));

    // 设置菜单
    SetupMenu();

    // ============ 方式 1：使用 RenderWindowWidget 作为中心窗口 ============
    // 创建 RenderWindowWidget
    RenderWindowWidget* renderWindowWidget = new RenderWindowWidget(this);

    // 设置为中心窗口
    setCentralWidget(renderWindowWidget);

    // 获取内部的 RenderWindow 并设置事件回调
    GNXEngine::RenderWindowPtr renderWindow = renderWindowWidget->GetRenderWindow();
    if (renderWindow)
    {
        renderWindow->SetEventCallback([](GNXEngine::Event& event)
        {
            GNXEngine::EventDispatcher dispatcher(event);

            // 处理窗口大小调整事件
            dispatcher.Dispatch<GNXEngine::WindowResizeEvent>(
                [](GNXEngine::WindowResizeEvent& e) {
                    // std::cout << "Window resized to " << e.GetWidth() << "x" << e.GetHeight() << std::endl;
                    return false;
                }
            );

            // 可以添加更多事件处理...
        });
    }

    setupDock();

    setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SetupMenu()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu("文件");

    QAction* newProjectAction = new QAction("新建工程", this);
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::OnNewProject);
    fileMenu->addAction(newProjectAction);

    QAction* openProjectAction = new QAction("打开工程", this);
    openProjectAction->setShortcut(QKeySequence::Open);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::OnOpenProject);
    fileMenu->addAction(openProjectAction);

    fileMenu->addSeparator();

    QAction* saveProjectAction = new QAction("保存工程", this);
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::OnSaveProject);
    fileMenu->addAction(saveProjectAction);

    fileMenu->addSeparator();

    QAction* closeProjectAction = new QAction("关闭工程", this);
    connect(closeProjectAction, &QAction::triggered, this, &MainWindow::OnCloseProject);
    fileMenu->addAction(closeProjectAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("退出", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
}

void MainWindow::OnNewProject()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // 更新窗口标题
        UpdateWindowTitle();

        // 更新 ContentWidget 的路径
        GNXEngine::ProjectConfig* project = GNXEngine::ProjectManager::GetInstance().GetProject();
        if (project && m_ContentWidget)
        {
            QString assetsPath = QString::fromStdString(project->GetAbsoluteAssetPath(""));
            m_ContentWidget->SetRootPath(assetsPath);
        }

        QMessageBox::information(this, "提示", "工程创建成功！\n现在可以开始创建场景和导入资源。");
    }
}

void MainWindow::OnOpenProject()
{
    OpenProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // 更新窗口标题
        UpdateWindowTitle();

        // 更新 ContentWidget 的路径
        GNXEngine::ProjectConfig* project = GNXEngine::ProjectManager::GetInstance().GetProject();
        if (project && m_ContentWidget)
        {
            QString assetsPath = QString::fromStdString(project->GetAbsoluteAssetPath(""));
            m_ContentWidget->SetRootPath(assetsPath);
        }
    }
}

void MainWindow::OnSaveProject()
{
    if (!GNXEngine::ProjectManager::GetInstance().IsProjectOpen())
    {
        QMessageBox::warning(this, "警告", "当前没有打开的工程！");
        return;
    }

    bool success = GNXEngine::ProjectManager::GetInstance().SaveProject();
    if (success)
    {
        QMessageBox::information(this, "成功", "工程保存成功！");
    }
    else
    {
        QMessageBox::critical(this, "错误", "工程保存失败！");
    }
}

void MainWindow::OnCloseProject()
{
    if (!GNXEngine::ProjectManager::GetInstance().IsProjectOpen())
    {
        QMessageBox::warning(this, "警告", "当前没有打开的工程！");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认",
        "确定要关闭当前工程吗？未保存的更改将会丢失。",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes)
    {
        GNXEngine::ProjectManager::GetInstance().CloseProject();
        UpdateWindowTitle();

        // 更新 ContentWidget 的路径为默认目录
        if (m_ContentWidget)
        {
            m_ContentWidget->SetRootPath(QString::fromStdString(getAssetsDir()));
        }
    }
}

void MainWindow::UpdateWindowTitle()
{
    GNXEngine::ProjectConfig* project = GNXEngine::ProjectManager::GetInstance().GetProject();
    if (project)
    {
        setWindowTitle(QString::fromStdString(project->projectName) + " - GNXEngine");
    }
    else
    {
        setWindowTitle("GNXEngine");
    }
}

void MainWindow::setupDock()
{
    // 层级结构 Dock Widget
    m_SceneDockWidget = new QDockWidget(this);
    m_SceneDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_SceneDockWidget->setWindowTitle("Hierarchy");

    // 内容浏览器窗口
    m_ContentDockWidget = new QDockWidget(this);
    m_ContentDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_ContentDockWidget->setWindowTitle("Content Browser");
    m_ContentDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea);

    GNXEngine::ProjectConfig* project = GNXEngine::ProjectManager::GetInstance().GetProject();
    if (project)
    {
        QString assetsPath = QString::fromStdString(project->GetAbsoluteAssetPath(""));
        m_ContentWidget = new ContentWidget(m_ContentDockWidget, assetsPath);
    }
    else
    {
        fs::path contentPath = getAssetsDir();
        if (EnsurePathExists(contentPath))
        {
        }
        m_ContentWidget = new ContentWidget(m_ContentDockWidget, QString::fromStdString(getAssetsDir()));
    }

    // 详细信息 Dock Widget
    m_DetailDockWidget = new QDockWidget(this);
    m_DetailDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_DetailDockWidget->setWindowTitle("Detail");

    // 添加 Dock Widgets 到主窗口
    this->addDockWidget(Qt::LeftDockWidgetArea, m_SceneDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::BottomDockWidgetArea, m_ContentDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::RightDockWidgetArea, m_DetailDockWidget, Qt::Orientation::Vertical);
}
