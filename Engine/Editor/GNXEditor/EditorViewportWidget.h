#pragma once
#include <QWidget>
#include <memory>
class EditorInputAdapter;
class EditorRenderHost;
class EditorViewportWidget final : public QWidget {
  Q_OBJECT
public:
  explicit EditorViewportWidget(EditorRenderHost &host,
                                QWidget *parent = nullptr);
  ~EditorViewportWidget() override;

protected:
  bool event(QEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void keyReleaseEvent(QKeyEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void focusOutEvent(QFocusEvent *) override;

private:
  void AttachIfReady();
  EditorRenderHost &mHost;
  std::unique_ptr<EditorInputAdapter> mInput;
};
