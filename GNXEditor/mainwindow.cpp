#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QFileSystemModel>
#include "View.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    setWindowTitle(QString::fromLocal8Bit("GNXEngine"));

    QWindow *window = QWindow::fromWinId((WId)test());
    window->resize(2560, 1080);
    QWidget *wrapper = QWidget::createWindowContainer(window, this);
    setCentralWidget(wrapper);
    wrapper->show();
//    window->setTitle("QWindow with MTKView");
//    window->resize(640, 480);
//    window->show();
    
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
    
    m_ContentDockWidget = new QDockWidget(this);
    m_ContentDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_ContentDockWidget->setWindowTitle("内容");
    
    QTreeView *fileTreeView = new QTreeView(this);
    
    m_ContentDockWidget->setWidget(fileTreeView);
    
    // 创建文件系统模型并设置根路径
    QFileSystemModel model;
    model.setRootPath(QDir::currentPath());
    // 设置文件过滤器，只显示目录和所有者可读的文件
    model.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    // 设置模型到视图中
    fileTreeView->setModel(&model);
    // 设置视图样式，并调整列宽以适应内容
    fileTreeView->setAnimated(false);
    fileTreeView->setIndentation(20);
    fileTreeView->setSortingEnabled(true);
    fileTreeView->resizeColumnToContents(0);
    fileTreeView->setEnabled(true);
    fileTreeView->show();
    
    
    
    m_DetailDockWidget = new QDockWidget(this);
    m_DetailDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    m_DetailDockWidget->setWindowTitle("细节");

    this->addDockWidget(Qt::LeftDockWidgetArea, m_SceneDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::BottomDockWidgetArea, m_ContentDockWidget, Qt::Orientation::Vertical);
    this->addDockWidget(Qt::RightDockWidgetArea, m_DetailDockWidget, Qt::Orientation::Vertical);
    
}

