//
//  TextureEditorDialog.h
//  GNXEngine
//
//  纹理查看器对话框 - 简洁版
//  提供纹理预览、缩放、Mipmap查看、通道查看等功能
//

#pragma once

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QToolBar>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <memory>

// 前向声明
namespace AssetManager {
    class TextureAsset;
}

/**
 * 纹理查看器对话框类
 */
class TextureEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * 构造函数 - 从TextureAsset加载
     * @param texture 纹理资源指针
     * @param filePath 纹理文件路径（用于显示标题）
     * @param parent 父窗口
     */
    explicit TextureEditorDialog(AssetManager::TextureAsset* texture,
                                 const QString& filePath,
                                 QWidget* parent = nullptr);

    /**
     * 构造函数 - 从QImage加载（用于原始图像文件）
     * @param image 图像对象
     * @param filePath 图像文件路径（用于显示标题）
     * @param parent 父窗口
     */
    explicit TextureEditorDialog(const QImage& image,
                                 const QString& filePath,
                                 QWidget* parent = nullptr);

    ~TextureEditorDialog();

private slots:
    // 缩放相关槽函数
    void OnZoomIn();
    void OnZoomOut();
    void OnZoomReset();
    void OnFitToScreen();
    void OnActualSize();

    // Mipmap级别改变
    void OnMipLevelChanged(int level);

    // 通道切换
    void OnChannelChanged(int index);

    // 切换棋盘格背景
    void OnToggleCheckerboard();

    // 导出功能
    void OnExportAs();
    void OnCopyToClipboard();

protected:
    /**
     * 处理鼠标滚轮事件（用于缩放）
     */
    void wheelEvent(QWheelEvent* event) override;

private:
    /**
     * 设置UI布局
     */
    void SetupUI();

    /**
     * 设置工具栏
     */
    void SetupToolbar();

    /**
     * 更新图像显示
     */
    void UpdateImageDisplay();

    /**
     * 将纹理数据转换为QImage
     * @return QImage对象
     */
    QImage ConvertToQImage();

    /**
     * 应用通道遮罩到QImage
     * @param image 原始图像
     * @param channel 通道类型（0=RGBA, 1=RGB, 2=R, 3=G, 4=B, 5=A）
     * @return 应用通道后的QImage
     */
    QImage ApplyChannelMask(const QImage& image, int channel);

    /**
     * 绘制棋盘格背景
     * @param scene 图形场景
     */
    void DrawCheckerboardBackground();

    /**
     * 更新缩放信息标签
     */
    void UpdateZoomLabel();

    /**
     * 更新标题栏显示
     */
    void UpdateWindowTitle();

private:
    // 数据成员
    AssetManager::TextureAsset* mTexture = nullptr;
    QImage mSourceImage;  // 原始图像数据（用于直接加载图像文件）
    QString mFilePath;
    int mCurrentMipLevel = 0;
    int mCurrentChannel = 0;  // 0=RGBA, 1=RGB, 2=R, 3=G, 4=B, 5=A
    bool mShowCheckerboard = true;
    bool mUseSourceImage = false;  // 是否使用源图像模式（非TextureAsset）
    float mZoomFactor = 1.0f;

    // UI组件
    QGraphicsView* mGraphicsView = nullptr;
    QGraphicsScene* mGraphicsScene = nullptr;
    QToolBar* mToolBar = nullptr;
    QComboBox* mMipLevelCombo = nullptr;
    QComboBox* mChannelCombo = nullptr;
    QPushButton* mCheckerboardButton = nullptr;
    QLabel* mZoomLabel = nullptr;
    QLabel* mInfoLabel = nullptr;
};
