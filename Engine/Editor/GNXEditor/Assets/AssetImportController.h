#pragma once

#include <QObject>
class AssetImportService;
class EditorSettings;
class QWidget;

class AssetImportController final : public QObject
{
    Q_OBJECT
  public:
    AssetImportController(AssetImportService &service, EditorSettings &settings,
                          QObject *parent = nullptr);
    void ImportInteractive(QWidget *parent, const QString &destinationDirectory);

  private:
    AssetImportService &mService;
    EditorSettings &mSettings;
};
