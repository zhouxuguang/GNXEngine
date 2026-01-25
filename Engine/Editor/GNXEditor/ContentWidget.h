#pragma once

#include <QWidget>
#include <QFileSystemModel>
#include <QListView>
#include <QString>
#include <QDockWidget>
#include <QPushButton>
#include <QMenu>
#include <QMainWindow>
#include <QLabel>

//内容浏览器的窗口

class ContentWidget : public QWidget
{
	Q_OBJECT
public:
	ContentWidget(QDockWidget* parent, const QString& currentDir);

	void SetRootPath(const QString& path);

private slots:
	void onDoubleClicked(const QModelIndex& index);

	void onBackClicked();

	void showContextMenu(const QPoint& pos);

	void OpenImportAssetDialog();

private:
	void UpdatePathLabel();

	QFileSystemModel* mModel = nullptr;
	QListView* mListView = nullptr;
	QPushButton* mBackButton = nullptr;
	QLabel* mPathLabel = nullptr;
	QString mInitDir;
	QString mCurrentDir;
};

