#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
