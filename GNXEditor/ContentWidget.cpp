#include "ContentWidget.h"
#include <QPushButton>

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
	mListView->setIconSize(QSize(100, 100));
	mListView->setResizeMode(QListView::Adjust);
	mListView->setSpacing(10);

	mBackButton = new QPushButton("back", parent);
	mBackButton->setEnabled(false); // 初始状态禁用
	mBackButton->move(120, 0);

	// 连接双击信号到槽函数
	connect(mListView, &QListView::doubleClicked, this, &ContentWidget::onDoubleClicked);
	connect(mBackButton, &QPushButton::clicked, this, &ContentWidget::onBackClicked);

	parent->setWidget(mListView);
}
