#include "EditorApplication.h"
#include "Assets/AssetImportService.h"
#include "Assets/AssetRegistry.h"
#include "Assets/ThumbnailService.h"
#include "Documents/CommandHistory.h"
#include "Documents/SceneDocument.h"
#include "Documents/SelectionService.h"
#include "EditorSettings.h"
#include "MainWindow/mainwindow.h"
#include "Project/EditorProjectService.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Viewport/EditorRenderHost.h"
#include <QApplication>
#include <QDir>
#include <algorithm>
EditorApplication::EditorApplication(QObject *parent) : QObject(parent)
{
    connect(&mTimer, &QTimer::timeout, this, &EditorApplication::Tick);
}
EditorApplication::~EditorApplication()
{
    Shutdown();
}
bool EditorApplication::Initialize(QApplication &app)
{
    return InitializeWithContentRoot(app, QString::fromStdString(GetProjectAssetDir()));
}
bool EditorApplication::InitializeWithContentRoot(QApplication &app, const QString &contentRoot)
{
    if (mInitialized)
        return true;
    mShuttingDown = false;
    mFirstTick = true;
    mApplication = &app;
    mContext = std::make_unique<EditorContext>();
    mContext->settings = std::make_unique<EditorSettings>();
    mContext->settings->Load();
    if (!AssetManager::AssetManager::Initialize(contentRoot.toStdString()))
    {
        mContext.reset();
        mApplication = nullptr;
        return false;
    }
    try
    {
        mContext->projectService = std::make_unique<EditorProjectService>(contentRoot);
        mContext->assetImportService = std::make_unique<AssetImportService>();
        mContext->assetRegistry = std::make_unique<AssetRegistry>();
        mContext->renderHost = std::make_unique<EditorRenderHost>();
        mContext->thumbnailService = std::make_unique<ThumbnailService>(*mContext->assetRegistry);
        mContext->commandHistory = std::make_unique<CommandHistory>();
        mContext->selectionService = std::make_unique<SelectionService>();
        mContext->sceneDocument = std::make_unique<SceneDocument>(*mContext->commandHistory);
        connect(mContext->projectService.get(), &EditorProjectService::ProjectAboutToClose, this,
                [this](const QString &sessionId)
                {
                    mContext->assetImportService->CloseProject(sessionId);
                    mContext->assetRegistry->CloseProject();
                    mContext->thumbnailService->CloseProject(sessionId);
                    AssetManager::AssetManager::ClearProjectRoot();
                });
        connect(
            mContext->projectService.get(), &EditorProjectService::ProjectOpened, this,
            [this](const QString &)
            {
                const auto &project = mContext->projectService->Context();
                QDir().mkpath(project.cacheRoot);
                if (!AssetManager::AssetManager::SetProjectRoot(project.cacheRoot.toStdString()))
                {
                    return;
                }
                mContext->assetImportService->OpenProject(project);
                mContext->assetRegistry->OpenProject(project);
                mContext->thumbnailService->OpenProject(project);
            });
        connect(mContext->projectService.get(), &EditorProjectService::ProjectClosed, this,
                [this]
                {
                    AssetManager::AssetManager::ClearProjectRoot();
                });
        connect(mContext->assetImportService.get(), &AssetImportService::ImportFinished, this,
                [this](const QString &source, const QString &destination,
                       const QStringList &generatedAssets)
                {
                    mContext->assetRegistry->RegisterImported(source, destination, generatedAssets);
                });
        mMainWindow = std::make_unique<MainWindow>(*mContext);
    }
    catch (...)
    {
        mMainWindow.reset();
        mContext.reset();
        AssetManager::AssetManager::Shutdown();
        mApplication = nullptr;
        return false;
    }
    connect(&app, &QCoreApplication::aboutToQuit, this,
            [this]
            {
                mTimer.stop();
            });
    mMainWindow->show();
    mElapsed.start();
    mTimer.start(16);
    mInitialized = true;
    return true;
}
int EditorApplication::Run()
{
    return mApplication ? mApplication->exec() : -1;
}
void EditorApplication::Tick()
{
    if (!mInitialized || !mContext || !mContext->renderHost)
        return;
    float dt = 1.0f / 60.0f;
    if (mFirstTick)
    {
        mElapsed.restart();
        mFirstTick = false;
    }
    else
    {
        dt = std::min(float(mElapsed.restart()) / 1000.0f, 0.1f);
    }
    if (mMainWindow && mMainWindow->isMinimized())
    {
        mTimer.setInterval(100);
        return;
    }
    mTimer.setInterval(mApplication->applicationState() == Qt::ApplicationActive ? 16 : 100);
    mContext->renderHost->Tick(dt);
}
void EditorApplication::Shutdown()
{
    if (mShuttingDown || !mContext)
        return;
    mShuttingDown = true;
    mTimer.stop();
    if (mContext->assetImportService)
        mContext->assetImportService->Shutdown();
    if (mContext->thumbnailService)
        mContext->thumbnailService->Shutdown();

    const bool hadRenderDevice = static_cast<bool>(RenderCore::GetRenderDevice());
    if (mMainWindow)
    {
        mContext->settings->SetWindowGeometry(mMainWindow->saveGeometry());
        mContext->settings->SetWindowState(mMainWindow->saveState());
        mContext->settings->Save();
    }

    if (mContext->projectService && mContext->projectService->HasOpenProject())
        mContext->projectService->CloseProject();

    mMainWindow.reset();
    if (mContext->renderHost)
        mContext->renderHost->Detach();
    if (hadRenderDevice)
        RenderSystem::SceneManager::DestroyInstance();
    AssetManager::AssetManager::Shutdown();
    if (hadRenderDevice)
        RenderCore::DestroyRenderDevice();
    mContext.reset();
    mApplication = nullptr;
    mInitialized = false;
    mShuttingDown = false;
}
