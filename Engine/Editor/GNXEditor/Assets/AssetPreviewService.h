#pragma once

#include <QImage>
#include <QString>

class AssetPreviewService final
{
  public:
    static QImage LoadSourceImage(const QString &filePath, QString *errorMessage = nullptr);
};
