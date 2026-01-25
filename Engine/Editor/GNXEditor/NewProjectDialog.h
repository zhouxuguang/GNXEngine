//
//  NewProjectDialog.h
//  GNXEngine
//
//  新建工程对话框
//

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);
    ~NewProjectDialog();

    // 获取工程名称
    QString GetProjectName() const;

    // 获取工程路径
    QString GetProjectPath() const;

private slots:
    void OnBrowseButtonClicked();
    void OnCreateButtonClicked();
    void OnCancelButtonClicked();

    // 验证输入
    void ValidateInput();

private:
    void SetupUI();

    QLineEdit* mProjectNameEdit = nullptr;
    QLineEdit* mProjectPathEdit = nullptr;
    QPushButton* mBrowseButton = nullptr;
    QPushButton* mCreateButton = nullptr;
    QPushButton* mCancelButton = nullptr;

    QLabel* mStatusLabel = nullptr;
};
