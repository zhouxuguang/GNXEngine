#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTreeView>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    void setupDock();

private:
    Ui::MainWindow *ui;
    
    QDockWidget *m_SceneDockWidget = nullptr;
    QDockWidget *m_ContentDockWidget = nullptr;
    QDockWidget *m_DetailDockWidget = nullptr;
};
#endif // MAINWINDOW_H
