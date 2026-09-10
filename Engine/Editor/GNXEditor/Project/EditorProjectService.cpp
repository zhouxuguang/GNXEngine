#include "EditorProjectService.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <utility>
EditorProjectService::EditorProjectService(QString engineContentRoot, QObject *parent)
    : QObject(parent), mEngineContentRoot(std::move(engineContentRoot))
{
    if (mEngineContentRoot.isEmpty())
        mEngineContentRoot = QString::fromStdString(GetProjectAssetDir());
    mEngineContentRoot = QDir::cleanPath(mEngineContentRoot);
}
bool EditorProjectService::CreateProject(const ProjectCreateRequest &request)
{
    static const QRegularExpression invalidCharacters(QStringLiteral(R"([<>:"/\\|?*])"));
    const QString name = request.projectName.trimmed();
    const QString upperName = name.toUpper();
    const QString deviceName = upperName.section('.', 0, 0);
    static const QSet<QString> reservedNames = {
        "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
        "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    if (name.isEmpty() || name == "." || name == ".." || name.size() > 255 || name.endsWith('.') ||
        name.endsWith(' ') || name.contains(invalidCharacters) ||
        reservedNames.contains(deviceName) || !QDir(request.parentDirectory).exists())
    {
        emit ProjectOperationFailed(tr("工程名称或保存路径无效"));
        return false;
    }
    const QString root = QDir(request.parentDirectory).filePath(name);
    const bool hadOpenProject = HasOpenProject();
    if (hadOpenProject)
        emit ProjectAboutToClose(mContext.sessionId);
    if (!GNXEngine::ProjectManager::GetInstance().CreateNewProject(root.toStdString(),
                                                                   name.toStdString()))
    {
        emit ProjectOperationFailed(tr("工程创建失败：%1").arg(root));
        if (hadOpenProject && HasOpenProject())
        {
            RefreshContext();
            emit ProjectOpened(ProjectRoot());
        }
        return false;
    }
    RefreshContext();
    emit ProjectOpened(ProjectRoot());
    return true;
}
bool EditorProjectService::OpenProject(const QString &file)
{
    GNXEngine::ProjectConfig candidate;
    if (!candidate.LoadFromFile(file.toStdString()))
    {
        emit ProjectOperationFailed(tr("工程文件无效：%1").arg(file));
        return false;
    }
    if (HasOpenProject())
        emit ProjectAboutToClose(mContext.sessionId);
    if (!GNXEngine::ProjectManager::GetInstance().OpenProject(file.toStdString()))
    {
        emit ProjectOperationFailed(tr("工程打开失败：%1").arg(file));
        if (HasOpenProject())
        {
            RefreshContext();
            emit ProjectOpened(ProjectRoot());
        }
        return false;
    }
    RefreshContext();
    emit ProjectOpened(ProjectRoot());
    return true;
}
bool EditorProjectService::SaveProject()
{
    if (!HasOpenProject() || !GNXEngine::ProjectManager::GetInstance().SaveProject())
    {
        emit ProjectOperationFailed(tr("工程保存失败"));
        return false;
    }
    return true;
}
void EditorProjectService::CloseProject()
{
    if (!HasOpenProject())
        return;
    emit ProjectAboutToClose(mContext.sessionId);
    if (!GNXEngine::ProjectManager::GetInstance().CloseProject())
    {
        emit ProjectOperationFailed(tr("工程关闭失败"));
        return;
    }
    mContext = {};
    emit ProjectClosed();
}
bool EditorProjectService::HasOpenProject() const
{
    return GNXEngine::ProjectManager::GetInstance().IsProjectOpen();
}
QString EditorProjectService::ProjectName() const
{
    auto *p = GNXEngine::ProjectManager::GetInstance().GetProject();
    return p ? QString::fromStdString(p->projectName) : QString();
}
QString EditorProjectService::ProjectRoot() const
{
    auto *p = GNXEngine::ProjectManager::GetInstance().GetProject();
    return p ? QString::fromStdString(p->GetProjectDirectory()) : QString();
}
QString EditorProjectService::AssetRoot() const
{
    auto *p = GNXEngine::ProjectManager::GetInstance().GetProject();
    return p ? QString::fromStdString(p->GetAbsoluteAssetPath("")) : QString();
}

void EditorProjectService::RefreshContext()
{
    auto *project = GNXEngine::ProjectManager::GetInstance().GetProject();
    if (!project)
    {
        mContext = {};
        return;
    }
    mContext.engineContentRoot = mEngineContentRoot;
    mContext.projectFile = QString::fromStdString(project->projectFilePath);
    mContext.projectRoot = QString::fromStdString(project->projectPath);
    mContext.assetRoot = QString::fromStdString(project->assetsPath);
    mContext.cacheRoot = QString::fromStdString(project->cachePath);
    mContext.settingsRoot = QString::fromStdString(project->settingsPath);
    mContext.scenesRoot = QString::fromStdString(project->scenesPath);
    mContext.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}
