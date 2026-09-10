#include "EditorApplication.h"
#include <QApplication>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char *argv[])
{
#if GNX_OS_WINDOWS
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
    QApplication application(argc, argv);
    application.setOrganizationName("GNXEngine");
    application.setApplicationName("GNXEditor");
    EditorApplication editor;
    if (!editor.Initialize(application))
    {
        QMessageBox::critical(nullptr, "GNXEditor",
                              "编辑器初始化失败，请检查 Runtime 资源目录和日志。");
        return 1;
    }
    if (application.arguments().contains("--smoke-test"))
        QTimer::singleShot(3000, &application, &QCoreApplication::quit);
    const int result = editor.Run();
    editor.Shutdown();
    return result;
}
