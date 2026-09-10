#pragma once

#include <QWidget>

class AssetBrowserModel;
class AssetImportController;
class AssetImportService;
class AssetRegistry;
class EditorSettings;
class QFileSystemModel;
class QLabel;
class QListView;
class QModelIndex;
class QPushButton;
class QDockWidget;
class TextureItemDelegate;
class ThumbnailService;

class ContentWidget final : public QWidget
{
    Q_OBJECT
  public:
    ContentWidget(QDockWidget *parent, const QString &currentDir, AssetImportService &importService,
                  AssetRegistry &assetRegistry, ThumbnailService &thumbnailService,
                  EditorSettings &settings);
    void SetRootPath(const QString &path);

  private slots:
    void onDoubleClicked(const QModelIndex &index);
    void onBackClicked();
    void showContextMenu(const QPoint &pos);
    void OpenImportAssetDialog();

  private:
    void UpdatePathLabel();

    QFileSystemModel *mModel = nullptr;
    AssetBrowserModel *mProxyModel = nullptr;
    QListView *mListView = nullptr;
    TextureItemDelegate *mThumbnailDelegate = nullptr;
    QPushButton *mBackButton = nullptr;
    QLabel *mPathLabel = nullptr;
    QString mInitDir;
    QString mCurrentDir;
    AssetImportService &mImportService;
    AssetRegistry &mAssetRegistry;
    EditorSettings &mSettings;
    AssetImportController *mImportController = nullptr;
};
