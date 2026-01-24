#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QFileSystemModel>
#include <QListView>
#include "View.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    setWindowTitle(QString::fromLocal8Bit("GNXEngine"));

	QWindow* window = QWindow::fromWinId((WId)test());
	window->resize(2560, 1080);
	QWidget* wrapper = QWidget::createWindowContainer(window, this);
	setCentralWidget(wrapper);
	wrapper->show();
    
    setupDock();

    setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDock()
{
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
    
    m_DetailDockWidget = new QDockWidget(this);
    m_DetailDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_DetailDockWidget->setWindowTitle("Detail");

    this->addDockWidget(Qt::LeftDockWidgetArea, m_SceneDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::BottomDockWidgetArea, m_ContentDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::RightDockWidgetArea, m_DetailDockWidget, Qt::Orientation::Vertical);
    
}

