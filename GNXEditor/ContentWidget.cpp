#include "ContentWidget.h"
#include <QPushButton>
#include <QFileDialog>

#include "ImageCodec/ImageDecoder.h"
#include "AssetProcess/AssetImporter.h"

ContentWidget::ContentWidget(QDockWidget* parent, const QString& currentDir)
	: QWidget(parent),
	mModel(new QFileSystemModel(parent)), 
	mListView(new QListView(parent))
{
	mModel->setRootPath(currentDir);
	mCurrentDir = currentDir;
	mInitDir = currentDir;

	// 创建QListView并设置相关属性
	mListView->setModel(mModel);
	mListView->setRootIndex(mModel->index(currentDir));
	mListView->setViewMode(QListView::IconMode);
	mListView->setIconSize(QSize(50, 50));
	mListView->setResizeMode(QListView::Adjust);
	mListView->setSpacing(3);
	// 启用上下文菜单
	mListView->setContextMenuPolicy(Qt::CustomContextMenu);

	mBackButton = new QPushButton("back", parent);
	mBackButton->setEnabled(false); // 初始状态禁用
	mBackButton->move(120, 0);

	// 连接双击信号到槽函数
	connect(mListView, &QListView::doubleClicked, this, &ContentWidget::onDoubleClicked);
	// 连接后退按钮的槽函数
	connect(mBackButton, &QPushButton::clicked, this, &ContentWidget::onBackClicked);
	// 连接空白地方弹出菜单的槽函数
	connect(mListView, &QListView::customContextMenuRequested, this, &ContentWidget::showContextMenu);

	parent->setWidget(mListView);
}

void ContentWidget::onDoubleClicked(const QModelIndex& index)
{
	if (mModel->isDir(index))
	{
		QString path = mModel->filePath(index);
		mCurrentDir = path;
		mBackButton->setEnabled(true);
		mListView->setRootIndex(mModel->index(path));
	}
}

void ContentWidget::onBackClicked()
{
	QDir dir(mCurrentDir);
	dir.cd(".."); // 返回上一级目录
	QString parentPath = dir.absolutePath();
	mListView->setRootIndex(mModel->index(parentPath));
	mCurrentDir = parentPath;

	// 如果回到初始目录，则禁用返回按钮
	if (mCurrentDir == mInitDir)
	{
		mBackButton->setEnabled(false);
	}
}

void ContentWidget::showContextMenu(const QPoint& pos)
{
	QModelIndex index = mListView->indexAt(pos);

	// 如果点击的是空白区域（没有选中任何项），则显示自定义菜单
	if (!index.isValid())
	{
		QMenu menu(this);

		QAction* refreshAction = new QAction("ImportAsset", this);
		connect(refreshAction, &QAction::triggered, this, &ContentWidget::OpenImportAssetDialog);
		menu.addAction(refreshAction);

		QAction* exitAction = new QAction("退出", this);
		connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
		menu.addAction(exitAction);

		// 在鼠标位置显示菜单
		menu.exec(mListView->viewport()->mapToGlobal(pos));
	}
	// 如果点击的是某个文件或文件夹，可以在这里处理相应的右键菜单
}

void ContentWidget::OpenImportAssetDialog()
{
	// 使用 QFileDialog::getOpenFileName 显示文件打开对话框
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"导入资产",          // 对话框标题
		QDir::homePath(),       // 默认目录
		"模型文件 (*.obj *.fbx *.gltf *.glb);;图像文件 (*.png *.jpg *.bmp *.tga *.hdr *.webp)" // 文件过滤器
	);

	//选择了文件，进行导入操作
	if (!filePath.isEmpty())
	{
		AssetProcess::AssetImporter assetImporter;
		assetImporter.ImportFromFile(filePath.toStdString(), mCurrentDir.toStdString());
		/*imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
		imagecodec::ImageDecoder::DecodeFile(filePath.toUtf8().constData(), image.get());*/
	}
}
