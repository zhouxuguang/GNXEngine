#include "TextureEditorDialog.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/AssetManager/include/TextureAsset.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstring>

TextureEditorDialog::TextureEditorDialog(AssetManager::TextureAsset *texture,
                                         const QString &filePath, QWidget *parent)
    : QDialog(parent), mTexture(texture), mFilePath(filePath), mUseSourceImage(false)
{
    SetupUI();
    SetupToolbar();
    UpdateWindowTitle();
    UpdateImageDisplay();
}

TextureEditorDialog::TextureEditorDialog(const QImage &image, const QString &filePath,
                                         QWidget *parent)
    : QDialog(parent), mSourceImage(image), mFilePath(filePath), mUseSourceImage(true)
{
    SetupUI();
    SetupToolbar();
    UpdateWindowTitle();
    UpdateImageDisplay();
}

TextureEditorDialog::~TextureEditorDialog()
{
}

void TextureEditorDialog::SetupUI()
{
    setWindowTitle("Texture Editor");
    resize(800, 600);
    setMinimumSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mToolBar = new QToolBar(this);
    mToolBar->setMovable(false);
    mainLayout->addWidget(mToolBar);

    mGraphicsScene = new QGraphicsScene(this);
    mGraphicsView = new QGraphicsView(mGraphicsScene, this);
    mGraphicsView->setRenderHint(QPainter::Antialiasing);
    mGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    mGraphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainLayout->addWidget(mGraphicsView, 1);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(5, 5, 5, 5);

    mInfoLabel = new QLabel(this);
    bottomLayout->addWidget(mInfoLabel);

    bottomLayout->addStretch();

    mZoomLabel = new QLabel("100%", this);
    mZoomLabel->setMinimumWidth(60);
    mZoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bottomLayout->addWidget(mZoomLabel);

    mainLayout->addLayout(bottomLayout);
}

void TextureEditorDialog::SetupToolbar()
{
    QAction *zoomInAction = new QAction("放大 (+)", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomIn);
    mToolBar->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction("缩小 (-)", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomOut);
    mToolBar->addAction(zoomOutAction);

    QAction *zoomResetAction = new QAction("重置 (R)", this);
    zoomResetAction->setShortcut(QKeySequence("R"));
    connect(zoomResetAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomReset);
    mToolBar->addAction(zoomResetAction);

    QAction *fitToScreenAction = new QAction("适应屏幕 (F)", this);
    fitToScreenAction->setShortcut(QKeySequence("F"));
    connect(fitToScreenAction, &QAction::triggered, this, &TextureEditorDialog::OnFitToScreen);
    mToolBar->addAction(fitToScreenAction);

    QAction *actualSizeAction = new QAction("实际大小 (A)", this);
    actualSizeAction->setShortcut(QKeySequence("A"));
    connect(actualSizeAction, &QAction::triggered, this, &TextureEditorDialog::OnActualSize);
    mToolBar->addAction(actualSizeAction);

    mToolBar->addSeparator();

    mToolBar->addWidget(new QLabel("Mip Level:", this));
    mMipLevelCombo = new QComboBox(this);
    mMipLevelCombo->setMinimumWidth(60);
    connect(mMipLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &TextureEditorDialog::OnMipLevelChanged);
    mToolBar->addWidget(mMipLevelCombo);

    mToolBar->addSeparator();

    mToolBar->addWidget(new QLabel("通道:", this));
    mChannelCombo = new QComboBox(this);
    mChannelCombo->addItem("RGBA");
    mChannelCombo->addItem("RGB");
    mChannelCombo->addItem("R");
    mChannelCombo->addItem("G");
    mChannelCombo->addItem("B");
    mChannelCombo->addItem("A");
    mChannelCombo->setMinimumWidth(60);
    connect(mChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &TextureEditorDialog::OnChannelChanged);
    mToolBar->addWidget(mChannelCombo);

    mToolBar->addSeparator();

    mCheckerboardButton = new QPushButton("背景: 棋盘格", this);
    mCheckerboardButton->setCheckable(true);
    mCheckerboardButton->setChecked(true);
    mCheckerboardButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #4CAF50; color: white; }");
    connect(mCheckerboardButton, &QPushButton::toggled, this,
            &TextureEditorDialog::OnToggleCheckerboard);
    mToolBar->addWidget(mCheckerboardButton);

    mToolBar->addSeparator();

    QAction *exportAction = new QAction("导出", this);
    connect(exportAction, &QAction::triggered, this, &TextureEditorDialog::OnExportAs);
    mToolBar->addAction(exportAction);

    QAction *copyAction = new QAction("复制到剪贴板", this);
    connect(copyAction, &QAction::triggered, this, &TextureEditorDialog::OnCopyToClipboard);
    mToolBar->addAction(copyAction);

    if (mUseSourceImage)
    {
        mMipLevelCombo->setEnabled(false);
        mMipLevelCombo->addItem("0 (Base)");
    }
    else if (mTexture)
    {
        int mipLevels = mTexture->GetMipLevels();
        for (int i = 0; i < mipLevels; ++i)
        {
            mMipLevelCombo->addItem(QString::number(i));
        }
    }
}

void TextureEditorDialog::UpdateImageDisplay()
{
    if (!mTexture && mSourceImage.isNull())
    {
        mGraphicsScene->clear();
        return;
    }

    mGraphicsScene->clear();

    if (mShowCheckerboard)
    {
        DrawCheckerboardBackground();
    }

    QImage image;

    if (mUseSourceImage)
    {
        image = mSourceImage;
    }
    else
    {
        image = ConvertToQImage();
    }

    if (image.isNull())
    {
        mGraphicsScene->addText("无法加载纹理数据", QFont("Arial", 12));
        return;
    }

    image = ApplyChannelMask(image, mCurrentChannel);

    QPixmap pixmap = QPixmap::fromImage(image);
    QGraphicsPixmapItem *pixmapItem = mGraphicsScene->addPixmap(pixmap);
    mGraphicsScene->setSceneRect(pixmap.rect());

    if (mUseSourceImage)
    {
        mInfoLabel->setText(QString("%1 x %2 | 原始图像").arg(image.width()).arg(image.height()));
    }
    else
    {
        uint32_t width = mTexture->GetWidth() >> mCurrentMipLevel;
        uint32_t height = mTexture->GetHeight() >> mCurrentMipLevel;
        mInfoLabel->setText(QString("%1 x %2 | Mip: %3/%4 | Format: %5")
                                .arg(width)
                                .arg(height)
                                .arg(mCurrentMipLevel)
                                .arg(mTexture->GetMipLevels() - 1)
                                .arg(static_cast<int>(mTexture->GetFormat())));
    }

    UpdateZoomLabel();
}

QImage TextureEditorDialog::ConvertToQImage()
{
    if (!mTexture || RenderCore::IsAnyCompressedTextureFormat(mTexture->GetFormat()))
    {
        return {};
    }

    const uint8_t *data = mTexture->GetData();
    const size_t dataSize = mTexture->GetDataSize();
    if (!data || dataSize < sizeof(uint32_t))
    {
        return {};
    }

    const int mipLevel =
        std::clamp(mCurrentMipLevel, 0, static_cast<int>(mTexture->GetMipLevels()) - 1);
    const uint32_t mipWidth = std::max(1u, mTexture->GetWidth() >> mipLevel);
    const uint32_t mipHeight = std::max(1u, mTexture->GetHeight() >> mipLevel);

    QImage::Format imageFormat = QImage::Format_Invalid;
    uint32_t bytesPerPixel = 0;
    bool floatingPoint = false;
    switch (mTexture->GetFormat())
    {
    case RenderCore::kTexFormatRGBA8:
    case RenderCore::kTexFormatBGRA32:
    case RenderCore::kTexFormatSRGB8_ALPHA8:
        imageFormat = QImage::Format_RGBA8888;
        bytesPerPixel = 4;
        break;
    case RenderCore::kTexFormatRGB24:
    case RenderCore::kTexFormatBGR24:
    case RenderCore::kTexFormatSRGB8:
        imageFormat = QImage::Format_RGB888;
        bytesPerPixel = 3;
        break;
    case RenderCore::kTexFormatAlpha8:
        imageFormat = QImage::Format_Grayscale8;
        bytesPerPixel = 1;
        break;
    case RenderCore::kTexFormatRGBA32Float:
        bytesPerPixel = 16;
        floatingPoint = true;
        break;
    default:
        return {};
    }

    size_t offset = mTexture->GetImageDataOffset();
    uint32_t imageSize = 0;
    for (int level = 0; level <= mipLevel; ++level)
    {
        if (offset + sizeof(imageSize) > dataSize)
        {
            return {};
        }
        std::memcpy(&imageSize, data + offset, sizeof(imageSize));
        offset += sizeof(imageSize);
        if (imageSize > dataSize - offset)
        {
            return {};
        }
        if (level != mipLevel)
        {
            const size_t imageCount =
                mTexture->GetTextureType() == RenderCore::TextureType_CUBE ? 6u : 1u;
            if (imageSize > (dataSize - offset) / imageCount)
            {
                return {};
            }
            const size_t alignedImageSize = (static_cast<size_t>(imageSize) + 3u) & ~size_t(3u);
            offset += alignedImageSize * imageCount;
            if (offset > dataSize)
            {
                return {};
            }
        }
    }

    const size_t rowBytes = static_cast<size_t>(mipWidth) * bytesPerPixel;
    const size_t requiredSize = rowBytes * mipHeight;
    if (requiredSize > imageSize || requiredSize > dataSize - offset)
    {
        return {};
    }

    if (floatingPoint)
    {
        QImage image(static_cast<int>(mipWidth), static_cast<int>(mipHeight),
                     QImage::Format_RGBA8888);
        for (uint32_t y = 0; y < mipHeight; ++y)
        {
            const uint8_t *sourceRow = data + offset + y * rowBytes;
            uint8_t *destination = image.scanLine(static_cast<int>(y));
            for (uint32_t x = 0; x < mipWidth; ++x)
            {
                float values[4];
                std::memcpy(values, sourceRow + x * sizeof(values), sizeof(values));
                for (int channel = 0; channel < 3; ++channel)
                {
                    const float linear = std::max(0.0f, values[channel]);
                    const float mapped = linear / (1.0f + linear);
                    destination[x * 4 + channel] = static_cast<uint8_t>(
                        std::clamp(std::pow(mapped, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f));
                }
                destination[x * 4 + 3] =
                    static_cast<uint8_t>(std::clamp(values[3] * 255.0f, 0.0f, 255.0f));
            }
        }
        return image;
    }

    QImage image(data + offset, static_cast<int>(mipWidth), static_cast<int>(mipHeight),
                 static_cast<qsizetype>(rowBytes), imageFormat);
    if (mTexture->GetFormat() == RenderCore::kTexFormatBGRA32 ||
        mTexture->GetFormat() == RenderCore::kTexFormatBGR24)
    {
        image = image.rgbSwapped();
    }

    return image.copy();
}

QImage TextureEditorDialog::ApplyChannelMask(const QImage &image, int channel)
{
    if (image.isNull() || channel == 0)
    {
        return image;
    }

    QImage result = image.convertToFormat(QImage::Format_RGBA8888);

    for (int y = 0; y < result.height(); ++y)
    {
        uint8_t *scanline = result.scanLine(y);
        for (int x = 0; x < result.width(); ++x)
        {
            uint8_t *pixel = scanline + x * 4;
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];
            uint8_t a = pixel[3];

            switch (channel)
            {
            case 1:
                pixel[3] = 255;
                break;
            case 2:
                pixel[0] = r;
                pixel[1] = r;
                pixel[2] = r;
                pixel[3] = 255;
                break;
            case 3:
                pixel[0] = g;
                pixel[1] = g;
                pixel[2] = g;
                pixel[3] = 255;
                break;
            case 4:
                pixel[0] = b;
                pixel[1] = b;
                pixel[2] = b;
                pixel[3] = 255;
                break;
            case 5:
                pixel[0] = a;
                pixel[1] = a;
                pixel[2] = a;
                pixel[3] = 255;
                break;
            }
        }
    }

    return result;
}

void TextureEditorDialog::DrawCheckerboardBackground()
{
    const int checkerSize = 16;
    int width, height;

    if (mUseSourceImage)
    {
        width = mSourceImage.width();
        height = mSourceImage.height();
    }
    else
    {
        width = static_cast<int>(mTexture->GetWidth() >> mCurrentMipLevel);
        height = static_cast<int>(mTexture->GetHeight() >> mCurrentMipLevel);
    }

    QImage checkerboard(width, height, QImage::Format_RGBA8888);
    checkerboard.fill(Qt::transparent);

    QPainter painter(&checkerboard);
    for (int y = 0; y < height; y += checkerSize)
    {
        for (int x = 0; x < width; x += checkerSize)
        {
            bool light = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            painter.fillRect(x, y, checkerSize, checkerSize,
                             light ? QColor(240, 240, 240) : QColor(180, 180, 180));
        }
    }
    painter.end();

    mGraphicsScene->addPixmap(QPixmap::fromImage(checkerboard));
}

void TextureEditorDialog::OnZoomIn()
{
    const float newZoom = std::min(mZoomFactor * 1.25f, 32.0f);
    mGraphicsView->scale(newZoom / mZoomFactor, newZoom / mZoomFactor);
    mZoomFactor = newZoom;
    UpdateZoomLabel();
}

void TextureEditorDialog::OnZoomOut()
{
    const float newZoom = std::max(mZoomFactor / 1.25f, 0.05f);
    mGraphicsView->scale(newZoom / mZoomFactor, newZoom / mZoomFactor);
    mZoomFactor = newZoom;
    UpdateZoomLabel();
}

void TextureEditorDialog::OnZoomReset()
{
    mGraphicsView->resetTransform();
    mZoomFactor = 1.0f;
    UpdateZoomLabel();
}

void TextureEditorDialog::OnFitToScreen()
{
    if (mGraphicsScene->items().isEmpty())
    {
        return;
    }

    mGraphicsView->fitInView(mGraphicsScene->sceneRect(), Qt::KeepAspectRatio);
    mZoomFactor = mGraphicsView->transform().m11();
    UpdateZoomLabel();
}

void TextureEditorDialog::OnActualSize()
{
    OnZoomReset();
}

void TextureEditorDialog::OnMipLevelChanged(int level)
{
    mCurrentMipLevel = level;
    UpdateImageDisplay();
}

void TextureEditorDialog::OnChannelChanged(int index)
{
    mCurrentChannel = index;
    UpdateImageDisplay();
}

void TextureEditorDialog::OnToggleCheckerboard()
{
    mShowCheckerboard = mCheckerboardButton->isChecked();
    mCheckerboardButton->setText(mShowCheckerboard ? "背景: 棋盘格" : "背景: 纯白");
    UpdateImageDisplay();
}

void TextureEditorDialog::OnExportAs()
{
    if (!mTexture && mSourceImage.isNull())
    {
        return;
    }

    const QString defaultFileName = QFileInfo(mFilePath).baseName();
    QFileDialog dialog(this, "导出纹理", defaultFileName,
                       "PNG图像 (*.png);;JPEG图像 (*.jpg);;BMP图像 (*.bmp)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
    {
        return;
    }
    const QString filePath = dialog.selectedFiles().constFirst();

    if (filePath.isEmpty())
    {
        return;
    }

    QImage image;

    if (mUseSourceImage)
    {
        image = mSourceImage;
    }
    else
    {
        image = ConvertToQImage();
    }

    if (image.isNull())
    {
        QMessageBox::warning(this, "导出失败", "无法生成图像数据");
        return;
    }

    image = ApplyChannelMask(image, mCurrentChannel);

    if (!image.save(filePath))
    {
        QMessageBox::warning(this, "导出失败", "无法保存图像文件");
    }
}

void TextureEditorDialog::OnCopyToClipboard()
{
    if (!mTexture && mSourceImage.isNull())
    {
        return;
    }

    QImage image;

    if (mUseSourceImage)
    {
        image = mSourceImage;
    }
    else
    {
        image = ConvertToQImage();
    }

    if (image.isNull())
    {
        return;
    }

    image = ApplyChannelMask(image, mCurrentChannel);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(image);
}

void TextureEditorDialog::UpdateZoomLabel()
{
    int zoomPercent = static_cast<int>(mZoomFactor * 100);
    mZoomLabel->setText(QString("%1%").arg(zoomPercent));
}

void TextureEditorDialog::UpdateWindowTitle()
{
    if (!mFilePath.isEmpty())
    {
        QFileInfo fileInfo(mFilePath);
        setWindowTitle(QString("Texture Editor - %1").arg(fileInfo.fileName()));
    }
    else
    {
        setWindowTitle("Texture Editor");
    }
}

void TextureEditorDialog::wheelEvent(QWheelEvent *event)
{
    const bool preciseZoom = event->modifiers() & Qt::ControlModifier;
    const float step = preciseZoom ? 1.1f : 1.25f;
    float newZoom = mZoomFactor;
    if (event->angleDelta().y() > 0)
    {
        newZoom = std::min(mZoomFactor * step, 32.0f);
    }
    else if (event->angleDelta().y() < 0)
    {
        newZoom = std::max(mZoomFactor / step, 0.05f);
    }

    mGraphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    if (newZoom != mZoomFactor)
    {
        mGraphicsView->scale(newZoom / mZoomFactor, newZoom / mZoomFactor);
        mZoomFactor = newZoom;
    }
    UpdateZoomLabel();
    event->accept();
}
