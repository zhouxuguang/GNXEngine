#include "AssetBrowserModel.h"
#include "AssetRegistry.h"
#include <QFileSystemModel>

AssetBrowserModel::AssetBrowserModel(QFileSystemModel &fileSystem, AssetRegistry &registry,
                                     QObject *parent)
    : QSortFilterProxyModel(parent), mFileSystem(fileSystem), mRegistry(registry)
{
    setSourceModel(&mFileSystem);
    connect(&mRegistry, &AssetRegistry::RegistryReset, this, &AssetBrowserModel::invalidate);
    connect(&mRegistry, &AssetRegistry::AssetChanged, this,
            [this]
            {
                invalidate();
            });
}

QString AssetBrowserModel::FilePath(const QModelIndex &index) const
{
    return mFileSystem.filePath(mapToSource(index));
}

bool AssetBrowserModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = mFileSystem.index(row, 0, parent);
    const QString name = mFileSystem.fileName(index);
    if (mFileSystem.isDir(index))
        return name != ".gnx";
    if (name.endsWith(".meta", Qt::CaseInsensitive) ||
        name.endsWith(".gnxmeta", Qt::CaseInsensitive))
        return false;
    return mRegistry.ContainsSource(mFileSystem.filePath(index));
}
