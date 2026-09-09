#pragma once
#include <QObject>
#include <QString>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <set>
#include <thread>
struct AssetImportRequest { QString sourceFile; QString destinationDirectory; };
class AssetImportService final : public QObject {
  Q_OBJECT
public: explicit AssetImportService(QObject* parent=nullptr); ~AssetImportService() override;
  bool Enqueue(const AssetImportRequest& request); void Shutdown();
signals: void ImportStarted(const QString& sourceFile); void ImportFinished(const QString& sourceFile,const QString& outputAsset); void ImportFailed(const QString& sourceFile,const QString& errorMessage);
private: void WorkerMain(); static QString Key(const AssetImportRequest& request);
  std::mutex mMutex; std::condition_variable mWake; std::deque<AssetImportRequest> mQueue; std::set<QString> mPending;
  std::thread mWorker; bool mStopping=false;
};
