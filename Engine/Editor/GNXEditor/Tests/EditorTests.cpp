#include "Application/EditorApplication.h"
#include "Application/EditorSettings.h"
#include "Assets/AssetImportService.h"
#include "Assets/AssetRegistry.h"
#include "Assets/ContentWidget.h"
#include "Assets/ThumbnailService.h"
#include "Documents/CommandHistory.h"
#include "Documents/SceneDocument.h"
#include "Project/EditorProjectService.h"
#include "Project/OpenProjectDialog.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/AssetProcess/include/AssetImporter.h"
#include "Runtime/GNXEngine/include/RenderWindow.h"
#include "Viewport/EditorRenderHost.h"
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QTemporaryDir>
#include <QTimer>
#include <type_traits>

namespace
{
class ScopedEnvironment final
{
  public:
    ScopedEnvironment(const char *name, const QByteArray &value)
        : mName(name), mPrevious(qgetenv(name)), mExisted(qEnvironmentVariableIsSet(name))
    {
        qputenv(name, value);
    }
    ~ScopedEnvironment()
    {
        if (mExisted)
            qputenv(mName.constData(), mPrevious);
        else
            qunsetenv(mName.constData());
    }

  private:
    QByteArray mName;
    QByteArray mPrevious;
    bool mExisted;
};

bool InvokeDialog(QObject &owner, const char *slot)
{
    bool safeDialog = false;
    QTimer observer;
    observer.setInterval(10);
    QObject::connect(&observer, &QTimer::timeout,
                     [&safeDialog]
                     {
                         for (QWidget *widget : QApplication::topLevelWidgets())
                         {
                             auto *dialog = qobject_cast<QFileDialog *>(widget);
                             if (dialog && dialog->isVisible())
                             {
                                 safeDialog = dialog->testOption(QFileDialog::DontUseNativeDialog);
                                 dialog->reject();
                                 return;
                             }
                         }
                     });
    observer.start();
    QTimer::singleShot(1000,
                       []
                       {
                           if (QWidget *widget = QApplication::activeModalWidget())
                               widget->close();
                       });
    const bool invoked = QMetaObject::invokeMethod(&owner, slot, Qt::DirectConnection);
    observer.stop();
    return invoked && safeDialog;
}

EditorProjectContext MakeContext(const QString &root)
{
    EditorProjectContext context;
    context.engineContentRoot = root;
    context.projectRoot = root;
    context.projectFile = QDir(root).filePath("Test.gnxproj");
    context.assetRoot = QDir(root).filePath("Assets");
    context.cacheRoot = QDir(root).filePath(".gnx/Cache");
    context.settingsRoot = QDir(root).filePath("Settings");
    context.scenesRoot = QDir(root).filePath("Scenes");
    context.sessionId = "test-session";
    QDir().mkpath(context.assetRoot);
    QDir().mkpath(context.cacheRoot);
    QDir().mkpath(context.settingsRoot);
    QDir().mkpath(context.scenesRoot);
    return context;
}

int TestProject()
{
    QTemporaryDir temporary;
#if defined(Q_OS_WIN)
    ScopedEnvironment recentProjects("LOCALAPPDATA", temporary.path().toUtf8());
#else
    ScopedEnvironment recentProjects("HOME", temporary.path().toUtf8());
#endif
    EditorProjectService service;
    if (!temporary.isValid() || !service.CreateProject({temporary.path(), "ProjectA"}))
        return 1;
    const QString original = service.ProjectRoot();
    QFile broken(temporary.filePath("Broken.gnxproj"));
    if (!broken.open(QIODevice::WriteOnly) || broken.write("{") != 1)
        return 2;
    broken.close();
    if (service.OpenProject(broken.fileName()) || service.ProjectRoot() != original)
        return 3;
    if (service.CreateProject({temporary.path(), "CON"}) || service.ProjectRoot() != original)
        return 4;
    if (!QFileInfo(service.Context().projectFile).isFile() ||
        !QDir(service.Context().cacheRoot).exists())
        return 5;
    service.CloseProject();
    return service.HasOpenProject() ? 6 : 0;
}

int TestAssetImport()
{
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 1;
    const EditorProjectContext context = MakeContext(temporary.path());
    const QString nested = QDir(context.assetRoot).filePath("Characters/Hero");
    QDir().mkpath(nested);
    const QString source = temporary.filePath("Color.PNG");
    QImage(8, 8, QImage::Format_RGBA8888).save(source, "PNG");
    AssetProcess::AssetImporter importer;
    const auto result = importer.ImportFromFileDetailed(source.toStdString(), nested.toStdString(),
                                                        context.projectRoot.toStdString());
    if (!result.success || result.generatedAssets.empty())
        return 2;
    if (!QDir(context.cacheRoot).entryList({"*.texture"}, QDir::Files).size())
        return 3;
    const QString unsupportedSource = temporary.filePath("Unsupported.XYZ");
    QFile unsupportedFile(unsupportedSource);
    if (!unsupportedFile.open(QIODevice::WriteOnly) || unsupportedFile.write("unsupported") < 0)
        return 4;
    unsupportedFile.close();
    const auto unsupported = importer.ImportFromFileDetailed(
        unsupportedSource.toStdString(), nested.toStdString(), context.projectRoot.toStdString());
    if (unsupported.success || unsupported.errorCode != "unsupported_format")
        return 5;
    AssetRegistry registry;
    if (!registry.OpenProject(context) ||
        !registry.ContainsSource(QDir(nested).filePath("Color.PNG")))
        return 6;
    const AssetRecord record = registry.RecordForSource(QDir(nested).filePath("Color.PNG"));
    if (record.guid.isEmpty() || record.artifacts.isEmpty())
        return 7;

    const QString secondRoot = temporary.filePath("SecondProject");
    QDir().mkpath(secondRoot);
    EditorProjectContext secondContext = MakeContext(secondRoot);
    secondContext.sessionId = "second-session";
    AssetImportService service;
    service.OpenProject(context);
    bool oldResultPublished = false;
    bool switched = false;
    QObject::connect(&service, &AssetImportService::ImportStarted, &service,
                     [&](const QString &)
                     {
                         switched = true;
                         service.CloseProject(context.sessionId);
                         service.OpenProject(secondContext);
                     });
    QObject::connect(&service, &AssetImportService::ImportFinished, &service,
                     [&](const QString &, const QString &, const QStringList &)
                     {
                         oldResultPublished = true;
                     });
    if (!service.EnqueueFile(source, nested))
        return 8;
    QEventLoop wait;
    QTimer::singleShot(500, &wait, &QEventLoop::quit);
    wait.exec();
    service.Shutdown();
    return switched && !oldResultPublished ? 0 : 9;
}

int TestDialogs()
{
    QTemporaryDir temporary;
    EditorSettings settings(temporary.filePath("editor.ini"));
    OpenProjectDialog open(settings);
    if (!InvokeDialog(open, "OnBrowseButtonClicked"))
        return 1;
    AssetImportService service;
    AssetRegistry registry;
    ThumbnailService thumbnails(registry);
    QDockWidget dock;
    ContentWidget content(&dock, temporary.path(), service, registry, thumbnails, settings);
    const bool result = InvokeDialog(content, "OpenImportAssetDialog");
    service.Shutdown();
    thumbnails.Shutdown();
    return result ? 0 : 2;
}

int TestContentBrowser()
{
    QTemporaryDir temporary;
    const EditorProjectContext context = MakeContext(temporary.path());
    const QString file = QDir(context.assetRoot).filePath("asset.png");
    QImage(2, 2, QImage::Format_RGBA8888).save(file);
    AssetRegistry registry;
    if (!registry.OpenProject(context))
        return 1;
    const AssetRecord first = registry.RecordForSource(file);
    if (first.guid.isEmpty() || !QFileInfo(file + ".gnxmeta").isFile())
        return 2;
    if (!registry.Refresh() || registry.RecordForSource(file).guid != first.guid)
        return 3;
    const QString moved = QDir(context.assetRoot).filePath("Nested/moved.png");
    if (!registry.SetImportSettings(file, QJsonObject{{"srgb", true}}) ||
        !registry.SetDependencies(file, {QStringLiteral("dependency-guid")}) ||
        !registry.MoveAsset(file, moved))
        return 4;
    const AssetRecord movedRecord = registry.RecordForSource(moved);
    return movedRecord.guid == first.guid && movedRecord.dependencies.contains("dependency-guid") &&
                   movedRecord.importSettings.value("srgb").toBool()
               ? 0
               : 5;
}

int TestViewport()
{
    EditorRenderHost host;
    host.Tick(1.0f / 60.0f);
    host.Resize(1280, 720);
    if (host.Attach(nullptr, 1, 1) || host.State() != EditorRenderState::Detached)
        return 1;
    EditorRenderHost failingHost(
        [](const GNXEngine::WindowProps &, void *)
        {
            return GNXEngine::RenderWindowPtr{};
        });
    if (failingHost.Attach(reinterpret_cast<void *>(1), 64, 64) ||
        failingHost.State() != EditorRenderState::Failed || failingHost.LastError().empty())
        return 2;
    host.MarkSurfaceLost();
    if (host.State() != EditorRenderState::SurfaceLost)
        return 3;
    host.MarkDeviceLost("test device loss");
    if (host.State() != EditorRenderState::DeviceLost || host.LastError().empty())
        return 4;
    host.Detach();
    host.Detach();
    return host.State() == EditorRenderState::Detached ? 0 : 5;
}

int TestLifecycle()
{
    QTemporaryDir temporary;
    const QString settingsPath = temporary.filePath("editor.ini");
    QFile damagedSettings(settingsPath);
    if (!damagedSettings.open(QIODevice::WriteOnly) ||
        damagedSettings.write("[broken\nvalue=\0\1") < 0)
        return 1;
    damagedSettings.close();
    EditorSettings settings(settingsPath);
    settings.Load();
    settings.SetLastImportDirectory(temporary.path());
    settings.SetWindowGeometry("geometry");
    settings.Save();
    EditorSettings restored(settingsPath);
    restored.Load();
    if (restored.LastImportDirectory() != temporary.path() ||
        restored.WindowGeometry() != "geometry")
        return 2;
    EditorApplication application;
    if (application.InitializeWithContentRoot(
            *qobject_cast<QApplication *>(QCoreApplication::instance()),
            QStringLiteral("Z:/GNXEngine/nonexistent-content-root")))
        return 3;
    application.Shutdown();
    application.Shutdown();
    AssetImportService service;
    service.Shutdown();
    service.Shutdown();
    AssetRegistry registry;
    ThumbnailService thumbnails(registry);
    thumbnails.Shutdown();
    thumbnails.Shutdown();
    return 0;
}

int TestRuntimeCompatibility()
{
    static_assert(std::is_same_v<decltype(GNXEngine::RenderWindow::CreateWithExternalWindow(
                                     GNXEngine::WindowProps{}, nullptr)),
                                 GNXEngine::RenderWindowPtr>);
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 1;
    const QString engineRoot = temporary.filePath("EngineContent");
    const QString cacheRoot = temporary.filePath("Project/.gnx/Cache");
    QDir().mkpath(engineRoot);
    QDir().mkpath(cacheRoot);
    const QString artifact = QDir(cacheRoot).filePath("content.texture");
    QFile file(artifact);
    if (!file.open(QIODevice::WriteOnly) || file.write("asset") < 0)
        return 2;
    file.close();
    if (!AssetManager::AssetManager::Initialize(engineRoot.toStdString()) ||
        !AssetManager::AssetManager::SetProjectRoot(cacheRoot.toStdString()) ||
        !AssetManager::AssetManager::RegisterResourcePath("asset-guid", artifact.toStdString()))
        return 3;
    const QString resolved = QString::fromStdString(
        AssetManager::AssetManager::ResolveResourcePath("asset-guid.texture"));
    const QString corrupted = QDir(cacheRoot).filePath("corrupted.texture");
    QFile corruptedFile(corrupted);
    if (!corruptedFile.open(QIODevice::WriteOnly) || corruptedFile.write("not-a-valid-texture") < 0)
        return 4;
    corruptedFile.close();
    if (AssetManager::AssetManager::GetInstance()->LoadTexture(corrupted.toStdString()))
        return 5;
    AssetManager::AssetManager::ClearProjectRoot();
    AssetManager::AssetManager::Shutdown();
    return QFileInfo(resolved) == QFileInfo(artifact) ? 0 : 6;
}

int TestDocument()
{
    QTemporaryDir temporary;
    CommandHistory history;
    SceneDocument document(history);
    const QString path = temporary.filePath("Main.scene.json");
    document.NewDocument(path);
    const int initial = document.Entities().size();
    document.AddEntity("Cube");
    if (document.Entities().size() != initial + 1 || !document.IsDirty())
        return 1;
    history.Undo();
    if (document.Entities().size() != initial)
        return 2;
    history.Redo();
    QString error;
    if (!document.Save(&error) || document.IsDirty())
        return 3;
    document.AddEntity("Temporary");
    history.Undo();
    if (document.IsDirty())
        return 4;
    const QString originalPath = document.Path();
    const int originalCount = document.Entities().size();
    const QString brokenPath = temporary.filePath("Broken.scene.json");
    QFile broken(brokenPath);
    if (!broken.open(QIODevice::WriteOnly) || broken.write("{") != 1)
        return 5;
    broken.close();
    if (document.Load(brokenPath, &error) || document.Path() != originalPath ||
        document.Entities().size() != originalCount)
        return 6;
    CommandHistory loadedHistory;
    SceneDocument loaded(loadedHistory);
    return loaded.Load(path, &error) && loaded.Entities().size() == initial + 1 ? 0 : 7;
}
} // namespace

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    const QString suite = argc > 1 ? argv[1] : QString();
    if (suite == "Project")
        return TestProject();
    if (suite == "AssetImport")
        return TestAssetImport();
    if (suite == "ContentBrowser")
        return TestContentBrowser();
    if (suite == "Dialogs")
        return TestDialogs();
    if (suite == "Viewport")
        return TestViewport();
    if (suite == "Lifecycle")
        return TestLifecycle();
    if (suite == "RuntimeCompatibility")
        return TestRuntimeCompatibility();
    if (suite == "Document")
        return TestDocument();
    return 64;
}
