#include "AssetImportService.h"
#include "Runtime/AssetProcess/include/AssetImporter.h"
#include <QDir>
#include <QFileInfo>
AssetImportService::AssetImportService(QObject *parent)
    : QObject(parent), mWorker(&AssetImportService::WorkerMain, this) {}
AssetImportService::~AssetImportService() { Shutdown(); }
QString AssetImportService::Key(const AssetImportRequest &r) {
  return QDir::cleanPath(r.sourceFile) + "|" +
         QDir::cleanPath(r.destinationDirectory);
}
bool AssetImportService::Enqueue(const AssetImportRequest &r) {
  if (!QFileInfo::exists(r.sourceFile) || r.destinationDirectory.isEmpty()) {
    emit ImportFailed(r.sourceFile, tr("源文件不存在或目标目录无效"));
    return false;
  }
  std::lock_guard lock(mMutex);
  if (mStopping || mPending.contains(Key(r)))
    return false;
  mPending.insert(Key(r));
  mQueue.push_back(r);
  mWake.notify_one();
  return true;
}
void AssetImportService::Shutdown() {
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
void AssetImportService::WorkerMain() {
  for (;;) {
    AssetImportRequest r;
    {
      std::unique_lock lock(mMutex);
      mWake.wait(lock, [this] { return mStopping || !mQueue.empty(); });
      if (mStopping)
        return;
      r = mQueue.front();
      mQueue.pop_front();
    }
    emit ImportStarted(r.sourceFile);
    AssetProcess::AssetImporter importer;
    bool ok = importer.ImportFromFile(r.sourceFile.toStdString(),
                                      r.destinationDirectory.toStdString());
    {
      std::lock_guard lock(mMutex);
      mPending.erase(Key(r));
    }
    if (ok)
      emit ImportFinished(r.sourceFile, r.destinationDirectory);
    else
      emit ImportFailed(r.sourceFile, tr("资源导入器返回失败"));
  }
}
