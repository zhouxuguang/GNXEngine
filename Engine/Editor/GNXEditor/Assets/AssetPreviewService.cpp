#include "AssetPreviewService.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include <algorithm>
#include <cmath>

namespace
{
uchar ToDisplay(float value)
{
    const float positive = std::max(value, 0.0f);
    const float mapped = positive / (1.0f + positive);
    return static_cast<uchar>(std::clamp(std::pow(mapped, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f));
}

QImage FloatImage(const imagecodec::VImage &source, int channels)
{
    QImage result(static_cast<int>(source.GetWidth()), static_cast<int>(source.GetHeight()),
                  QImage::Format_RGBA8888);
    const float *pixels = reinterpret_cast<const float *>(source.GetImageData());
    for (int y = 0; y < result.height(); ++y)
    {
        auto *target = result.scanLine(y);
        for (int x = 0; x < result.width(); ++x)
        {
            const float *pixel = pixels + (y * result.width() + x) * channels;
            target[x * 4] = ToDisplay(pixel[0]);
            target[x * 4 + 1] = ToDisplay(pixel[std::min(1, channels - 1)]);
            target[x * 4 + 2] = ToDisplay(pixel[std::min(2, channels - 1)]);
            target[x * 4 + 3] = channels == 4 ? ToDisplay(pixel[3]) : 255;
        }
    }
    return result;
}
} // namespace

QImage AssetPreviewService::LoadSourceImage(const QString &filePath, QString *errorMessage)
{
    imagecodec::VImage image;
    if (!imagecodec::ImageDecoder::DecodeFile(filePath.toStdString().c_str(), &image))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法解码图像文件");
        return {};
    }

    QImage result;
    switch (image.GetFormat())
    {
    case imagecodec::FORMAT_RGBA8:
    case imagecodec::FORMAT_SRGB8_ALPHA8:
        result = QImage(image.GetImageData(), image.GetWidth(), image.GetHeight(),
                        image.GetBytesPerRow(), QImage::Format_RGBA8888);
        break;
    case imagecodec::FORMAT_RGB8:
    case imagecodec::FORMAT_SRGB8:
        result = QImage(image.GetImageData(), image.GetWidth(), image.GetHeight(),
                        image.GetBytesPerRow(), QImage::Format_RGB888);
        break;
    case imagecodec::FORMAT_GRAY8:
        result = QImage(image.GetImageData(), image.GetWidth(), image.GetHeight(),
                        image.GetBytesPerRow(), QImage::Format_Grayscale8);
        break;
    case imagecodec::FORMAT_RGBA32Float:
        return FloatImage(image, 4);
    case imagecodec::FORMAT_RGB32Float:
        return FloatImage(image, 3);
    default:
        if (errorMessage)
            *errorMessage = QStringLiteral("暂不支持该图像像素格式的预览");
        return {};
    }
    return result.copy();
}
