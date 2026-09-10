#include "AssetRegistry.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetProcess/include/TextureMetaFormat.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>
#include <algorithm>
#include <utility>

namespace
{
QString AssetTypeForSuffix(const QString &suffix)
{
    const QString value = suffix.toLower();
    if (QStringList{"png", "jpg", "jpeg", "bmp", "tga", "webp", "hdr", "exr"}.contains(value))
        return QStringLiteral("Texture");
    if (QStringList{"obj", "fbx", "gltf", "glb", "3ds"}.contains(value))
        return QStringLiteral("Mesh");
    return QStringLiteral("Unknown");
}
bool IsSafeRelativePath(const QString &path)
{
    const QString clean = QDir::cleanPath(path);
    return !clean.isEmpty() && !QDir::isAbsolutePath(clean) && clean != ".." &&
           !clean.startsWith("../") && !clean.startsWith("..\\");
}
} // namespace

AssetRegistry::AssetRegistry(QObject *parent) : QObject(parent)
{
    mRefreshTimer.setSingleShot(true);
    mRefreshTimer.setInterval(150);
    connect(&mRefreshTimer, &QTimer::timeout, this,
            [this]
            {
                Refresh();
            });
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
            [this]
            {
                mRefreshTimer.start();
            });
    connect(&mWatcher, &QFileSystemWatcher::fileChanged, this,
            [this]
            {
                mRefreshTimer.start();
            });
}

bool AssetRegistry::OpenProject(const EditorProjectContext &context)
{
    CloseProject();
    if (!context.IsValid())
        return false;
    mProject = context;
    QDir().mkpath(QDir(mProject.projectRoot).filePath(".gnx"));
    if (!Load())
        mRecords.clear();
    return Refresh();
}

void AssetRegistry::CloseProject()
{
    mRefreshTimer.stop();
    mWatcher.removePaths(mWatcher.directories());
    mWatcher.removePaths(mWatcher.files());
    mProject = {};
    mRecords.clear();
    emit RegistryReset();
}

bool AssetRegistry::Refresh()
{
    if (!mProject.IsValid())
        return false;
    QVector<AssetRecord> records;
    QDirIterator iterator(mProject.assetRoot, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        const QString path = iterator.next();
        if (path.endsWith(".meta", Qt::CaseInsensitive) ||
            path.endsWith(".gnxmeta", Qt::CaseInsensitive))
            continue;
        records.push_back(BuildRecord(path));
    }
    for (AssetRecord record : std::as_const(mRecords))
    {
        bool found = false;
        for (const AssetRecord &current : std::as_const(records))
        {
            if (current.guid == record.guid)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            record.importState = QStringLiteral("Missing");
            records.push_back(std::move(record));
        }
    }
    mRecords = std::move(records);
    const bool saved = Save();
    RebuildWatchList();
    SyncRuntimeMappings();
    emit RegistryReset();
    return saved;
}

bool AssetRegistry::MoveAsset(const QString &sourcePath, const QString &destinationPath)
{
    if (!mProject.IsValid() || FindBySource(sourcePath) < 0 || QFileInfo(destinationPath).exists())
        return false;
    const QString relative = QDir(mProject.assetRoot).relativeFilePath(destinationPath);
    if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative))
        return false;
    if (!QDir().mkpath(QFileInfo(destinationPath).absolutePath()) ||
        !QFile::rename(sourcePath, destinationPath))
        return false;
    QVector<QPair<QString, QString>> moved{{sourcePath, destinationPath}};
    for (const QString &suffix : {QStringLiteral(".gnxmeta"), QStringLiteral(".meta")})
    {
        const QString oldSidecar = sourcePath + suffix;
        if (QFileInfo(oldSidecar).exists() && !QFile::rename(oldSidecar, destinationPath + suffix))
        {
            for (auto item = moved.crbegin(); item != moved.crend(); ++item)
                QFile::rename(item->second, item->first);
            return false;
        }
        if (QFileInfo(destinationPath + suffix).exists())
            moved.push_back({oldSidecar, destinationPath + suffix});
    }
    return Refresh();
}

bool AssetRegistry::SetDependencies(const QString &sourcePath, const QStringList &dependencyGuids)
{
    const int index = FindBySource(sourcePath);
    if (index < 0)
        return false;
    mRecords[index].dependencies = dependencyGuids;
    mRecords[index].dependencies.removeDuplicates();
    return Save();
}

bool AssetRegistry::SetImportSettings(const QString &sourcePath, const QJsonObject &settings)
{
    const int index = FindBySource(sourcePath);
    if (index < 0)
        return false;
    mRecords[index].importSettings = settings;
    QSaveFile sidecar(sourcePath + ".gnxmeta");
    if (!sidecar.open(QIODevice::WriteOnly))
        return false;
    sidecar.write(QJsonDocument(QJsonObject{{"version", 1},
                                            {"guid", mRecords[index].guid},
                                            {"importSettings", settings}})
                      .toJson(QJsonDocument::Compact));
    return sidecar.commit() && Save();
}

bool AssetRegistry::RegisterImported(const QString &sourceFile, const QString &destinationDirectory,
                                     const QStringList &generatedAssets)
{
    if (!mProject.IsValid())
        return false;
    const QString imported = QDir(destinationDirectory).filePath(QFileInfo(sourceFile).fileName());
    if (!QFileInfo(imported).isFile())
        return false;
    AssetRecord record = BuildRecord(imported);
    record.importState = QStringLiteral("Imported");
    for (const QString &artifact : generatedAssets)
    {
        const QString absolute = QFileInfo(artifact).absoluteFilePath();
        const QString relative = QDir(mProject.projectRoot).relativeFilePath(absolute);
        if (absolute != QFileInfo(imported).absoluteFilePath() && QFileInfo(absolute).exists() &&
            IsSafeRelativePath(relative))
            record.artifacts.push_back(relative);
    }
    record.artifacts.removeDuplicates();
    record.platformArtifacts[QStringLiteral("Host")] = record.artifacts;
    const int index = FindBySource(imported);
    if (index >= 0)
        mRecords[index] = record;
    else
        mRecords.push_back(record);
    if (!Refresh())
        return false;
    QStringList dependencies;
    for (const QString &artifact : generatedAssets)
    {
        const AssetRecord dependency = RecordForSource(artifact);
        if (!dependency.guid.isEmpty() &&
            dependency.sourcePath.compare(imported, Qt::CaseInsensitive) != 0)
            dependencies.push_back(dependency.guid);
    }
    if (!dependencies.isEmpty())
    {
        const int refreshedIndex = FindBySource(imported);
        if (refreshedIndex >= 0)
        {
            dependencies.removeDuplicates();
            mRecords[refreshedIndex].dependencies = dependencies;
            if (!Save())
                return false;
        }
    }
    emit AssetChanged(record.relativePath);
    return true;
}

bool AssetRegistry::Load()
{
    QFile file(QDir(mProject.projectRoot).filePath(".gnx/AssetRegistry.json"));
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;
    const int version = document.object().value("version").toInt(1);
    if (version < 1 || version > 2)
        return false;
    QVector<AssetRecord> records;
    for (const QJsonValue &value : document.object().value("assets").toArray())
    {
        const QJsonObject object = value.toObject();
        AssetRecord record;
        record.guid = object.value("guid").toString();
        record.type = object.value("type").toString();
        record.relativePath = object.value("path").toString();
        if (!IsSafeRelativePath(record.relativePath))
            continue;
        record.sourcePath = QDir(mProject.assetRoot).filePath(record.relativePath);
        record.sourceModified = static_cast<qint64>(object.value("modified").toDouble());
        record.sourceHash = object.value("hash").toString();
        record.importState = object.value("importState").toString("SourceOnly");
        record.importSettings = object.value("importSettings").toObject();
        for (const QJsonValue &artifact : object.value("artifacts").toArray())
        {
            const QString path = artifact.toString();
            if (IsSafeRelativePath(path))
                record.artifacts.push_back(path);
        }
        for (const QJsonValue &dependency : object.value("dependencies").toArray())
            record.dependencies.push_back(dependency.toString());
        const QJsonObject platforms = object.value("platformArtifacts").toObject();
        for (auto platform = platforms.begin(); platform != platforms.end(); ++platform)
        {
            QStringList values;
            for (const QJsonValue &artifact : platform.value().toArray())
            {
                const QString path = artifact.toString();
                if (IsSafeRelativePath(path))
                    values.push_back(path);
            }
            record.platformArtifacts.insert(platform.key(), values);
        }
        if (!record.guid.isEmpty() && !record.relativePath.isEmpty())
            records.push_back(std::move(record));
    }
    mRecords = std::move(records);
    return true;
}

bool AssetRegistry::Save() const
{
    if (!mProject.IsValid())
        return false;
    QJsonArray assets;
    for (const AssetRecord &record : mRecords)
    {
        QJsonArray artifacts;
        for (const QString &artifact : record.artifacts)
            artifacts.push_back(artifact);
        QJsonArray dependencies;
        for (const QString &dependency : record.dependencies)
            dependencies.push_back(dependency);
        QJsonObject platformArtifacts;
        for (auto platform = record.platformArtifacts.cbegin();
             platform != record.platformArtifacts.cend(); ++platform)
        {
            QJsonArray values;
            for (const QString &artifact : platform.value())
                values.push_back(artifact);
            platformArtifacts.insert(platform.key(), values);
        }
        assets.push_back(QJsonObject{{"guid", record.guid},
                                     {"type", record.type},
                                     {"path", record.relativePath},
                                     {"modified", record.sourceModified},
                                     {"hash", record.sourceHash},
                                     {"artifacts", artifacts},
                                     {"dependencies", dependencies},
                                     {"importState", record.importState},
                                     {"importSettings", record.importSettings},
                                     {"platformArtifacts", platformArtifacts}});
    }
    QSaveFile file(QDir(mProject.projectRoot).filePath(".gnx/AssetRegistry.json"));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(QJsonObject{{"version", 2}, {"assets", assets}})
                   .toJson(QJsonDocument::Indented));
    return file.commit();
}

AssetRecord AssetRegistry::BuildRecord(const QString &sourcePath)
{
    QFileInfo info(sourcePath);
    const QString relative = QDir(mProject.assetRoot).relativeFilePath(sourcePath);
    AssetRecord record;
    const int existing = FindBySource(sourcePath);
    if (existing >= 0)
        record = mRecords[existing];
    const QString identityPath = sourcePath + ".gnxmeta";
    QFile identityFile(identityPath);
    if (identityFile.open(QIODevice::ReadOnly))
    {
        const QJsonObject identity = QJsonDocument::fromJson(identityFile.readAll()).object();
        record.guid = identity.value("guid").toString();
        record.importSettings = identity.value("importSettings").toObject();
        if (record.guid.isEmpty())
            emit RegistryError(tr("资源 meta 无效：%1").arg(identityPath));
    }
    if (existing < 0 && !record.guid.isEmpty())
    {
        for (const AssetRecord &previous : std::as_const(mRecords))
        {
            if (previous.guid == record.guid)
            {
                const QJsonObject settings = record.importSettings;
                record = previous;
                record.importSettings = settings;
                break;
            }
        }
    }
    if (record.guid.isEmpty())
    {
        record.guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSaveFile output(identityPath);
        if (output.open(QIODevice::WriteOnly))
        {
            output.write(QJsonDocument(QJsonObject{{"version", 1}, {"guid", record.guid}})
                             .toJson(QJsonDocument::Compact));
            output.commit();
        }
    }
    record.type = AssetTypeForSuffix(info.suffix());
    record.sourcePath = info.absoluteFilePath();
    record.relativePath = relative;
    record.sourceModified = info.lastModified().toMSecsSinceEpoch();
    record.sourceHash = QStringLiteral("%1:%2").arg(record.sourceModified).arg(info.size());
    const QString metaPath = sourcePath + ".meta";
    if (QFileInfo(metaPath).isFile())
    {
        record.artifacts.push_back(QDir(mProject.projectRoot).relativeFilePath(metaPath));
        AssetProcess::TextureMeta textureMeta;
        if (record.type == "Texture" &&
            AssetProcess::TextureMetaSerializer::LoadFromYAML(textureMeta, metaPath.toStdString()))
        {
            const QString cooked =
                QDir(mProject.cacheRoot)
                    .filePath(QString::number(textureMeta.sourceFileHash) + ".texture");
            const QString thumbnail =
                QString::fromStdString(AssetProcess::TextureImporter::GetThumbnailFilePath(
                    textureMeta.sourceFileHash, mProject.projectRoot.toStdString()));
            if (QFileInfo(cooked).isFile())
                record.artifacts.push_back(QDir(mProject.projectRoot).relativeFilePath(cooked));
            if (QFileInfo(thumbnail).isFile())
                record.artifacts.push_back(QDir(mProject.projectRoot).relativeFilePath(thumbnail));
            record.importState = QStringLiteral("Imported");
        }
    }
    record.artifacts.erase(
        std::remove_if(
            record.artifacts.begin(), record.artifacts.end(),
            [this](const QString &artifact)
            {
                return !QFileInfo(QDir(mProject.projectRoot).filePath(artifact)).exists();
            }),
        record.artifacts.end());
    record.artifacts.removeDuplicates();
    if (record.importState == "Imported")
        record.platformArtifacts[QStringLiteral("Host")] = record.artifacts;
    return record;
}

void AssetRegistry::RebuildWatchList()
{
    const QStringList oldDirectories = mWatcher.directories();
    if (!oldDirectories.isEmpty())
        mWatcher.removePaths(oldDirectories);
    QStringList directories{mProject.assetRoot};
    QDirIterator iterator(mProject.assetRoot, QDir::Dirs | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
        directories.push_back(iterator.next());
    mWatcher.addPaths(directories);
}

int AssetRegistry::FindBySource(const QString &sourcePath) const
{
    const QString normalized = QDir::cleanPath(QFileInfo(sourcePath).absoluteFilePath());
    for (int index = 0; index < mRecords.size(); ++index)
    {
        if (QDir::cleanPath(mRecords[index].sourcePath).compare(normalized, Qt::CaseInsensitive) ==
            0)
            return index;
    }
    return -1;
}

bool AssetRegistry::ContainsSource(const QString &sourcePath) const
{
    return FindBySource(sourcePath) >= 0;
}

AssetRecord AssetRegistry::RecordForSource(const QString &sourcePath) const
{
    const int index = FindBySource(sourcePath);
    return index >= 0 ? mRecords[index] : AssetRecord{};
}

void AssetRegistry::SyncRuntimeMappings() const
{
    for (const AssetRecord &record : mRecords)
    {
        if (record.importState == "Missing")
            continue;
        const QStringList hostArtifacts =
            record.platformArtifacts.value(QStringLiteral("Host"), record.artifacts);
        for (const QString &artifact : hostArtifacts)
        {
            const QString absolute = QDir(mProject.projectRoot).absoluteFilePath(artifact);
            if (!QFileInfo(absolute).isFile())
                continue;
            const QString suffix = QFileInfo(absolute).suffix().toLower();
            if (suffix == "texture" || suffix == "meshasset" || suffix == "gnxasset")
            {
                AssetManager::AssetManager::RegisterResourcePath(record.guid.toStdString(),
                                                                 absolute.toStdString());
                break;
            }
        }
    }
}
