#include "RenderWindowWidget.h"

#include "Runtime/GNXEngine/include/Input.h"
#include "Runtime/GNXEngine/include/InputState.h"
#include "Runtime/GNXEngine/include/Events/Event.h"
#include "Runtime/GNXEngine/include/Events/ApplicationEvent.h"
#include "Runtime/GNXEngine/include/Events/MouseEvent.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/GNXEngine/include/KeyCodes.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/RenderSystem/include/SceneManager.h"

#include <QWindow>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MAC)
#include "macUtils.h"
#endif

RenderWindowWidget::RenderWindowWidget(QWidget* parent)
    : QWidget(parent)
    , mRenderTimer(new QTimer(this))
{
    // 设置窗口属性，允许嵌入
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_PaintOnScreen, true);

    // 启用鼠标追踪，确保鼠标移动事件能被捕获
    setMouseTracking(true);

    // 设置焦点策略
    setFocusPolicy(Qt::StrongFocus);

    // 连接渲染定时器（60 FPS）
    connect(mRenderTimer, &QTimer::timeout, this, &RenderWindowWidget::onRenderTick);
    mRenderTimer->start(16); // ~60 FPS
}

RenderWindowWidget::~RenderWindowWidget()
{
    if (mRenderTimer)
    {
        mRenderTimer->stop();
    }
}

void RenderWindowWidget::InitializeRenderWindow()
{
    if (mRenderWindowInitialized)
    {
        return;
    }

    // 获取原生窗口句柄，需要根据平台进行转换
    void* nativeWindowHandle = nullptr;

#if defined(Q_OS_WIN)
    // Windows: winId() 返回 HWND，直接转换为 void*
    nativeWindowHandle = reinterpret_cast<void*>(winId());

#elif defined(Q_OS_MAC)
    // macOS: winId() 返回 WId（uintptr_t），需要转换为 NSView* 指针
    // 然后获取其 layer（即 CAMetalLayer）
    // WId 在 macOS 上是 unsigned long long，需要通过 reinterpret_cast 转换
    WId wid = winId();
    nativeWindowHandle = GetMetalLayer(wid);
#else
    // 其他平台：暂时不支持
    return;
#endif

    // 创建 WindowProps
    GNXEngine::WindowProps props;
    props.width = width();
    props.height = height();
    props.title = "GNXEngine Render Window";

    // 调用 RenderWindow::CreateWithExternalWindow，传入外部窗口句柄
    mRenderWindow = GNXEngine::RenderWindow::CreateWithExternalWindow(props, nativeWindowHandle);

    mRenderWindowInitialized = true;
}

void RenderWindowWidget::resizeEvent(QResizeEvent* event)
{
    if (mRenderWindow)
    {
        uint32_t newWidth = event->size().width();
        uint32_t newHeight = event->size().height();
        mRenderWindow->Resize(newWidth, newHeight);

        // 发送窗口调整大小事件
        GNXEngine::WindowResizeEvent resizeEvent(newWidth, newHeight);
        ForwardEventToRenderWindow(resizeEvent);
    }

    QWidget::resizeEvent(event);
}

void RenderWindowWidget::mousePressEvent(QMouseEvent* event)
{
    int button = QtButtonToGNXButton(event->button());
    if (button >= 0)
    {
        GNXEngine::MouseButtonPressedEvent mouseEvent(button);
        ForwardEventToRenderWindow(mouseEvent);
    }
    QWidget::mousePressEvent(event);
}

void RenderWindowWidget::mouseReleaseEvent(QMouseEvent* event)
{
    int button = QtButtonToGNXButton(event->button());
    if (button >= 0)
    {
        GNXEngine::MouseButtonReleasedEvent mouseEvent(button);
        ForwardEventToRenderWindow(mouseEvent);
    }
    QWidget::mouseReleaseEvent(event);
}

void RenderWindowWidget::mouseMoveEvent(QMouseEvent* event)
{
    float x = static_cast<float>(event->position().x());
    float y = static_cast<float>(event->position().y());
    GNXEngine::MouseMovedEvent mouseEvent(x, y);
    ForwardEventToRenderWindow(mouseEvent);
    QWidget::mouseMoveEvent(event);
}

void RenderWindowWidget::wheelEvent(QWheelEvent* event)
{
    // Qt 的滚轮角度需要转换为像素值
    // 通常滚轮滚动一次是 120 像素
    float deltaX = static_cast<float>(event->angleDelta().x());
    float deltaY = static_cast<float>(event->angleDelta().y());

    // 标准化滚轮值
    if (deltaY != 0)
    {
        deltaY = deltaY > 0 ? 120.0f : -120.0f;
    }

    GNXEngine::MouseScrolledEvent scrollEvent(deltaX, deltaY);
    ForwardEventToRenderWindow(scrollEvent);

    QWidget::wheelEvent(event);
}

void RenderWindowWidget::keyPressEvent(QKeyEvent* event)
{
    int key = QtKeyToGNXKey(event->key());
    if (key >= 0)
    {
        GNXEngine::KeyPressedEvent keyEvent(key, event->isAutoRepeat());
        ForwardEventToRenderWindow(keyEvent);
    }
    QWidget::keyPressEvent(event);
}

void RenderWindowWidget::keyReleaseEvent(QKeyEvent* event)
{
    int key = QtKeyToGNXKey(event->key());
    if (key >= 0)
    {
        GNXEngine::KeyReleasedEvent keyEvent(key);
        ForwardEventToRenderWindow(keyEvent);
    }
    QWidget::keyReleaseEvent(event);
}

void RenderWindowWidget::focusInEvent(QFocusEvent* event)
{
    // 获得焦点时，确保 RenderWindow 能够接收输入
    QWidget::focusInEvent(event);
}

void RenderWindowWidget::focusOutEvent(QFocusEvent* event)
{
    // 失去焦点时的处理
    QWidget::focusOutEvent(event);
}

bool RenderWindowWidget::event(QEvent* event)
{
    // 处理原生窗口事件
    if (event->type() == QEvent::WinIdChange && !mRenderWindowInitialized)
    {
        InitializeRenderWindow();
    }

    return QWidget::event(event);
}

void RenderWindowWidget::onRenderTick()
{
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    LOG_INFO("deltaTime = %f\n", deltaTime);

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    
    if (mRenderWindow)
    {
        mRenderWindow->OnUpdate();
        sceneManager->Render(nullptr);
        
        mathutil::Vector2f mousePos = GNXEngine::Input::GetMousePosition();
        LOG_INFO("mousePos x = %f, y = %f", mousePos.x, mousePos.y);
    }
}

int RenderWindowWidget::QtButtonToGNXButton(Qt::MouseButton button) const
{
    switch (button)
    {
        case Qt::LeftButton:
            return 0; // MouseButton::Left
        case Qt::RightButton:
            return 1; // MouseButton::Right
        case Qt::MiddleButton:
            return 2; // MouseButton::Middle
        default:
            return -1;
    }
}

int RenderWindowWidget::QtKeyToGNXKey(int key) const
{
    // 将 Qt 键码映射到 GNXEngine 的 KeyCode
    // 这里需要根据 KeyCodes.h 中的定义进行映射
    switch (key)
    {
        case Qt::Key_Space: return 32;
        case Qt::Key_Apostrophe: return 39;
        case Qt::Key_Comma: return 44;
        case Qt::Key_Minus: return 45;
        case Qt::Key_Period: return 46;
        case Qt::Key_Slash: return 47;
        case Qt::Key_0: return 48;
        case Qt::Key_1: return 49;
        case Qt::Key_2: return 50;
        case Qt::Key_3: return 51;
        case Qt::Key_4: return 52;
        case Qt::Key_5: return 53;
        case Qt::Key_6: return 54;
        case Qt::Key_7: return 55;
        case Qt::Key_8: return 56;
        case Qt::Key_9: return 57;
        case Qt::Key_Semicolon: return 59;
        case Qt::Key_Equal: return 61;
        case Qt::Key_A: return 65;
        case Qt::Key_B: return 66;
        case Qt::Key_C: return 67;
        case Qt::Key_D: return 68;
        case Qt::Key_E: return 69;
        case Qt::Key_F: return 70;
        case Qt::Key_G: return 71;
        case Qt::Key_H: return 72;
        case Qt::Key_I: return 73;
        case Qt::Key_J: return 74;
        case Qt::Key_K: return 75;
        case Qt::Key_L: return 76;
        case Qt::Key_M: return 77;
        case Qt::Key_N: return 78;
        case Qt::Key_O: return 79;
        case Qt::Key_P: return 80;
        case Qt::Key_Q: return 81;
        case Qt::Key_R: return 82;
        case Qt::Key_S: return 83;
        case Qt::Key_T: return 84;
        case Qt::Key_U: return 85;
        case Qt::Key_V: return 86;
        case Qt::Key_W: return 87;
        case Qt::Key_X: return 88;
        case Qt::Key_Y: return 89;
        case Qt::Key_Z: return 90;
        case Qt::Key_Escape: return 256;
        case Qt::Key_Enter: return 257;
        case Qt::Key_Tab: return 258;
        case Qt::Key_Backspace: return 259;
        case Qt::Key_Insert: return 260;
        case Qt::Key_Delete: return 261;
        case Qt::Key_Right: return 262;
        case Qt::Key_Left: return 263;
        case Qt::Key_Down: return 264;
        case Qt::Key_Up: return 265;
        case Qt::Key_PageUp: return 266;
        case Qt::Key_PageDown: return 267;
        case Qt::Key_Home: return 268;
        case Qt::Key_End: return 269;
        case Qt::Key_CapsLock: return 280;
        case Qt::Key_ScrollLock: return 281;
        case Qt::Key_NumLock: return 282;
        case Qt::Key_Print: return 283;
        case Qt::Key_Pause: return 284;
        case Qt::Key_F1: return 290;
        case Qt::Key_F2: return 291;
        case Qt::Key_F3: return 292;
        case Qt::Key_F4: return 293;
        case Qt::Key_F5: return 294;
        case Qt::Key_F6: return 295;
        case Qt::Key_F7: return 296;
        case Qt::Key_F8: return 297;
        case Qt::Key_F9: return 298;
        case Qt::Key_F10: return 299;
        case Qt::Key_F11: return 300;
        case Qt::Key_F12: return 301;
        case Qt::Key_Shift: return 340;
        case Qt::Key_Control: return 341;
        case Qt::Key_Alt: return 342;
        case Qt::Key_Meta: return 343;
        default:
            return -1;
    }
}

void RenderWindowWidget::ForwardEventToRenderWindow(GNXEngine::Event& event)
{
    if (mRenderWindow)
    {
        // 直接调用 RenderWindow 基类的 TriggerEventCallback 方法
        // 这样不依赖于具体的 DefaultRenderWindow 实现，未来可以支持 SDL 等其他窗口系统
        mRenderWindow->TriggerEventCallback(event);
    }

    // 同时更新 InputState，用于输入查询
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    // 根据事件类型更新输入状态
    GNXEngine::EventDispatcher dispatcher(event);

    // 键盘按下
    dispatcher.Dispatch<GNXEngine::KeyPressedEvent>(
        [&inputState](GNXEngine::KeyPressedEvent& e)
        {
            inputState.SetKeyState(e.GetKeyCode(), true);
            return false;
        }
    );

    // 键盘释放
    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>(
        [&inputState](GNXEngine::KeyReleasedEvent& e)
        {
            inputState.SetKeyState(e.GetKeyCode(), false);
            return false;
        }
    );

    // 鼠标按钮按下
    dispatcher.Dispatch<GNXEngine::MouseButtonPressedEvent>(
        [&inputState](GNXEngine::MouseButtonPressedEvent& e)
        {
            inputState.SetMouseButtonState(e.GetMouseButton(), true);
            return false;
        }
    );

    // 鼠标按钮释放
    dispatcher.Dispatch<GNXEngine::MouseButtonReleasedEvent>(
        [&inputState](GNXEngine::MouseButtonReleasedEvent& e)
        {
            inputState.SetMouseButtonState(e.GetMouseButton(), false);
            return false;
        }
    );

    // 鼠标移动
    dispatcher.Dispatch<GNXEngine::MouseMovedEvent>(
        [&inputState](GNXEngine::MouseMovedEvent& e)
        {
            inputState.SetMousePosition(e.GetX(), e.GetY());
            return false;
        }
    );

    // 鼠标滚轮
    dispatcher.Dispatch<GNXEngine::MouseScrolledEvent>(
        [&inputState](GNXEngine::MouseScrolledEvent& e)
        {
            inputState.UpdateMouseScroll(e.GetXOffset(), e.GetYOffset());
            return false;
        }
    );
}
