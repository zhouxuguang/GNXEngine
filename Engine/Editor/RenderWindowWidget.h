#pragma once

#include <QWidget>
#include <QTimer>
#include <memory>

#include "Runtime/GNXEngine/include/RenderWindow.h"
#include "Runtime/GNXEngine/include/Events/Event.h"

// Qt 包装器，用于将 RenderWindow 嵌入到 Qt 界面中
class RenderWindowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RenderWindowWidget(QWidget* parent = nullptr);
    ~RenderWindowWidget();

    // 获取内部的 RenderWindow 指针
    GNXEngine::RenderWindowPtr GetRenderWindow() const
    {
        return mRenderWindow;
    }

protected:
    // Qt 事件处理函数，将 Qt 事件转发到 RenderWindow
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void onRenderTick();

private:
    // 初始化 RenderWindow
    void InitializeRenderWindow();

    // 将 Qt 按钮码映射到 GNXEngine 的 MouseCode
    int QtButtonToGNXButton(Qt::MouseButton button) const;

    // 将 Qt 键码映射到 GNXEngine 的 KeyCode
    int QtKeyToGNXKey(int key) const;

    // 将事件转发到 RenderWindow 的事件回调
    void ForwardEventToRenderWindow(GNXEngine::Event& event);

    GNXEngine::RenderWindowPtr mRenderWindow;
    QTimer* mRenderTimer;
    bool mRenderWindowInitialized = false;
};
