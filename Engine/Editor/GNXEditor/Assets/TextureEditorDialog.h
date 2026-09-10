#pragma once

#include <QDialog>
#include <QImage>

class QComboBox;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QPushButton;
class QToolBar;
class QWheelEvent;

namespace AssetManager
{
class TextureAsset;
}

class TextureEditorDialog final : public QDialog
{
    Q_OBJECT
  public:
    explicit TextureEditorDialog(AssetManager::TextureAsset *texture, const QString &filePath,
                                 QWidget *parent = nullptr);
    explicit TextureEditorDialog(const QImage &image, const QString &filePath,
                                 QWidget *parent = nullptr);
    ~TextureEditorDialog() override;

  private slots:
    void OnZoomIn();
    void OnZoomOut();
    void OnZoomReset();
    void OnFitToScreen();
    void OnActualSize();
    void OnMipLevelChanged(int level);
    void OnChannelChanged(int index);
    void OnToggleCheckerboard();
    void OnExportAs();
    void OnCopyToClipboard();

  protected:
    void wheelEvent(QWheelEvent *event) override;

  private:
    void SetupUI();
    void SetupToolbar();
    void UpdateImageDisplay();
    QImage ConvertToQImage();
    QImage ApplyChannelMask(const QImage &image, int channel);
    void DrawCheckerboardBackground();
    void UpdateZoomLabel();
    void UpdateWindowTitle();

    AssetManager::TextureAsset *mTexture = nullptr;
    QImage mSourceImage;
    QString mFilePath;
    int mCurrentMipLevel = 0;
    int mCurrentChannel = 0;
    bool mShowCheckerboard = true;
    bool mUseSourceImage = false;
    float mZoomFactor = 1.0f;
    QGraphicsView *mGraphicsView = nullptr;
    QGraphicsScene *mGraphicsScene = nullptr;
    QToolBar *mToolBar = nullptr;
    QComboBox *mMipLevelCombo = nullptr;
    QComboBox *mChannelCombo = nullptr;
    QPushButton *mCheckerboardButton = nullptr;
    QLabel *mZoomLabel = nullptr;
    QLabel *mInfoLabel = nullptr;
};
