#pragma once
#include "Project/EditorProjectContext.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <thread>
struct AssetImportRequest
{
    QString sourceFile;
    QString destinationDirectory;
    QString projectRoot;
    QString assetRoot;
    QString cacheRoot;
    QString projectSessionId;
};
struct EditorAssetImportResult
{
    bool success = false;
    QString sourceFile;
    QString destinationDirectory;
    QStringList generatedAssets;
    QString errorCode;
    QString errorMessage;
};
Q_DECLARE_METATYPE(EditorAssetImportResult)
class AssetImportService final : public QObject
{
    Q_OBJECT
  public:
    explicit AssetImportService(QObject *parent = nullptr);
    ~AssetImportService() override;
    bool Enqueue(const AssetImportRequest &request);
    bool EnqueueFile(const QString &sourceFile, const QString &destinationDirectory);
    void OpenProject(const EditorProjectContext &context);
    void CloseProject(const QString &sessionId);
    void Shutdown();
    int PendingTaskCount() const;
  signals:
    void ImportStarted(const QString &sourceFile);
    void ImportFinished(const QString &sourceFile, const QString &destinationDirectory,
                        const QStringList &generatedAssets);
    void ImportFailed(const QString &sourceFile, const QString &errorMessage);
    void ImportCompleted(const EditorAssetImportResult &result);
    void ImportQueueChanged(int pendingCount, bool taskRunning);

  private:
    struct WorkerState;
    static void WorkerMain(const std::shared_ptr<WorkerState> &state, AssetImportService *service);
    static QString Key(const AssetImportRequest &request);
    std::shared_ptr<WorkerState> mState;
    std::thread mWorker;
};
