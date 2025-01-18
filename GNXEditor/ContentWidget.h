#pragma once

#include <QWidget>
#include <QFileSystemModel>
#include <QListView>
#include <QString>
#include <QDockWidget>
#include <QPushButton>
#include <QMenu>
#include <QMainWindow>

//内容浏览器的窗口

class ContentWidget : public QWidget
{
	Q_OBJECT
public:
	ContentWidget(QDockWidget* parent, const QString& currentDir);

private slots:
	void onDoubleClicked(const QModelIndex& index);

	void onBackClicked();

	void showContextMenu(const QPoint& pos);

	void OpenImportAssetDialog();

private:
	QFileSystemModel* mModel = nullptr;
	QListView* mListView = nullptr;
	QPushButton* mBackButton = nullptr;
	QString mInitDir;
	QString mCurrentDir;
};

