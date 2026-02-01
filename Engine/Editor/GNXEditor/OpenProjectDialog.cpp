//
//  OpenProjectDialog.cpp
//  GNXEngine
//

#include "OpenProjectDialog.h"
#include "EditorConfig.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

OpenProjectDialog::OpenProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    // 加载编辑器配置
    EditorConfig::GetInstance().LoadConfig();

    SetupUI();

    setWindowTitle("打开工程");
    setModal(true);
    resize(600, 400);

    LoadRecentProjects();
}

OpenProjectDialog::~OpenProjectDialog()
{
}

void OpenProjectDialog::SetupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 最近工程列表
    QLabel* recentLabel = new QLabel("最近打开的工程:");
    mainLayout->addWidget(recentLabel);

    mRecentProjectsList = new QListWidget();
    mainLayout->addWidget(mRecentProjectsList);

    // 状态标签
    mStatusLabel = new QLabel("");
    mStatusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(mStatusLabel);

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    mBrowseButton = new QPushButton("浏览...");
    buttonLayout->addWidget(mBrowseButton);

    buttonLayout->addStretch();

    mOpenButton = new QPushButton("打开");
    mOpenButton->setEnabled(false);

    mCancelButton = new QPushButton("取消");

    buttonLayout->addWidget(mOpenButton);
    buttonLayout->addWidget(mCancelButton);

    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(mRecentProjectsList, &QListWidget::itemSelectionChanged, this, &OpenProjectDialog::OnProjectSelectionChanged);
    connect(mRecentProjectsList, &QListWidget::itemDoubleClicked, this, &OpenProjectDialog::OnOpenButtonClicked);
    connect(mBrowseButton, &QPushButton::clicked, this, &OpenProjectDialog::OnBrowseButtonClicked);
    connect(mOpenButton, &QPushButton::clicked, this, &OpenProjectDialog::OnOpenButtonClicked);
    connect(mCancelButton, &QPushButton::clicked, this, &OpenProjectDialog::OnCancelButtonClicked);
}

QString OpenProjectDialog::GetSelectedProjectPath() const
{
    return mSelectedProjectPath;
}

void OpenProjectDialog::LoadRecentProjects()
{
    mRecentProjectsList->clear();

    // 从编辑器配置获取最近工程列表
    auto recentProjects = EditorConfig::GetInstance().GetRecentProjects();

    if (recentProjects.empty())
    {
        QListWidgetItem* item = new QListWidgetItem("没有最近打开的工程");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        mRecentProjectsList->addItem(item);
    }
    else
    {
        for (const auto& projectPath : recentProjects)
        {
            QFileInfo fileInfo(projectPath.c_str());
            QString displayName = fileInfo.fileName() + " (" + fileInfo.absolutePath() + ")";
            mRecentProjectsList->addItem(displayName);
            mRecentProjectsList->item(mRecentProjectsList->count() - 1)->setData(Qt::UserRole, QString::fromStdString(projectPath));
        }
    }
}

void OpenProjectDialog::OnBrowseButtonClicked()
{
    // 从编辑器配置获取上次打开工程时的路径作为默认路径
    QString defaultPath = EditorConfig::GetInstance().GetLastOpenProjectPath().c_str();

    // 如果没有记录，则使用用户主目录
    if (defaultPath.isEmpty())
    {
        defaultPath = QDir::homePath();
    }

    // 确保路径存在
    QDir dir(defaultPath);
    if (!dir.exists())
    {
        defaultPath = QDir::homePath();
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "打开工程",
        defaultPath,
        "GNXEngine Project Files (*.gnxproj)"
    );

    if (!filepath.isEmpty())
    {
        mSelectedProjectPath = filepath;
        mStatusLabel->setText("");
        mStatusLabel->setStyleSheet("");
        mOpenButton->setEnabled(true);
    }
}

void OpenProjectDialog::OnOpenButtonClicked()
{
    if (mSelectedProjectPath.isEmpty())
    {
        QMessageBox::warning(this, "错误", "请选择要打开的工程！");
        return;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(mSelectedProjectPath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "错误", "工程文件不存在！");
        return;
    }

    // 调用工程管理器打开工程
    bool success = GNXEngine::ProjectManager::GetInstance().OpenProject(mSelectedProjectPath.toStdString());

    if (success)
    {
        // 添加到最近工程列表
        EditorConfig::GetInstance().AddRecentProject(mSelectedProjectPath.toStdString());

        // 保存上次打开工程时的路径（使用工程文件所在目录）
        std::string lastOpenPath = fileInfo.absolutePath().toStdString();
        EditorConfig::GetInstance().SetLastOpenProjectPath(lastOpenPath);

        QMessageBox::information(this, "成功", "工程打开成功！");
        accept();
    }
    else
    {
        QMessageBox::critical(this, "错误", "工程打开失败！");
    }
}

void OpenProjectDialog::OnCancelButtonClicked()
{
    reject();
}

void OpenProjectDialog::OnProjectSelectionChanged()
{
    QListWidgetItem* currentItem = mRecentProjectsList->currentItem();
    if (currentItem && currentItem->flags() & Qt::ItemIsEnabled)
    {
        mSelectedProjectPath = currentItem->data(Qt::UserRole).toString();
        mStatusLabel->setText("");
        mStatusLabel->setStyleSheet("");
        mOpenButton->setEnabled(true);
    }
    else
    {
        mSelectedProjectPath = "";
        mOpenButton->setEnabled(false);
    }
}
