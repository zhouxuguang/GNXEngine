#include "ContentWidget.h"
#include "Application/EditorSettings.h"
#include "AssetBrowserModel.h"
#include "AssetImportController.h"
#include "AssetImportService.h"
#include "AssetPreviewService.h"
#include "AssetRegistry.h"
#include "TextureEditorDialog.h"
#include "TextureItemDelegate.h"
#include "ThumbnailService.h"
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ContentWidget::ContentWidget(QDockWidget *parent, const QString &currentDir,
                             AssetImportService &importService, AssetRegistry &assetRegistry,
                             ThumbnailService &thumbnailService, EditorSettings &settings)
    : QWidget(parent), mModel(new QFileSystemModel(this)),
      mProxyModel(new AssetBrowserModel(*mModel, assetRegistry, this)),
      mListView(new QListView(this)),
      mThumbnailDelegate(new TextureItemDelegate(thumbnailService, this)),
      mImportService(importService), mAssetRegistry(assetRegistry), mSettings(settings)
{
    mImportController = new AssetImportController(importService, settings, this);
    mModel->setRootPath(currentDir);
    mCurrentDir = currentDir;
    mInitDir = currentDir;

    mListView->setModel(mProxyModel);
    mListView->setRootIndex(mProxyModel->mapFromSource(mModel->index(currentDir)));
    mListView->setViewMode(QListView::IconMode);
    mListView->setIconSize(QSize(64, 64));
    mListView->setResizeMode(QListView::Adjust);
    mListView->setSpacing(8);
    mListView->setGridSize(QSize(100, 120));
    mListView->setItemDelegate(mThumbnailDelegate);
    mListView->setContextMenuPolicy(Qt::CustomContextMenu);
    mListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mBackButton = new QPushButton("← Back", this);
    mBackButton->setEnabled(false);
    mBackButton->setMinimumWidth(80);
    mBackButton->setToolTip("返回上一级目录");
    mBackButton->setStyleSheet("QPushButton {"
                               "  padding: 6px 12px;"
                               "  background-color: #2196F3;"
                               "  border: none;"
                               "  border-radius: 4px;"
                               "  font-weight: 600;"
                               "  color: white;"
                               "  font-size: 13px;"
                               "}"
                               "QPushButton:hover {"
                               "  background-color: #1976D2;"
                               "}"
                               "QPushButton:pressed {"
                               "  background-color: #0D47A1;"
                               "}"
                               "QPushButton:disabled {"
                               "  background-color: #BDBDBD;"
                               "  color: #757575;"
                               "}");

    mPathLabel = new QLabel(this);
    mPathLabel->setText(QDir(currentDir).dirName());
    mPathLabel->setWordWrap(true);
    mPathLabel->setToolTip(currentDir);
    mPathLabel->setStyleSheet("QLabel {"
                              "  padding: 5px;"
                              "  background-color: #fafafa;"
                              "  border: 1px solid #e0e0e0;"
                              "  border-radius: 3px;"
                              "  color: #666666;"
                              "  font-size: 11px;"
                              "}");

    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(5, 5, 5, 5);
    toolbarLayout->setSpacing(5);
    toolbarLayout->addWidget(mBackButton);
    toolbarLayout->addWidget(mPathLabel, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(mListView, 1);

    connect(mListView, &QListView::doubleClicked, this, &ContentWidget::onDoubleClicked);
    connect(mBackButton, &QPushButton::clicked, this, &ContentWidget::onBackClicked);
    connect(mListView, &QListView::customContextMenuRequested, this,
            &ContentWidget::showContextMenu);
    connect(&thumbnailService, &ThumbnailService::ThumbnailReady, mListView,
            [this]
            {
                mListView->viewport()->update();
            });

    parent->setWidget(this);
    SetRootPath(currentDir);
}

void ContentWidget::UpdatePathLabel()
{
    if (mPathLabel)
    {
        QString displayPath = mCurrentDir;
        if (displayPath.length() > 60)
        {
            QStringList parts = displayPath.split('/');
            if (parts.size() > 3)
            {
                displayPath = ".../" + parts.mid(parts.size() - 2).join('/');
            }
        }
        mPathLabel->setText(displayPath);
        mPathLabel->setToolTip(mCurrentDir);
    }
}

void ContentWidget::SetRootPath(const QString &path)
{
    const bool hasProjectAssetRoot = !path.isEmpty() && QDir(path).exists();
    if (!hasProjectAssetRoot)
    {
        mCurrentDir.clear();
        mInitDir.clear();
        mListView->setRootIndex(QModelIndex());
        mListView->setVisible(false);
        mPathLabel->setText(tr("未打开工程"));
        mPathLabel->setToolTip(QString());
        mBackButton->setEnabled(false);
        return;
    }

    const QString assetRoot = QDir(path).absolutePath();
    mModel->setRootPath(assetRoot);
    mCurrentDir = assetRoot;
    mInitDir = assetRoot;
    mListView->setRootIndex(mProxyModel->mapFromSource(mModel->index(assetRoot)));
    mListView->setVisible(true);
    mBackButton->setEnabled(false);
    UpdatePathLabel();
}

void ContentWidget::onDoubleClicked(const QModelIndex &index)
{
    QModelIndex sourceIndex = mProxyModel->mapToSource(index);
    if (mModel->isDir(sourceIndex))
    {
        QString path = mModel->filePath(sourceIndex);
        mCurrentDir = path;
        mBackButton->setEnabled(true);
        mListView->setRootIndex(mProxyModel->mapFromSource(mModel->index(path)));
        UpdatePathLabel();
    }
    else
    {
        QString filePath = mModel->filePath(sourceIndex);

        if (filePath.endsWith(".png", Qt::CaseInsensitive) ||
            filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
            filePath.endsWith(".jpeg", Qt::CaseInsensitive) ||
            filePath.endsWith(".bmp", Qt::CaseInsensitive) ||
            filePath.endsWith(".tga", Qt::CaseInsensitive) ||
            filePath.endsWith(".webp", Qt::CaseInsensitive) ||
            filePath.endsWith(".hdr", Qt::CaseInsensitive) ||
            filePath.endsWith(".exr", Qt::CaseInsensitive))
        {
            QString error;
            const QImage image = AssetPreviewService::LoadSourceImage(filePath, &error);
            if (image.isNull())
            {
                QMessageBox::warning(this, tr("加载失败"), error);
                return;
            }
            auto *dialog = new TextureEditorDialog(image, filePath, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }
}

void ContentWidget::onBackClicked()
{
    QDir dir(mCurrentDir);
    dir.cd("..");
    QString parentPath = dir.absolutePath();
    mListView->setRootIndex(mProxyModel->mapFromSource(mModel->index(parentPath)));
    mCurrentDir = parentPath;
    UpdatePathLabel();

    if (mCurrentDir == mInitDir)
    {
        mBackButton->setEnabled(false);
    }
}

void ContentWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = mListView->indexAt(pos);

    if (!index.isValid())
    {
        QMenu menu(this);

        QAction *refreshAction = new QAction("导入资产", this);
        connect(refreshAction, &QAction::triggered, this, &ContentWidget::OpenImportAssetDialog);
        menu.addAction(refreshAction);

        menu.exec(mListView->viewport()->mapToGlobal(pos));
        return;
    }

    const QString sourcePath = mProxyModel->FilePath(index);
    if (QFileInfo(sourcePath).isDir())
        return;
    QMenu menu(this);
    menu.addAction(
        tr("重命名"), this,
        [this, sourcePath]
        {
            bool accepted = false;
            const QFileInfo info(sourcePath);
            const QString name =
                QInputDialog::getText(this, tr("重命名资源"), tr("文件名"), QLineEdit::Normal,
                                      info.fileName(), &accepted);
            if (!accepted || name.trimmed().isEmpty() || name.contains('/') || name.contains('\\'))
                return;
            const QString destination = info.dir().filePath(name.trimmed());
            if (!mAssetRegistry.MoveAsset(sourcePath, destination))
                QMessageBox::warning(this, tr("重命名失败"), tr("无法移动资源或同名文件已存在"));
        });
    menu.exec(mListView->viewport()->mapToGlobal(pos));
}

void ContentWidget::OpenImportAssetDialog()
{
    mImportController->ImportInteractive(this, mCurrentDir);
}
