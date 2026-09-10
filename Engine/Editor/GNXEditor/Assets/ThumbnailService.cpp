#include "ThumbnailService.h"
#include "AssetRegistry.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetProcess/include/TextureMetaFormat.h"
#include <QMetaObject>

ThumbnailService::ThumbnailService(AssetRegistry &registry, QObject *parent)
    : QObject(parent), mRegistry(registry), mWorker(&ThumbnailService::WorkerMain, this)
{
}

ThumbnailService::~ThumbnailService()
{
    Shutdown();
}

void ThumbnailService::OpenProject(const EditorProjectContext &context)
{
    std::lock_guard lock(mMutex);
    mProject = context;
    mQueue.clear();
    mPending.clear();
    mCache.clear();
    mSourceHashes.clear();
}

void ThumbnailService::CloseProject(const QString &sessionId)
{
    std::lock_guard lock(mMutex);
    if (mProject.sessionId != sessionId)
        return;
    mProject = {};
    mQueue.clear();
    mPending.clear();
    mCache.clear();
    mSourceHashes.clear();
}

QImage ThumbnailService::Thumbnail(const QString &sourcePath)
{
    const AssetRecord record = mRegistry.RecordForSource(sourcePath);
    if (record.guid.isEmpty())
        return {};
    const QString cacheKey = record.guid;
    if (QImage *cached = mCache.object(cacheKey))
    {
        if (mSourceHashes.value(cacheKey) == record.sourceHash)
            return *cached;
        mCache.remove(cacheKey);
        mSourceHashes.remove(cacheKey);
    }

    std::lock_guard lock(mMutex);
    if (!mProject.IsValid() || mPending.contains(cacheKey))
        return {};
    mPending.insert(cacheKey);
    mQueue.push_back(
        {cacheKey, sourcePath, mProject.projectRoot, mProject.sessionId, record.sourceHash});
    mWake.notify_one();
    return {};
}

void ThumbnailService::Shutdown()
{
    {
        std::lock_guard lock(mMutex);
        if (mStopping)
            return;
        mStopping = true;
        mQueue.clear();
        mPending.clear();
    }
    mWake.notify_all();
    if (mWorker.joinable())
        mWorker.join();
}

void ThumbnailService::WorkerMain()
{
    for (;;)
    {
        Request request;
        {
            std::unique_lock lock(mMutex);
            mWake.wait(lock,
                       [this]
                       {
                           return mStopping || !mQueue.empty();
                       });
            if (mStopping)
                return;
            request = mQueue.front();
            mQueue.pop_front();
        }

        AssetProcess::TextureMeta meta;
        QImage image;
        if (AssetProcess::TextureMetaSerializer::LoadFromYAML(
                meta, (request.sourcePath + ".meta").toStdString()))
        {
            const QString thumbnailPath =
                QString::fromStdString(AssetProcess::TextureImporter::GetThumbnailFilePath(
                    meta.sourceFileHash, request.projectRoot.toStdString()));
            image.load(thumbnailPath);
        }
        QMetaObject::invokeMethod(
            this,
            [this, request, image]
            {
                StoreThumbnail(request.guid, request.sourcePath, request.sessionId,
                               request.sourceHash, image);
            },
            Qt::QueuedConnection);
    }
}

void ThumbnailService::StoreThumbnail(const QString &guid, const QString &sourcePath,
                                      const QString &sessionId, const QString &sourceHash,
                                      const QImage &image)
{
    {
        std::lock_guard lock(mMutex);
        mPending.remove(guid);
        if (mProject.sessionId != sessionId)
            return;
    }
    if (image.isNull())
        return;
    mCache.insert(guid, new QImage(image));
    mSourceHashes.insert(guid, sourceHash);
    emit ThumbnailReady(sourcePath);
}
