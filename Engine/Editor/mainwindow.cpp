#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QFileSystemModel>
#include <QListView>
#include "View.h"
#include "RenderWindowWidget.h"  // 添加 RenderWindowWidget 头文件
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/GNXEngine/include/Events/Event.h"
#include "Runtime/GNXEngine/include/Events/ApplicationEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QString::fromLocal8Bit("GNXEngine"));

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

    // ============ 方式 2：如果不想使用中心窗口，可以将 RenderWindowWidget 放入 Dock Widget ============
    // 注释掉上面的方式1，取消注释下面的代码即可

    /*
    QDockWidget* renderDockWidget = new QDockWidget(this);
    renderDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    renderDockWidget->setWindowTitle("Scene View");

    RenderWindowWidget* renderWidget = new RenderWindowWidget(renderDockWidget);
    renderDockWidget->setWidget(renderWidget);

    addDockWidget(Qt::RightDockWidgetArea, renderDockWidget);
    */

    setupDock();

    setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDock()
{
    // 层级结构 Dock Widget
    m_SceneDockWidget = new QDockWidget(this);
    m_SceneDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_SceneDockWidget->setWindowTitle("层级结构");

    // 内容浏览器窗口
    m_ContentDockWidget = new QDockWidget(this);
    m_ContentDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_ContentDockWidget->setWindowTitle("Content Browser");
    m_ContentDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
    fs::path contentPath = getAssetsDir();
    if (EnsurePathExists(contentPath))
    {
    }
    ContentWidget* contentWidget = new ContentWidget(m_ContentDockWidget, QString::fromStdString(getAssetsDir()));

    // 详细信息 Dock Widget
    m_DetailDockWidget = new QDockWidget(this);
    m_DetailDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_DetailDockWidget->setWindowTitle("Detail");

    // 添加 Dock Widgets 到主窗口
    this->addDockWidget(Qt::LeftDockWidgetArea, m_SceneDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::BottomDockWidgetArea, m_ContentDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::RightDockWidgetArea, m_DetailDockWidget, Qt::Orientation::Vertical);
}
