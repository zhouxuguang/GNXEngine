#include "EditorProjectService.h"
#include "Runtime/GNXEngine/include/ProjectConfig.h"
#include <QDir>
EditorProjectService::EditorProjectService(QObject* parent) : QObject(parent) {}
bool EditorProjectService::CreateProject(const ProjectCreateRequest& request) {
  const QString root = QDir(request.parentDirectory).filePath(request.projectName);
  if (!GNXEngine::ProjectManager::GetInstance().CreateNewProject(root.toStdString(), request.projectName.toStdString())) {
    emit ProjectOperationFailed(tr("工程创建失败：%1").arg(root)); return false;
  }
  emit ProjectOpened(ProjectRoot()); return true;
}
bool EditorProjectService::OpenProject(const QString& file) {
  if (!GNXEngine::ProjectManager::GetInstance().OpenProject(file.toStdString())) {
    emit ProjectOperationFailed(tr("工程打开失败：%1").arg(file)); return false;
  }
  emit ProjectOpened(ProjectRoot()); return true;
}
bool EditorProjectService::SaveProject() {
  if (!HasOpenProject() || !GNXEngine::ProjectManager::GetInstance().SaveProject()) {
    emit ProjectOperationFailed(tr("工程保存失败")); return false;
  }
  return true;
}
void EditorProjectService::CloseProject() {
  if (!HasOpenProject()) return;
  if (!GNXEngine::ProjectManager::GetInstance().CloseProject()) { emit ProjectOperationFailed(tr("工程关闭失败")); return; }
  emit ProjectClosed();
}
bool EditorProjectService::HasOpenProject() const { return GNXEngine::ProjectManager::GetInstance().IsProjectOpen(); }
QString EditorProjectService::ProjectName() const { auto* p=GNXEngine::ProjectManager::GetInstance().GetProject(); return p?QString::fromStdString(p->projectName):QString(); }
QString EditorProjectService::ProjectRoot() const { auto* p=GNXEngine::ProjectManager::GetInstance().GetProject(); return p?QString::fromStdString(p->GetProjectDirectory()):QString(); }
QString EditorProjectService::AssetRoot() const { auto* p=GNXEngine::ProjectManager::GetInstance().GetProject(); return p?QString::fromStdString(p->GetAbsoluteAssetPath("")):QString(); }
