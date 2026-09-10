#pragma once

#include <QSortFilterProxyModel>
class AssetRegistry;
class QFileSystemModel;

class AssetBrowserModel final : public QSortFilterProxyModel
{
    Q_OBJECT
  public:
    AssetBrowserModel(QFileSystemModel &fileSystem, AssetRegistry &registry,
                      QObject *parent = nullptr);
    QString FilePath(const QModelIndex &index) const;

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

  private:
    QFileSystemModel &mFileSystem;
    AssetRegistry &mRegistry;
};
