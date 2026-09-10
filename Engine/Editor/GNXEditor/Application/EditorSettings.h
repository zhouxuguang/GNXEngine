#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <memory>
class QSettings;
class EditorSettings
{
  public:
    EditorSettings();
    explicit EditorSettings(const QString &settingsFile);
    ~EditorSettings();
    void Load();
    void Save();
    void AddRecentProject(const QString &path);
    QStringList RecentProjects() const
    {
        return mRecentProjects;
    }
    QString LastProject() const
    {
        return mLastProject;
    }
    QString LastProjectDirectory() const
    {
        return mLastProjectDirectory;
    }
    QString LastImportDirectory() const
    {
        return mLastImportDirectory;
    }
    QByteArray WindowGeometry() const
    {
        return mWindowGeometry;
    }
    QByteArray WindowState() const
    {
        return mWindowState;
    }
    void SetLastImportDirectory(const QString &value)
    {
        mLastImportDirectory = value;
    }
    void SetWindowGeometry(const QByteArray &value)
    {
        mWindowGeometry = value;
    }
    void SetWindowState(const QByteArray &value)
    {
        mWindowState = value;
    }

  private:
    std::unique_ptr<QSettings> mSettings;
    QStringList mRecentProjects;
    QString mLastProject, mLastProjectDirectory, mLastImportDirectory;
    QByteArray mWindowGeometry, mWindowState;
};
