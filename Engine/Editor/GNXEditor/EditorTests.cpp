#include "EditorProjectService.h"
#include "EditorRenderHost.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include <QCoreApplication>
#include <type_traits>

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    static_assert(std::is_same_v<decltype(GNXEngine::RenderWindow::CreateWithExternalWindow(
        GNXEngine::WindowProps{}, nullptr)), GNXEngine::RenderWindowPtr>);

    EditorRenderHost renderHost;
    renderHost.Tick(1.0f / 60.0f);
    renderHost.Resize(1280, 720);
    renderHost.Detach();

    EditorProjectService projectService;
    if (projectService.HasOpenProject())
        return 1;
    return 0;
}
