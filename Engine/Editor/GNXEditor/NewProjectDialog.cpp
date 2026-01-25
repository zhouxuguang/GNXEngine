//
//  NewProjectDialog.cpp
//  GNXEngine
//

#include "NewProjectDialog.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"
#include <QMessageBox>
#include <QDir>

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    SetupUI();

    setWindowTitle("新建工程");
    setModal(true);
    resize(500, 150);
}

NewProjectDialog::~NewProjectDialog()
{
}

void NewProjectDialog::SetupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 工程名称
    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("工程名称:");
    mProjectNameEdit = new QLineEdit();
    mProjectNameEdit->setPlaceholderText("输入工程名称（如：MyGame）");

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(mProjectNameEdit);

    mainLayout->addLayout(nameLayout);

    // 工程路径
    QHBoxLayout* pathLayout = new QHBoxLayout();
    QLabel* pathLabel = new QLabel("工程路径:");
    mProjectPathEdit = new QLineEdit();
    mProjectPathEdit->setPlaceholderText("选择工程保存位置");
    mProjectPathEdit->setText(QDir::homePath() + "/GNXEngine"); // 默认为用户主目录

    mBrowseButton = new QPushButton("浏览...");

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(mProjectPathEdit);
    pathLayout->addWidget(mBrowseButton);

    mainLayout->addLayout(pathLayout);

    // 状态标签
    mStatusLabel = new QLabel("");
    mStatusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(mStatusLabel);

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    mCreateButton = new QPushButton("创建");
    mCreateButton->setEnabled(false);

    mCancelButton = new QPushButton("取消");

    buttonLayout->addWidget(mCreateButton);
    buttonLayout->addWidget(mCancelButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(mProjectNameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::ValidateInput);
    connect(mProjectPathEdit, &QLineEdit::textChanged, this, &NewProjectDialog::ValidateInput);
    connect(mBrowseButton, &QPushButton::clicked, this, &NewProjectDialog::OnBrowseButtonClicked);
    connect(mCreateButton, &QPushButton::clicked, this, &NewProjectDialog::OnCreateButtonClicked);
    connect(mCancelButton, &QPushButton::clicked, this, &NewProjectDialog::OnCancelButtonClicked);
}

QString NewProjectDialog::GetProjectName() const
{
    return mProjectNameEdit->text();
}

QString NewProjectDialog::GetProjectPath() const
{
    return mProjectPathEdit->text();
}

void NewProjectDialog::OnBrowseButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择工程保存位置", QDir::homePath());

    if (!dir.isEmpty())
    {
        mProjectPathEdit->setText(dir);
    }
}

void NewProjectDialog::OnCreateButtonClicked()
{
    QString projectName = GetProjectName();
    QString projectPath = GetProjectPath();

    if (projectName.isEmpty() || projectPath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "工程名称和路径不能为空！");
        return;
    }

    // 构建完整的工程路径
    QString fullProjectPath = QDir(projectPath).filePath(projectName);

    // 检查工程是否已存在
    QDir dir(fullProjectPath);
    if (dir.exists())
    {
        QMessageBox::warning(this, "错误", "工程已存在，请选择其他名称或路径！");
        return;
    }

    // 调用工程管理器创建工程
    bool success = GNXEngine::ProjectManager::GetInstance().CreateNewProject(
        fullProjectPath.toStdString(),
        projectName.toStdString()
    );

    if (success)
    {
        QMessageBox::information(this, "成功", "工程创建成功！");
        accept();
    }
    else
    {
        QMessageBox::critical(this, "错误", "工程创建失败！");
    }
}

void NewProjectDialog::OnCancelButtonClicked()
{
    reject();
}

void NewProjectDialog::ValidateInput()
{
    QString projectName = mProjectNameEdit->text();
    QString projectPath = mProjectPathEdit->text();

    bool isValid = true;
    QString errorMsg = "";

    // 验证工程名称
    if (projectName.isEmpty())
    {
        isValid = false;
        errorMsg = "请输入工程名称";
    }
    else if (projectName.contains('/') || projectName.contains('\\') || projectName.contains(':'))
    {
        isValid = false;
        errorMsg = "工程名称不能包含特殊字符";
    }

    // 验证工程路径
    if (projectPath.isEmpty())
    {
        isValid = false;
        errorMsg = "请选择工程路径";
    }
    else if (!QDir(projectPath).exists())
    {
        isValid = false;
        errorMsg = "工程路径不存在";
    }

    // 更新状态标签
    if (isValid)
    {
        mStatusLabel->setText("");
        mStatusLabel->setStyleSheet("");
    }
    else
    {
        mStatusLabel->setText(errorMsg);
        mStatusLabel->setStyleSheet("color: red;");
    }

    mCreateButton->setEnabled(isValid);
}
