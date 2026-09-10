#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Viewport/EditorRenderHost.h"
#include "Viewport/EditorViewportWidget.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>
#include <algorithm>

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    if (!AssetManager::AssetManager::Initialize(GetProjectAssetDir()))
        return 1;

    EditorRenderHost renderHost;
    QMainWindow window;
    window.setWindowTitle("GNXEditor Render Sandbox");
    window.resize(1280, 720);
    window.setCentralWidget(new EditorViewportWidget(renderHost, &window));

    QElapsedTimer elapsed;
    elapsed.start();
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout,
                     [&]
                     {
                         renderHost.Tick(std::min(elapsed.restart() / 1000.0f, 0.1f));
                     });
    timer.start(16);
    window.show();
    if (application.arguments().contains("--smoke-test"))
        QTimer::singleShot(3000, &application, &QCoreApplication::quit);
    const int result = application.exec();
    timer.stop();
    const bool hadDevice = static_cast<bool>(RenderCore::GetRenderDevice());
    renderHost.Detach();
    if (hadDevice)
        RenderSystem::SceneManager::DestroyInstance();
    AssetManager::AssetManager::Shutdown();
    if (hadDevice)
        RenderCore::DestroyRenderDevice();
    return result;
}
