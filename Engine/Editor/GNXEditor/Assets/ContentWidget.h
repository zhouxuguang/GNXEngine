#pragma once

#include <QDockWidget>
#include <QFileSystemModel>
#include <QLabel>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QString>
#include <QWidget>

// 前向声明
class TextureItemDelegate;
class AssetImportService;
class EditorSettings;

// 内容浏览器的窗口

class ContentWidget : public QWidget {
  Q_OBJECT
public:
  ContentWidget(QDockWidget *parent, const QString &currentDir,
                AssetImportService &importService, EditorSettings &settings);

  void SetRootPath(const QString &path);

private slots:
  void onDoubleClicked(const QModelIndex &index);

  void onBackClicked();

  void showContextMenu(const QPoint &pos);

  void OpenImportAssetDialog();

private:
  void UpdatePathLabel();

  QFileSystemModel *mModel = nullptr;
  QSortFilterProxyModel *mProxyModel = nullptr;
  QListView *mListView = nullptr;
  TextureItemDelegate *mThumbnailDelegate = nullptr;
  QPushButton *mBackButton = nullptr;
  QLabel *mPathLabel = nullptr;
  QString mInitDir;
  QString mCurrentDir;
  AssetImportService &mImportService;
  EditorSettings &mSettings;
};
