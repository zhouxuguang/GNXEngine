#include "ContentWidget.h"
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/AssetProcess/include/AssetImporter.h"

ContentWidget::ContentWidget(QDockWidget* parent, const QString& currentDir)
	: QWidget(parent),
	mModel(new QFileSystemModel(this)),
	mListView(new QListView(this))
{
	mModel->setRootPath(currentDir);
	mCurrentDir = currentDir;
	mInitDir = currentDir;

	// 创建QListView并设置相关属性
	mListView->setModel(mModel);
	mListView->setRootIndex(mModel->index(currentDir));
	mListView->setViewMode(QListView::IconMode);
	mListView->setIconSize(QSize(64, 64));
	mListView->setResizeMode(QListView::Adjust);
	mListView->setSpacing(8);
	mListView->setGridSize(QSize(80, 100));
	// 启用上下文菜单
	mListView->setContextMenuPolicy(Qt::CustomContextMenu);
	mListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// 创建Back按钮
	mBackButton = new QPushButton("← Back", this);
	mBackButton->setEnabled(false); // 初始状态禁用
	mBackButton->setMinimumWidth(80);
	mBackButton->setToolTip("返回上一级目录");
	mBackButton->setStyleSheet(
		"QPushButton {"
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
		"}"
	);

	// 创建路径显示标签
	mPathLabel = new QLabel(this);
	mPathLabel->setText(QDir(currentDir).dirName());
	mPathLabel->setWordWrap(true);
	mPathLabel->setToolTip(currentDir);
	mPathLabel->setStyleSheet(
		"QLabel {"
		"  padding: 5px;"
		"  background-color: #fafafa;"
		"  border: 1px solid #e0e0e0;"
		"  border-radius: 3px;"
		"  color: #666666;"
		"  font-size: 11px;"
		"}"
	);

	// 创建顶部工具栏
	QHBoxLayout* toolbarLayout = new QHBoxLayout();
	toolbarLayout->setContentsMargins(5, 5, 5, 5);
	toolbarLayout->setSpacing(5);
	toolbarLayout->addWidget(mBackButton);
	toolbarLayout->addWidget(mPathLabel, 1); // 拉伸因子为1，占据剩余空间

	// 创建主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addLayout(toolbarLayout);
	mainLayout->addWidget(mListView, 1); // 拉伸因子为1，占据剩余空间

	// 连接双击信号到槽函数
	connect(mListView, &QListView::doubleClicked, this, &ContentWidget::onDoubleClicked);
	// 连接后退按钮的槽函数
	connect(mBackButton, &QPushButton::clicked, this, &ContentWidget::onBackClicked);
	// 连接空白地方弹出菜单的槽函数
	connect(mListView, &QListView::customContextMenuRequested, this, &ContentWidget::showContextMenu);

	parent->setWidget(this);
}

void ContentWidget::UpdatePathLabel()
{
	if (mPathLabel)
	{
		QString displayPath = mCurrentDir;
		// 如果路径太长，只显示最后几个目录
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

void ContentWidget::SetRootPath(const QString& path)
{
	mModel->setRootPath(path);
	mCurrentDir = path;
	mInitDir = path;
	mListView->setRootIndex(mModel->index(path));
	mBackButton->setEnabled(false);
	UpdatePathLabel();
}

void ContentWidget::onDoubleClicked(const QModelIndex& index)
{
	if (mModel->isDir(index))
	{
		QString path = mModel->filePath(index);
		mCurrentDir = path;
		mBackButton->setEnabled(true);
		mListView->setRootIndex(mModel->index(path));
		UpdatePathLabel();
	}
}

void ContentWidget::onBackClicked()
{
	QDir dir(mCurrentDir);
	dir.cd(".."); // 返回上一级目录
	QString parentPath = dir.absolutePath();
	mListView->setRootIndex(mModel->index(parentPath));
	mCurrentDir = parentPath;
	UpdatePathLabel();

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

        QAction* refreshAction = new QAction("导入资产", this);
        connect(refreshAction, &QAction::triggered, this, &ContentWidget::OpenImportAssetDialog);
        menu.addAction(refreshAction);

        QAction* exitAction = new QAction("退出", this);
        connect(exitAction, &QAction::triggered, [this]() { this->window()->close(); });
        menu.addAction(exitAction);

		// 在鼠标位置显示菜单
		menu.exec(mListView->viewport()->mapToGlobal(pos));
	}
	// 如果点击的是某个文件或文件夹，可以在这里处理相应的右键菜单
}

void ContentWidget::OpenImportAssetDialog()
{
	// 读取历史记录
	QSettings settings("GNXEngine", "GNXEngine");
	QStringList history = settings.value("FileDialogHistory").toStringList();
	QString lastDir = history.isEmpty() ? QDir::homePath() : history.first();

	// 使用 QFileDialog::getOpenFileName 显示文件打开对话框
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"导入资产",          // 对话框标题
		lastDir,       // 默认目录
		"模型文件 (*.obj *.fbx *.gltf *.glb);;图像文件 (*.png *.jpg *.bmp *.tga *.hdr *.webp)" // 文件过滤器
	);

	if (!filePath.isEmpty())
	{
		// 更新历史记录
		QFileInfo fileInfo(filePath);
		QString dirPath = fileInfo.absolutePath();

		// 移重复项并添加到开头
		history.removeAll(dirPath);
		history.prepend(dirPath);

		// 限制历史记录数量
		while (history.size() > 10) 
		{
			history.removeLast();
		}

		// 保存历史
		settings.setValue("FileDialogHistory", history);
	}

	

	//选择了文件，进行导入操作
	if (!filePath.isEmpty())
	{
		AssetProcess::AssetImporter assetImporter;
		assetImporter.ImportFromFile(filePath.toStdString(), mCurrentDir.toStdString());
		/*imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
		imagecodec::ImageDecoder::DecodeFile(filePath.toUtf8().constData(), image.get());*/
	}
}
