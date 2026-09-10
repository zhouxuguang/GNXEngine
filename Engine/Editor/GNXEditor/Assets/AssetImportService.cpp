#include "AssetImportService.h"
#include "Runtime/AssetProcess/include/AssetImporter.h"
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <set>

struct AssetImportService::WorkerState
{
    std::mutex mutex;
    std::condition_variable wake;
    std::condition_variable exited;
    std::deque<AssetImportRequest> queue;
    std::set<QString> pending;
    EditorProjectContext project;
    bool stopping = false;
    bool publish = true;
    bool running = false;
    bool workerExited = false;
};

namespace
{
bool IsWithin(const QString &child, const QString &root)
{
    const QString normalizedChild =
        QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(child).absoluteFilePath()));
    const QString normalizedRoot =
        QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(root).absoluteFilePath()));
#if defined(Q_OS_WIN)
    constexpr Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
    return normalizedChild.compare(normalizedRoot, pathCase) == 0 ||
           normalizedChild.startsWith(normalizedRoot + '/', pathCase);
}
} // namespace

AssetImportService::AssetImportService(QObject *parent)
    : QObject(parent), mState(std::make_shared<WorkerState>()),
      mWorker(&AssetImportService::WorkerMain, mState, this)
{
    qRegisterMetaType<EditorAssetImportResult>();
}

AssetImportService::~AssetImportService()
{
    Shutdown();
}

QString AssetImportService::Key(const AssetImportRequest &request)
{
    return request.projectSessionId + "|" + QDir::cleanPath(request.sourceFile) + "|" +
           QDir::cleanPath(request.destinationDirectory);
}

void AssetImportService::OpenProject(const EditorProjectContext &context)
{
    bool running = false;
    {
        std::lock_guard lock(mState->mutex);
        mState->queue.clear();
        mState->pending.clear();
        mState->project = context;
        running = mState->running;
    }
    emit ImportQueueChanged(0, running);
}

void AssetImportService::CloseProject(const QString &sessionId)
{
    bool running = false;
    {
        std::lock_guard lock(mState->mutex);
        if (mState->project.sessionId != sessionId)
            return;
        mState->project = {};
        mState->queue.clear();
        mState->pending.clear();
        running = mState->running;
    }
    emit ImportQueueChanged(0, running);
}

bool AssetImportService::EnqueueFile(const QString &sourceFile, const QString &destinationDirectory)
{
    AssetImportRequest request;
    {
        std::lock_guard lock(mState->mutex);
        request = {sourceFile,
                   destinationDirectory,
                   mState->project.projectRoot,
                   mState->project.assetRoot,
                   mState->project.cacheRoot,
                   mState->project.sessionId};
    }
    return Enqueue(request);
}

bool AssetImportService::Enqueue(const AssetImportRequest &request)
{
    if (!QFileInfo::exists(request.sourceFile) || request.destinationDirectory.isEmpty() ||
        request.projectSessionId.isEmpty() ||
        !IsWithin(request.destinationDirectory, request.assetRoot))
    {
        emit ImportFailed(request.sourceFile, tr("源文件不存在或目标目录无效"));
        return false;
    }

    int pendingCount = 0;
    bool running = false;
    {
        std::lock_guard lock(mState->mutex);
        if (mState->stopping || mState->project.sessionId != request.projectSessionId ||
            mState->pending.contains(Key(request)))
            return false;
        mState->pending.insert(Key(request));
        mState->queue.push_back(request);
        pendingCount = static_cast<int>(mState->queue.size());
        running = mState->running;
    }
    mState->wake.notify_one();
    emit ImportQueueChanged(pendingCount, running);
    return true;
}

int AssetImportService::PendingTaskCount() const
{
    std::lock_guard lock(mState->mutex);
    return static_cast<int>(mState->queue.size()) + (mState->running ? 1 : 0);
}

void AssetImportService::Shutdown()
{
    if (!mState)
        return;
    {
        std::lock_guard lock(mState->mutex);
        if (mState->stopping && !mWorker.joinable())
            return;
        mState->stopping = true;
        mState->publish = false;
        mState->queue.clear();
        mState->pending.clear();
    }
    mState->wake.notify_all();

    if (mWorker.joinable())
    {
        std::unique_lock lock(mState->mutex);
        const bool exited = mState->exited.wait_for(lock, std::chrono::seconds(3),
                                                    [this]
                                                    {
                                                        return mState->workerExited;
                                                    });
        lock.unlock();
        if (exited)
            mWorker.join();
        else
            mWorker.detach();
    }
}

void AssetImportService::WorkerMain(const std::shared_ptr<WorkerState> &state,
                                    AssetImportService *service)
{
    for (;;)
    {
        AssetImportRequest request;
        {
            std::unique_lock lock(state->mutex);
            state->wake.wait(lock,
                             [&]
                             {
                                 return state->stopping || !state->queue.empty();
                             });
            if (state->stopping)
                break;
            request = state->queue.front();
            state->queue.pop_front();
            state->running = true;
            if (state->publish)
            {
                QMetaObject::invokeMethod(service,
                                          [service, state, session = request.projectSessionId,
                                           source = request.sourceFile]
                                          {
                                              {
                                                  std::lock_guard lock(state->mutex);
                                                  if (!state->publish ||
                                                      state->project.sessionId != session)
                                                      return;
                                              }
                                              emit service->ImportStarted(source);
                                          });
            }
        }

        AssetProcess::AssetImporter importer;
        const auto outcome = importer.ImportFromFileDetailed(
            request.sourceFile.toStdString(), request.destinationDirectory.toStdString(),
            request.projectRoot.toStdString());

        std::lock_guard lock(state->mutex);
        state->pending.erase(Key(request));
        state->running = false;
        if (!state->publish || state->project.sessionId != request.projectSessionId)
            continue;

        const int pendingCount = static_cast<int>(state->queue.size());
        if (outcome.success)
        {
            QStringList generatedAssets;
            for (const std::string &asset : outcome.generatedAssets)
                generatedAssets.push_back(QString::fromStdString(asset));
            QMetaObject::invokeMethod(
                service,
                [service, source = request.sourceFile, destination = request.destinationDirectory,
                 generatedAssets, pendingCount, state, session = request.projectSessionId]
                {
                    {
                        std::lock_guard lock(state->mutex);
                        if (!state->publish || state->project.sessionId != session)
                            return;
                    }
                    emit service->ImportFinished(source, destination, generatedAssets);
                    emit service->ImportCompleted(
                        {true, source, destination, generatedAssets, {}, {}});
                    emit service->ImportQueueChanged(pendingCount, false);
                });
        }
        else
        {
            const QString errorCode = QString::fromStdString(outcome.errorCode);
            const QString error = QString::fromStdString(outcome.errorMessage);
            QMetaObject::invokeMethod(
                service,
                [service, state, session = request.projectSessionId, source = request.sourceFile,
                 destination = request.destinationDirectory, errorCode, error, pendingCount]
                {
                    {
                        std::lock_guard lock(state->mutex);
                        if (!state->publish || state->project.sessionId != session)
                            return;
                    }
                    emit service->ImportFailed(source, error);
                    emit service->ImportCompleted(
                        {false, source, destination, {}, errorCode, error});
                    emit service->ImportQueueChanged(pendingCount, false);
                });
        }
    }

    {
        std::lock_guard lock(state->mutex);
        state->workerExited = true;
        state->running = false;
    }
    state->exited.notify_all();
}
