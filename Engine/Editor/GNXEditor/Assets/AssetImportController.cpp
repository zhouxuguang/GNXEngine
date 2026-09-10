#include "AssetImportController.h"
#include "Application/EditorSettings.h"
#include "AssetImportService.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QWidget>

AssetImportController::AssetImportController(AssetImportService &service, EditorSettings &settings,
                                             QObject *parent)
    : QObject(parent), mService(service), mSettings(settings)
{
}

void AssetImportController::ImportInteractive(QWidget *parent, const QString &destinationDirectory)
{
    if (destinationDirectory.isEmpty())
        return;
    const QString initial = mSettings.LastImportDirectory().isEmpty()
                                ? QDir::homePath()
                                : mSettings.LastImportDirectory();
    const QString filters = tr("模型文件 (*.obj *.fbx *.gltf *.glb *.3ds);;图像文件 (*.png *.jpg "
                               "*.jpeg *.bmp *.tga *.hdr *.exr *.webp)");
    QFileDialog dialog(parent, tr("导入资产"), initial, filters);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilters(filters.split(";;"));
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return;
    const QString source = dialog.selectedFiles().constFirst();
    mSettings.SetLastImportDirectory(QFileInfo(source).absolutePath());
    mService.EnqueueFile(source, destinationDirectory);
}
