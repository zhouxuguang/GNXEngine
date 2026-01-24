//
//  InputState.h
//  GNXEngine
//
//  输入状态管理器 - 缓存输入状态，避免轮询开销
//

#ifndef GNXENGINE_INPUT_STATE_INCLUDE_SDFJSDJH
#define GNXENGINE_INPUT_STATE_INCLUDE_SDFJSDJH

#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/GNXEngine/include/KeyCodes.h"
#include <unordered_map>
#include <array>
#include <mutex>
#include <cstddef>

NAMESPACE_GNXENGINE_BEGIN

// 键码索引类型，用于数组索引
using KeyCodeIndex = size_t;

// 输入模式
enum class InputMode
{
    Poll,   // 轮询模式（如 GLFW）
    Event,  // 事件驱动模式（如 Qt）
    Auto     // 自动检测模式
};

// 输入状态管理器
// 缓存所有输入状态，避免每次都从系统轮询
class GNXENGINE_API InputState
{
public:
    static InputState& GetInstance();

    // ========== 输入模式控制 ==========

    // 设置输入模式
    void SetMode(InputMode mode);

    // 获取当前输入模式
    InputMode GetMode() const { return mMode; }

    // ========== 状态更新接口（由事件系统调用）==========

    // 设置按键状态
    void SetKeyState(KeyCode key, bool pressed);

    // 设置鼠标按钮状态
    void SetMouseButtonState(MouseCode button, bool pressed);

    // 设置鼠标位置
    void SetMousePosition(float x, float y);

    // 更新鼠标滚轮
    void UpdateMouseScroll(float xOffset, float yOffset);

    // ========== 状态查询接口（由 Input 类调用）==========

    // 检查按键是否按下
    bool IsKeyPressed(KeyCode key) const;

    // 检查鼠标按钮是否按下
    bool IsMouseButtonPressed(MouseCode button) const;

    // 获取鼠标位置
    mathutil::Vector2f GetMousePosition() const;

    // 获取鼠标 X 坐标
    float GetMouseX() const { return mMousePosition.x; }

    // 获取鼠标 Y 坐标
    float GetMouseY() const { return mMousePosition.y; }

    // ========== 轮询接口（用于 GLFW 模式）==========

    // 从 GLFW 窗口轮询并更新状态
    void PollFromGLFW(void* glfwWindow);

    // 清空状态（用于测试或重置）
    void Clear();

private:
    InputState();
    ~InputState();

    // 禁止拷贝和移动
    InputState(const InputState&) = delete;
    InputState& operator=(const InputState&) = delete;

    // 内部辅助函数
    KeyCodeIndex GetKeyCodeIndex(KeyCode key) const;

private:
    // 输入模式
    InputMode mMode = InputMode::Auto;

    // 键盘状态（使用数组存储，快速访问）
    // 假设 KeyCode 最大值为 512
    std::array<bool, 512> mKeyStates = {false};

    // 鼠标按钮状态
    std::array<bool, 8> mMouseButtonStates = {false};

    // 鼠标位置
    mathutil::Vector2f mMousePosition = {0.0f, 0.0f};

    // 鼠标滚轮偏移
    mathutil::Vector2f mMouseScrollDelta = {0.0f, 0.0f};

    // 线程安全锁
    mutable std::mutex mMutex;
};

NAMESPACE_GNXENGINE_END

#endif /* GNXENGINE_INPUT_STATE_INCLUDE_SDFJSDJH */
