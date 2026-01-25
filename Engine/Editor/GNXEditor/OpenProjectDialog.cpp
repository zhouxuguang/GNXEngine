//
//  OpenProjectDialog.cpp
//  GNXEngine
//

#include "OpenProjectDialog.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>

OpenProjectDialog::OpenProjectDialog(QWidget* parent)
    : QDialog(parent)
{
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

    // 从工程管理器获取最近工程列表
    auto recentProjects = GNXEngine::ProjectManager::GetInstance().GetRecentProjects();

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
    // 获取当前工程的目录作为默认路径
    QString defaultPath = QDir::homePath();
    GNXEngine::ProjectConfig* project = GNXEngine::ProjectManager::GetInstance().GetProject();
    if (project)
    {
        defaultPath = QString::fromStdString(project->projectPath);
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
