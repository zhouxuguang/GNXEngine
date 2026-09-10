#pragma once

#include "Project/EditorProjectContext.h"
#include <QCache>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QSet>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
class AssetRegistry;

class ThumbnailService final : public QObject
{
    Q_OBJECT
  public:
    explicit ThumbnailService(AssetRegistry &registry, QObject *parent = nullptr);
    ~ThumbnailService() override;
    void OpenProject(const EditorProjectContext &context);
    void CloseProject(const QString &sessionId);
    QImage Thumbnail(const QString &sourcePath);
    void Shutdown();

  signals:
    void ThumbnailReady(const QString &sourcePath);

  private:
    struct Request
    {
        QString guid;
        QString sourcePath;
        QString projectRoot;
        QString sessionId;
        QString sourceHash;
    };
    void WorkerMain();
    void StoreThumbnail(const QString &guid, const QString &sourcePath, const QString &sessionId,
                        const QString &sourceHash, const QImage &image);

    QCache<QString, QImage> mCache{128};
    QHash<QString, QString> mSourceHashes;
    std::mutex mMutex;
    std::condition_variable mWake;
    std::deque<Request> mQueue;
    QSet<QString> mPending;
    EditorProjectContext mProject;
    std::thread mWorker;
    bool mStopping = false;
    AssetRegistry &mRegistry;
};
