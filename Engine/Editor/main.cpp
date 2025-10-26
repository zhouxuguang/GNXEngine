#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
#if OS_WINDOWS
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
