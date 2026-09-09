#include "EditorSettings.h"
#include <QFileInfo>
#include <QSettings>
EditorSettings::EditorSettings()
    : mSettings(std::make_unique<QSettings>(QSettings::IniFormat,
                                            QSettings::UserScope, "GNXEngine",
                                            "GNXEditor")) {}
EditorSettings::EditorSettings(const QString &settingsFile)
    : mSettings(
          std::make_unique<QSettings>(settingsFile, QSettings::IniFormat)) {}
EditorSettings::~EditorSettings() = default;
void EditorSettings::Load() {
  constexpr int currentVersion = 1;
  if (mSettings->value("settings/version", currentVersion).toInt() >
      currentVersion)
    return;
  mRecentProjects = mSettings->value("projects/recent").toStringList();
  mLastProject = mSettings->value("projects/last").toString();
  mLastProjectDirectory = mSettings->value("projects/lastDirectory").toString();
  mLastImportDirectory =
      mSettings->value("assets/lastImportDirectory").toString();
  mWindowGeometry = mSettings->value("window/geometry").toByteArray();
  mWindowState = mSettings->value("window/state").toByteArray();
}
void EditorSettings::Save() {
  mSettings->setValue("settings/version", 1);
  mSettings->setValue("projects/recent", mRecentProjects);
  mSettings->setValue("projects/last", mLastProject);
  mSettings->setValue("projects/lastDirectory", mLastProjectDirectory);
  mSettings->setValue("assets/lastImportDirectory", mLastImportDirectory);
  mSettings->setValue("window/geometry", mWindowGeometry);
  mSettings->setValue("window/state", mWindowState);
  mSettings->sync();
}
void EditorSettings::AddRecentProject(const QString &path) {
  if (path.isEmpty())
    return;
  mRecentProjects.removeAll(path);
  mRecentProjects.prepend(path);
  while (mRecentProjects.size() > 10)
    mRecentProjects.removeLast();
  mLastProject = path;
  mLastProjectDirectory = QFileInfo(path).absolutePath();
}
