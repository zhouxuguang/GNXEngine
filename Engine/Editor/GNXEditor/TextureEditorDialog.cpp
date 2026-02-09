//
//  TextureEditorDialog.cpp
//  GNXEngine
//
//  纹理查看器对话框实现
//

#include "TextureEditorDialog.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/AssetManager/include/TextureAsset.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QFileInfo>
#include <QWheelEvent>
#include <QScrollBar>

TextureEditorDialog::TextureEditorDialog(AssetManager::TextureAsset* texture,
                                         const QString& filePath,
                                         QWidget* parent)
    : QDialog(parent)
    , mTexture(texture)
    , mFilePath(filePath)
    , mUseSourceImage(false)
{
    SetupUI();
    SetupToolbar();
    UpdateWindowTitle();
    UpdateImageDisplay();
}

TextureEditorDialog::TextureEditorDialog(const QImage& image,
                                         const QString& filePath,
                                         QWidget* parent)
    : QDialog(parent)
    , mSourceImage(image)
    , mFilePath(filePath)
    , mUseSourceImage(true)
{
    SetupUI();
    SetupToolbar();
    UpdateWindowTitle();
    UpdateImageDisplay();
}

TextureEditorDialog::~TextureEditorDialog()
{
    // QGraphicsScene会自动管理其中的QGraphicsPixmapItem
}

void TextureEditorDialog::SetupUI()
{
    // 设置窗口属性
    setWindowTitle("Texture Editor");
    resize(800, 600);
    setMinimumSize(400, 300);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建工具栏
    mToolBar = new QToolBar(this);
    mToolBar->setMovable(false);
    mainLayout->addWidget(mToolBar);

    // 创建图形视图和场景
    mGraphicsScene = new QGraphicsScene(this);
    mGraphicsView = new QGraphicsView(mGraphicsScene, this);
    mGraphicsView->setRenderHint(QPainter::Antialiasing);
    mGraphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    mGraphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainLayout->addWidget(mGraphicsView, 1);

    // 创建底部状态栏布局
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(5, 5, 5, 5);

    // 信息标签
    mInfoLabel = new QLabel(this);
    bottomLayout->addWidget(mInfoLabel);

    bottomLayout->addStretch();

    // 缩放信息标签
    mZoomLabel = new QLabel("100%", this);
    mZoomLabel->setMinimumWidth(60);
    mZoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bottomLayout->addWidget(mZoomLabel);

    mainLayout->addLayout(bottomLayout);
}

void TextureEditorDialog::SetupToolbar()
{
    // 缩放按钮
    QAction* zoomInAction = new QAction("放大 (+)", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomIn);
    mToolBar->addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("缩小 (-)", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomOut);
    mToolBar->addAction(zoomOutAction);

    QAction* zoomResetAction = new QAction("重置 (R)", this);
    zoomResetAction->setShortcut(QKeySequence("R"));
    connect(zoomResetAction, &QAction::triggered, this, &TextureEditorDialog::OnZoomReset);
    mToolBar->addAction(zoomResetAction);

    QAction* fitToScreenAction = new QAction("适应屏幕 (F)", this);
    fitToScreenAction->setShortcut(QKeySequence("F"));
    connect(fitToScreenAction, &QAction::triggered, this, &TextureEditorDialog::OnFitToScreen);
    mToolBar->addAction(fitToScreenAction);

    QAction* actualSizeAction = new QAction("实际大小 (A)", this);
    actualSizeAction->setShortcut(QKeySequence("A"));
    connect(actualSizeAction, &QAction::triggered, this, &TextureEditorDialog::OnActualSize);
    mToolBar->addAction(actualSizeAction);

    mToolBar->addSeparator();

    // Mipmap级别选择
    mToolBar->addWidget(new QLabel("Mip Level:", this));
    mMipLevelCombo = new QComboBox(this);
    mMipLevelCombo->setMinimumWidth(60);
    connect(mMipLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TextureEditorDialog::OnMipLevelChanged);
    mToolBar->addWidget(mMipLevelCombo);

    mToolBar->addSeparator();

    // 通道选择
    mToolBar->addWidget(new QLabel("通道:", this));
    mChannelCombo = new QComboBox(this);
    mChannelCombo->addItem("RGBA");
    mChannelCombo->addItem("RGB");
    mChannelCombo->addItem("R");
    mChannelCombo->addItem("G");
    mChannelCombo->addItem("B");
    mChannelCombo->addItem("A");
    mChannelCombo->setMinimumWidth(60);
    connect(mChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TextureEditorDialog::OnChannelChanged);
    mToolBar->addWidget(mChannelCombo);

    mToolBar->addSeparator();

    // 棋盘格背景切换按钮
    mCheckerboardButton = new QPushButton("背景: 棋盘格", this);
    mCheckerboardButton->setCheckable(true);
    mCheckerboardButton->setChecked(true);
    mCheckerboardButton->setStyleSheet(
        "QPushButton { padding: 4px 8px; }"
        "QPushButton:checked { background-color: #4CAF50; color: white; }"
    );
    connect(mCheckerboardButton, &QPushButton::toggled, this, &TextureEditorDialog::OnToggleCheckerboard);
    mToolBar->addWidget(mCheckerboardButton);

    mToolBar->addSeparator();

    // 导出和复制按钮
    QAction* exportAction = new QAction("导出", this);
    connect(exportAction, &QAction::triggered, this, &TextureEditorDialog::OnExportAs);
    mToolBar->addAction(exportAction);

    QAction* copyAction = new QAction("复制到剪贴板", this);
    connect(copyAction, &QAction::triggered, this, &TextureEditorDialog::OnCopyToClipboard);
    mToolBar->addAction(copyAction);

    // 如果是源图像模式，禁用Mipmap选择
    if (mUseSourceImage)
    {
        mMipLevelCombo->setEnabled(false);
        mMipLevelCombo->addItem("0 (Base)");
    }
    else if (mTexture)
    {
        // 填充Mipmap级别选项
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

    // 清除场景中的所有项目
    mGraphicsScene->clear();

    // 绘制棋盘格背景
    if (mShowCheckerboard)
    {
        DrawCheckerboardBackground();
    }

    QImage image;

    if (mUseSourceImage)
    {
        // 使用源图像
        image = mSourceImage;
    }
    else
    {
        // 从TextureAsset转换
        image = ConvertToQImage();
    }

    if (image.isNull())
    {
        mGraphicsScene->addText("无法加载纹理数据", QFont("Arial", 12));
        return;
    }

    // 应用通道遮罩
    image = ApplyChannelMask(image, mCurrentChannel);

    // 创建QPixmap并添加到场景
    QPixmap pixmap = QPixmap::fromImage(image);
    QGraphicsPixmapItem* pixmapItem = mGraphicsScene->addPixmap(pixmap);
    mGraphicsScene->setSceneRect(pixmap.rect());

    // 更新信息标签
    if (mUseSourceImage)
    {
        mInfoLabel->setText(QString("%1 x %2 | 原始图像")
            .arg(image.width()).arg(image.height()));
    }
    else
    {
        uint32_t width = mTexture->GetWidth() >> mCurrentMipLevel;
        uint32_t height = mTexture->GetHeight() >> mCurrentMipLevel;
        mInfoLabel->setText(QString("%1 x %2 | Mip: %3/%4 | Format: %5")
            .arg(width).arg(height)
            .arg(mCurrentMipLevel).arg(mTexture->GetMipLevels() - 1)
            .arg(static_cast<int>(mTexture->GetFormat())));
    }

    // 更新缩放
    UpdateZoomLabel();
}

QImage TextureEditorDialog::ConvertToQImage()
{
    if (!mTexture)
    {
        return QImage();
    }

    // 获取纹理数据
    const uint8_t* data = mTexture->GetData();
    if (!data)
    {
        return QImage();
    }

    // 计算当前Mipmap级别的尺寸
    uint32_t mipWidth = mTexture->GetWidth() >> mCurrentMipLevel;
    uint32_t mipHeight = mTexture->GetHeight() >> mCurrentMipLevel;

    if (mipWidth == 0 || mipHeight == 0)
    {
        return QImage();
    }

    // 获取纹理格式并转换为QImage::Format
    RenderCore::TextureFormat textureFormat = mTexture->GetFormat();
    QImage::Format qimageFormat;

    // 根据纹理格式确定QImage格式
    switch (textureFormat)
    {
        case RenderCore::kTexFormatRGBA32:
        case RenderCore::kTexFormatBGRA32:
        case RenderCore::kTexFormatSRGB8_ALPHA8:
            qimageFormat = QImage::Format_RGBA8888;
            break;
        case RenderCore::kTexFormatRGB24:
        case RenderCore::kTexFormatBGR24:
        case RenderCore::kTexFormatSRGB8:
            qimageFormat = QImage::Format_RGB888;
            break;
        case RenderCore::kTexFormatAlpha8:
            qimageFormat = QImage::Format_Grayscale8;
            break;
        default:
            // 对于不支持的格式，尝试按RGBA32处理
            qimageFormat = QImage::Format_RGBA8888;
            break;
    }

    // 计算当前Mipmap级别的数据偏移
    size_t dataOffset = 0;
    for (int i = 0; i < mCurrentMipLevel; ++i)
    {
        uint32_t levelWidth = mTexture->GetWidth() >> i;
        uint32_t levelHeight = mTexture->GetHeight() >> i;
        uint32_t bytesPerPixel = mTexture->GetBytesPerPixel();
        dataOffset += levelWidth * levelHeight * bytesPerPixel;
    }

    // 创建QImage（注意：QImage不会复制数据，所以需要确保数据生命周期）
    QImage image(data + dataOffset, mipWidth, mipHeight, qimageFormat);

    // 如果格式需要转换（如BGR到RGB）
    if (textureFormat == RenderCore::kTexFormatBGRA32 ||
        textureFormat == RenderCore::kTexFormatBGR24)
    {
        image = image.rgbSwapped();
    }

    return image.copy();  // 创建副本以确保数据安全
}

QImage TextureEditorDialog::ApplyChannelMask(const QImage& image, int channel)
{
    if (image.isNull() || channel == 0)  // 0 = RGBA, 显示原始图像
    {
        return image;
    }

    QImage result = image.convertToFormat(QImage::Format_RGBA8888);

    // 遍历所有像素应用通道遮罩
    for (int y = 0; y < result.height(); ++y)
    {
        uint8_t* scanline = result.scanLine(y);
        for (int x = 0; x < result.width(); ++x)
        {
            uint8_t* pixel = scanline + x * 4;
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];
            uint8_t a = pixel[3];

            switch (channel)
            {
                case 1:  // RGB - Alpha设为255
                    pixel[3] = 255;
                    break;
                case 2:  // R - 只显示红色通道
                    pixel[0] = r;
                    pixel[1] = r;
                    pixel[2] = r;
                    pixel[3] = 255;
                    break;
                case 3:  // G - 只显示绿色通道
                    pixel[0] = g;
                    pixel[1] = g;
                    pixel[2] = g;
                    pixel[3] = 255;
                    break;
                case 4:  // B - 只显示蓝色通道
                    pixel[0] = b;
                    pixel[1] = b;
                    pixel[2] = b;
                    pixel[3] = 255;
                    break;
                case 5:  // A - 只显示Alpha通道
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
    mZoomFactor *= 1.25f;
    mGraphicsView->scale(1.25f, 1.25f);
    UpdateZoomLabel();
}

void TextureEditorDialog::OnZoomOut()
{
    mZoomFactor /= 1.25f;
    mGraphicsView->scale(1.0f / 1.25f, 1.0f / 1.25f);
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
    if (!mTexture || mGraphicsScene->items().isEmpty())
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

    QString defaultFileName = QFileInfo(mFilePath).baseName();
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出纹理",
        defaultFileName,
        "PNG图像 (*.png);;JPEG图像 (*.jpg);;BMP图像 (*.bmp)"
    );

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

    QClipboard* clipboard = QApplication::clipboard();
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

void TextureEditorDialog::wheelEvent(QWheelEvent* event)
{
    // 检查是否按住Ctrl键（用于精确缩放）
    bool preciseZoom = (event->modifiers() & Qt::ControlModifier);

    // 将变换锚点设置为视图中心（防止缩放时平移）
    mGraphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    mGraphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // 完全禁用滚动条（防止任何滚动）
    mGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 保存当前的滚动条位置
    int hScrollPos = mGraphicsView->horizontalScrollBar()->value();
    int vScrollPos = mGraphicsView->verticalScrollBar()->value();

    // 滚轮向上滚动（放大）或向下滚动（缩小）
    if (event->angleDelta().y() > 0)
    {
        // 向上滚动，放大
        if (preciseZoom)
        {
            // 精确缩放：每次放大10%
            mGraphicsView->scale(1.1f, 1.1f);
            mZoomFactor *= 1.1f;
        }
        else
        {
            // 普通缩放：每次放大25%
            mGraphicsView->scale(1.25f, 1.25f);
            mZoomFactor *= 1.25f;
        }
    }
    else if (event->angleDelta().y() < 0)
    {
        // 向下滚动，缩小
        if (preciseZoom)
        {
            // 精确缩放：每次缩小10%
            mGraphicsView->scale(1.0f / 1.1f, 1.0f / 1.1f);
            mZoomFactor /= 1.1f;
        }
        else
        {
            // 普通缩放：每次缩小25%
            mGraphicsView->scale(1.0f / 1.25f, 1.0f / 1.25f);
            mZoomFactor /= 1.25f;
        }
    }

    // 恢复滚动条策略
    mGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 更新缩放标签
    UpdateZoomLabel();

    // 接受事件，完全阻止默认的滚动行为
    event->accept();
}
