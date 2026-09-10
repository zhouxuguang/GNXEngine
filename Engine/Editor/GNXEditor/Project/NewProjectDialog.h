#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class NewProjectDialog final : public QDialog
{
    Q_OBJECT

  public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    ~NewProjectDialog() override = default;

    QString GetProjectName() const;
    QString GetProjectPath() const;

  private slots:
    void OnBrowseButtonClicked();
    void OnCreateButtonClicked();
    void OnCancelButtonClicked();
    void ValidateInput();

  private:
    void SetupUI();

    QLineEdit *mProjectNameEdit = nullptr;
    QLineEdit *mProjectPathEdit = nullptr;
    QPushButton *mBrowseButton = nullptr;
    QPushButton *mCreateButton = nullptr;
    QPushButton *mCancelButton = nullptr;
    QLabel *mStatusLabel = nullptr;
};
