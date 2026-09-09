#include "AssetImportService.h"
#include "EditorProjectService.h"
#include "EditorRenderHost.h"
#include "EditorSettings.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include <QCoreApplication>
#include <QDir>
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

  ProjectCreateRequest projectRequest{temporaryDirectory.path(),
                                      QStringLiteral("AssetRootProject")};
  if (!projectService.CreateProject(projectRequest))
    return 3;
  const QString expectedAssetRoot =
      QDir(temporaryDirectory.filePath("AssetRootProject/Assets"))
          .absolutePath();
  if (QDir(projectService.AssetRoot()).absolutePath() != expectedAssetRoot)
    return 4;
  projectService.CloseProject();
  if (!projectService.AssetRoot().isEmpty())
    return 5;

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
      return 6;
  }

  AssetImportService importService;
  if (importService.Enqueue({temporaryDirectory.filePath("missing.fbx"),
                             temporaryDirectory.path()}))
    return 7;
  importService.Shutdown();
  importService.Shutdown();
  return 0;
}
