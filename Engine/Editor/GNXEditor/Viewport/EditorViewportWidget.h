#pragma once
#include <QWidget>
#include <memory>
class EditorInputAdapter;
class EditorRenderHost;
class EditorViewportWidget final : public QWidget
{
    Q_OBJECT
  public:
    explicit EditorViewportWidget(EditorRenderHost &host, QWidget *parent = nullptr);
    ~EditorViewportWidget() override;

  signals:
    void RenderError(const QString &message);

  protected:
    bool event(QEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void leaveEvent(QEvent *) override;
    void hideEvent(QHideEvent *) override;

  private:
    void AttachIfReady();
    EditorRenderHost &mHost;
    std::unique_ptr<EditorInputAdapter> mInput;
    void *mNativeHandle = nullptr;
    bool mAttachInProgress = false;
};
