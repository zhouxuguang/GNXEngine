#include "ContentWidget.h"
#include "AssetImportService.h"
#include "EditorSettings.h"
#include "TextureEditorDialog.h"
#include "TextureItemDelegate.h"
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "Runtime/ImageCodec/include/ImageDecoder.h"

// 代理模型：过滤掉.meta文件和.gnx目录
class FileSystemProxyModel : public QSortFilterProxyModel {
public:
  explicit FileSystemProxyModel(QObject *parent = nullptr)
      : QSortFilterProxyModel(parent) {}

protected:
  bool filterAcceptsRow(int sourceRow,
                        const QModelIndex &sourceParent) const override {
    QFileSystemModel *fileModel =
        qobject_cast<QFileSystemModel *>(sourceModel());
    if (!fileModel)
      return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);

    QModelIndex index = fileModel->index(sourceRow, 0, sourceParent);
    QString fileName = fileModel->fileName(index);

    // 过滤掉.gnx目录
    if (fileModel->isDir(index) && fileName == ".gnx")
      return false;

    // 过滤掉.meta文件
    if (!fileModel->isDir(index) && fileName.endsWith(".meta"))
      return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
  }
};

ContentWidget::ContentWidget(QDockWidget *parent, const QString &currentDir,
                             AssetImportService &importService,
                             EditorSettings &settings)
    : QWidget(parent), mModel(new QFileSystemModel(this)),
      mProxyModel(new FileSystemProxyModel(this)),
      mListView(new QListView(this)),
      mThumbnailDelegate(new TextureItemDelegate(this)),
      mImportService(importService), mSettings(settings) {
  mModel->setRootPath(currentDir);
  mCurrentDir = currentDir;
  mInitDir = currentDir;

  // 设置代理模型的源模型
  mProxyModel->setSourceModel(mModel);

  // 创建QListView并设置相关属性
  mListView->setModel(mProxyModel);
  mListView->setRootIndex(
      mProxyModel->mapFromSource(mModel->index(currentDir)));
  mListView->setViewMode(QListView::IconMode);
  mListView->setIconSize(QSize(64, 64));
  mListView->setResizeMode(QListView::Adjust);
  mListView->setSpacing(8);
  mListView->setGridSize(QSize(100, 120));
  // 设置自定义 delegate 来显示缩略图
  mListView->setItemDelegate(mThumbnailDelegate);
  // 启用上下文菜单
  mListView->setContextMenuPolicy(Qt::CustomContextMenu);
  mListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // 创建Back按钮
  mBackButton = new QPushButton("← Back", this);
  mBackButton->setEnabled(false); // 初始状态禁用
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

  // 创建路径显示标签
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

  // 创建顶部工具栏
  QHBoxLayout *toolbarLayout = new QHBoxLayout();
  toolbarLayout->setContentsMargins(5, 5, 5, 5);
  toolbarLayout->setSpacing(5);
  toolbarLayout->addWidget(mBackButton);
  toolbarLayout->addWidget(mPathLabel, 1); // 拉伸因子为1，占据剩余空间

  // 创建主布局
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  mainLayout->addLayout(toolbarLayout);
  mainLayout->addWidget(mListView, 1); // 拉伸因子为1，占据剩余空间

  // 连接双击信号到槽函数
  connect(mListView, &QListView::doubleClicked, this,
          &ContentWidget::onDoubleClicked);
  // 连接后退按钮的槽函数
  connect(mBackButton, &QPushButton::clicked, this,
          &ContentWidget::onBackClicked);
  // 连接空白地方弹出菜单的槽函数
  connect(mListView, &QListView::customContextMenuRequested, this,
          &ContentWidget::showContextMenu);

  parent->setWidget(this);
  SetRootPath(currentDir);
}

void ContentWidget::UpdatePathLabel() {
  if (mPathLabel) {
    QString displayPath = mCurrentDir;
    // 如果路径太长，只显示最后几个目录
    if (displayPath.length() > 60) {
      QStringList parts = displayPath.split('/');
      if (parts.size() > 3) {
        displayPath = ".../" + parts.mid(parts.size() - 2).join('/');
      }
    }
    mPathLabel->setText(displayPath);
    mPathLabel->setToolTip(mCurrentDir);
  }
}

void ContentWidget::SetRootPath(const QString &path) {
  const bool hasProjectAssetRoot = !path.isEmpty() && QDir(path).exists();
  if (!hasProjectAssetRoot) {
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
  mListView->setRootIndex(
      mProxyModel->mapFromSource(mModel->index(assetRoot)));
  mListView->setVisible(true);
  mBackButton->setEnabled(false);
  UpdatePathLabel();
}

void ContentWidget::onDoubleClicked(const QModelIndex &index) {
  QModelIndex sourceIndex = mProxyModel->mapToSource(index);
  if (mModel->isDir(sourceIndex)) {
    QString path = mModel->filePath(sourceIndex);
    mCurrentDir = path;
    mBackButton->setEnabled(true);
    mListView->setRootIndex(mProxyModel->mapFromSource(mModel->index(path)));
    UpdatePathLabel();
  } else {
    QString filePath = mModel->filePath(sourceIndex);

    // 检查是否为原始图像文件
    if (filePath.endsWith(".png", Qt::CaseInsensitive) ||
        filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
        filePath.endsWith(".jpeg", Qt::CaseInsensitive) ||
        filePath.endsWith(".bmp", Qt::CaseInsensitive) ||
        filePath.endsWith(".tga", Qt::CaseInsensitive) ||
        filePath.endsWith(".webp", Qt::CaseInsensitive) ||
        filePath.endsWith(".hdr", Qt::CaseInsensitive) ||
        filePath.endsWith(".exr", Qt::CaseInsensitive)) {
      // 使用ImageDecoder解码图像文件
      imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
      if (imagecodec::ImageDecoder::DecodeFile(filePath.toStdString().c_str(),
                                               image.get())) {
        // 将VImage转换为QImage
        QImage qimage;

        switch (image->GetFormat()) {
        case imagecodec::FORMAT_RGBA8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGBA8888);
          break;
        case imagecodec::FORMAT_RGB8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGB888);
          break;
        case imagecodec::FORMAT_SRGB8_ALPHA8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGBA8888);
          break;
        case imagecodec::FORMAT_SRGB8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGB888);
          break;
        case imagecodec::FORMAT_GRAY8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_Grayscale8);
          break;
        case imagecodec::FORMAT_GRAY8_ALPHA8:
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGBA8888);
          break;
        default:
          // 尝试按RGBA8处理
          qimage = QImage(image->GetImageData(), image->GetWidth(),
                          image->GetHeight(), image->GetBytesPerRow(),
                          QImage::Format_RGBA8888);
          break;
        }

        // 创建图像副本
        qimage = qimage.copy();

        if (!qimage.isNull()) {
          // 显示纹理查看器对话框
          TextureEditorDialog *dialog =
              new TextureEditorDialog(qimage, filePath, this);
          dialog->setAttribute(Qt::WA_DeleteOnClose);
          dialog->show();
        } else {
          QMessageBox::warning(this, "加载失败", "无法解码图像文件");
        }
      } else {
        QMessageBox::warning(this, "加载失败", "无法解码图像文件");
      }
    }
  }
}

void ContentWidget::onBackClicked() {
  QDir dir(mCurrentDir);
  dir.cd(".."); // 返回上一级目录
  QString parentPath = dir.absolutePath();
  mListView->setRootIndex(
      mProxyModel->mapFromSource(mModel->index(parentPath)));
  mCurrentDir = parentPath;
  UpdatePathLabel();

  // 如果回到初始目录，则禁用返回按钮
  if (mCurrentDir == mInitDir) {
    mBackButton->setEnabled(false);
  }
}

void ContentWidget::showContextMenu(const QPoint &pos) {
  QModelIndex index = mListView->indexAt(pos);

  // 如果点击的是空白区域（没有选中任何项），则显示自定义菜单
  if (!index.isValid()) {
    QMenu menu(this);

    QAction *refreshAction = new QAction("导入资产", this);
    connect(refreshAction, &QAction::triggered, this,
            &ContentWidget::OpenImportAssetDialog);
    menu.addAction(refreshAction);

    // 在鼠标位置显示菜单
    menu.exec(mListView->viewport()->mapToGlobal(pos));
  }
  // 如果点击的是某个文件或文件夹，可以在这里处理相应的右键菜单
}

void ContentWidget::OpenImportAssetDialog() {
  QString lastDir = mSettings.LastImportDirectory().isEmpty()
                        ? QDir::homePath()
                        : mSettings.LastImportDirectory();

  const QString filters =
      tr("模型文件 (*.obj *.fbx *.gltf *.glb);;图像文件 (*.png *.jpg *.bmp "
         "*.tga *.hdr *.exr *.webp)");
  QFileDialog fileDialog(this, tr("导入资产"), lastDir, filters);
  // 与工程选择保持一致，绕开 Windows Explorer Shell 的 COM/RPC 通道。
  fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setNameFilters(filters.split(";;"));

  QString filePath;
  if (fileDialog.exec() == QDialog::Accepted &&
      !fileDialog.selectedFiles().isEmpty()) {
    filePath = fileDialog.selectedFiles().constFirst();
  }

  if (!filePath.isEmpty()) {
    QFileInfo fileInfo(filePath);
    mSettings.SetLastImportDirectory(fileInfo.absolutePath());
  }

  // 选择了文件，进行导入操作
  if (!filePath.isEmpty()) {
    mImportService.Enqueue({filePath, mCurrentDir});
  }
}
