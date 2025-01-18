#pragma once

#include <QWidget>
#include <QFileSystemModel>
#include <QListView>
#include <QString>
#include <QDockWidget>
#include <QPushButton>

//内容浏览器的窗口

class ContentWidget : public QWidget
{
	Q_OBJECT
public:
	ContentWidget(QDockWidget* parent, const QString& currentDir);

private slots:
	void onDoubleClicked(const QModelIndex& index) 
	{
		if (mModel->isDir(index))
		{
			QString path = mModel->filePath(index);
			mCurrentDir = path;
			mListView->setRootIndex(mModel->index(path));
		}
	}

	void onBackClicked() 
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

private:
	QFileSystemModel* mModel = nullptr;
	QListView* mListView = nullptr;
	QPushButton* mBackButton = nullptr;
	QString mInitDir;
	QString mCurrentDir;
};

