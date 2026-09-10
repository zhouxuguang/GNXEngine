#pragma once

#include "Project/EditorProjectContext.h"
#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVector>

struct AssetRecord
{
    QString guid;
    QString type;
    QString sourcePath;
    QString relativePath;
    QStringList artifacts;
    QStringList dependencies;
    QString importState = QStringLiteral("SourceOnly");
    QJsonObject importSettings;
    QMap<QString, QStringList> platformArtifacts;
    qint64 sourceModified = 0;
    QString sourceHash;
};

class AssetRegistry final : public QObject
{
    Q_OBJECT
  public:
    explicit AssetRegistry(QObject *parent = nullptr);
    bool OpenProject(const EditorProjectContext &context);
    void CloseProject();
    bool Refresh();
    bool RegisterImported(const QString &sourceFile, const QString &destinationDirectory,
                          const QStringList &generatedAssets = {});
    bool MoveAsset(const QString &sourcePath, const QString &destinationPath);
    bool SetDependencies(const QString &sourcePath, const QStringList &dependencyGuids);
    bool SetImportSettings(const QString &sourcePath, const QJsonObject &settings);
    QVector<AssetRecord> Records() const
    {
        return mRecords;
    }
    bool ContainsSource(const QString &sourcePath) const;
    AssetRecord RecordForSource(const QString &sourcePath) const;
    const EditorProjectContext &Project() const
    {
        return mProject;
    }

  signals:
    void RegistryReset();
    void AssetChanged(const QString &relativePath);
    void RegistryError(const QString &message);

  private:
    bool Load();
    bool Save() const;
    AssetRecord BuildRecord(const QString &sourcePath);
    void RebuildWatchList();
    void SyncRuntimeMappings() const;
    int FindBySource(const QString &sourcePath) const;

    EditorProjectContext mProject;
    QVector<AssetRecord> mRecords;
    QFileSystemWatcher mWatcher;
    QTimer mRefreshTimer;
};
