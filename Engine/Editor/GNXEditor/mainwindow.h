#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTreeView>

#include "ContentWidget.h"

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

private slots:
    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnCloseProject();

private:
    Ui::MainWindow *ui;

    QDockWidget *m_SceneDockWidget = nullptr;
    QDockWidget *m_ContentDockWidget = nullptr;
    QDockWidget *m_DetailDockWidget = nullptr;
    ContentWidget *m_ContentWidget = nullptr;

    void SetupMenu();
    void UpdateWindowTitle();
};
#endif // MAINWINDOW_H
