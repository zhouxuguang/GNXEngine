#pragma once

#include <QDialog>

class EditorSettings;
class QLabel;
class QListWidget;
class QPushButton;

class OpenProjectDialog final : public QDialog
{
    Q_OBJECT

  public:
    explicit OpenProjectDialog(EditorSettings &settings, QWidget *parent = nullptr);
    ~OpenProjectDialog() override = default;

    QString GetSelectedProjectPath() const;

  private slots:
    void OnBrowseButtonClicked();
    void OnOpenButtonClicked();
    void OnCancelButtonClicked();
    void OnProjectSelectionChanged();

  private:
    void SetupUI();
    void LoadRecentProjects();

    QListWidget *mRecentProjectsList = nullptr;
    QPushButton *mBrowseButton = nullptr;
    QPushButton *mOpenButton = nullptr;
    QPushButton *mCancelButton = nullptr;
    QLabel *mStatusLabel = nullptr;
    QString mSelectedProjectPath;
    EditorSettings &mSettings;
};
