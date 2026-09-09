#include "AssetImportService.h"
#include "EditorProjectService.h"
#include "EditorRenderHost.h"
#include "EditorSettings.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <type_traits>

int main(int argc, char **argv) {
  QCoreApplication application(argc, argv);
  static_assert(
      std::is_same_v<decltype(GNXEngine::RenderWindow::CreateWithExternalWindow(
                         GNXEngine::WindowProps{}, nullptr)),
                     GNXEngine::RenderWindowPtr>);

  EditorRenderHost renderHost;
  renderHost.Tick(1.0f / 60.0f);
  renderHost.Resize(1280, 720);
  renderHost.Detach();
  renderHost.Detach();

  EditorProjectService projectService;
  if (projectService.HasOpenProject())
    return 1;

  QTemporaryDir temporaryDirectory;
  if (!temporaryDirectory.isValid())
    return 2;
  const QString settingsPath = temporaryDirectory.filePath("editor.ini");
  {
    EditorSettings settings(settingsPath);
    settings.AddRecentProject("C:/Projects/Test/Test.gnxproj");
    settings.SetLastImportDirectory("C:/Imports");
    settings.Save();
  }
  {
    EditorSettings settings(settingsPath);
    settings.Load();
    if (settings.RecentProjects().size() != 1 ||
        settings.LastImportDirectory() != "C:/Imports")
      return 3;
  }

  AssetImportService importService;
  if (importService.Enqueue({temporaryDirectory.filePath("missing.fbx"),
                             temporaryDirectory.path()}))
    return 4;
  importService.Shutdown();
  importService.Shutdown();
  return 0;
}
