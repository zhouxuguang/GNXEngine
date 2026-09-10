#include "OpenProjectDialog.h"
#include "Application/EditorSettings.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

OpenProjectDialog::OpenProjectDialog(EditorSettings &settings, QWidget *parent)
    : QDialog(parent), mSettings(settings)
{
    SetupUI();

    setWindowTitle("打开工程");
    setModal(true);
    resize(600, 400);

    LoadRecentProjects();
}

void OpenProjectDialog::SetupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *recentLabel = new QLabel("最近打开的工程:");
    mainLayout->addWidget(recentLabel);

    mRecentProjectsList = new QListWidget();
    mainLayout->addWidget(mRecentProjectsList);

    mStatusLabel = new QLabel("");
    mStatusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(mStatusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    mBrowseButton = new QPushButton("浏览...");
    buttonLayout->addWidget(mBrowseButton);

    buttonLayout->addStretch();

    mOpenButton = new QPushButton("打开");
    mOpenButton->setEnabled(false);

    mCancelButton = new QPushButton("取消");

    buttonLayout->addWidget(mOpenButton);
    buttonLayout->addWidget(mCancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(mRecentProjectsList, &QListWidget::itemSelectionChanged, this,
            &OpenProjectDialog::OnProjectSelectionChanged);
    connect(mRecentProjectsList, &QListWidget::itemDoubleClicked, this,
            &OpenProjectDialog::OnOpenButtonClicked);
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

    auto recentProjects = mSettings.RecentProjects();

    if (recentProjects.empty())
    {
        QListWidgetItem *item = new QListWidgetItem("没有最近打开的工程");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        mRecentProjectsList->addItem(item);
    }
    else
    {
        for (const auto &projectPath : recentProjects)
        {
            QFileInfo fileInfo(projectPath);
            QString displayName = fileInfo.fileName() + " (" + fileInfo.absolutePath() + ")";
            mRecentProjectsList->addItem(displayName);
            mRecentProjectsList->item(mRecentProjectsList->count() - 1)
                ->setData(Qt::UserRole, projectPath);
        }
    }
}

void OpenProjectDialog::OnBrowseButtonClicked()
{
    QString defaultPath = mSettings.LastProjectDirectory();

    if (defaultPath.isEmpty())
    {
        defaultPath = QDir::homePath();
    }

    QDir dir(defaultPath);
    if (!dir.exists())
    {
        defaultPath = QDir::homePath();
    }

    QFileDialog fileDialog(this, tr("打开工程"), defaultPath,
                           tr("GNXEngine Project Files (*.gnxproj)"));
    fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter(tr("GNXEngine Project Files (*.gnxproj)"));

    QString filepath;
    if (fileDialog.exec() == QDialog::Accepted && !fileDialog.selectedFiles().isEmpty())
    {
        filepath = fileDialog.selectedFiles().constFirst();
    }

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

    QFileInfo fileInfo(mSelectedProjectPath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "错误", "工程文件不存在！");
        return;
    }

    accept();
}

void OpenProjectDialog::OnCancelButtonClicked()
{
    reject();
}

void OpenProjectDialog::OnProjectSelectionChanged()
{
    QListWidgetItem *currentItem = mRecentProjectsList->currentItem();
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
