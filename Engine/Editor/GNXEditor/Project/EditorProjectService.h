#pragma once
#include "Project/EditorProjectContext.h"
#include <QObject>
#include <QString>
struct ProjectCreateRequest
{
    QString parentDirectory;
    QString projectName;
};
class EditorProjectService final : public QObject
{
    Q_OBJECT
  public:
    explicit EditorProjectService(QString engineContentRoot = {}, QObject *parent = nullptr);
    bool CreateProject(const ProjectCreateRequest &request);
    bool OpenProject(const QString &projectFile);
    bool SaveProject();
    void CloseProject();
    bool HasOpenProject() const;
    QString ProjectName() const;
    QString ProjectRoot() const;
    QString AssetRoot() const;
    const EditorProjectContext &Context() const
    {
        return mContext;
    }
  signals:
    void ProjectAboutToClose(const QString &sessionId);
    void ProjectOpened(const QString &projectRoot);
    void ProjectClosed();
    void ProjectOperationFailed(const QString &message);

  private:
    void RefreshContext();
    QString mEngineContentRoot;
    EditorProjectContext mContext;
};
