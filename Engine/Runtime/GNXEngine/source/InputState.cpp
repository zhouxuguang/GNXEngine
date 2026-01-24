//
//  InputState.cpp
//  GNXEngine
//

#include "InputState.h"
#include "RenderWindow.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "KeyCodes.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

NAMESPACE_GNXENGINE_BEGIN

// 单例实例
static InputState* g_Instance = nullptr;

InputState& InputState::GetInstance()
{
    if (!g_Instance)
    {
        g_Instance = new InputState();
    }
    return *g_Instance;
}

InputState::InputState()
{
    mKeyStates.fill(false);
    mMouseButtonStates.fill(false);
}

InputState::~InputState()
{
    if (g_Instance == this)
    {
        g_Instance = nullptr;
    }
}

void InputState::SetMode(InputMode mode)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mMode = mode;
}

void InputState::SetKeyState(KeyCode key, bool pressed)
{
    KeyCodeIndex index = GetKeyCodeIndex(key);
    if (index < mKeyStates.size())
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mKeyStates[index] = pressed;
    }
}

void InputState::SetMouseButtonState(MouseCode button, bool pressed)
{
    if (button < mMouseButtonStates.size())
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mMouseButtonStates[button] = pressed;
    }
}

void InputState::SetMousePosition(float x, float y)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mMousePosition.x = x;
    mMousePosition.y = y;
}

void InputState::UpdateMouseScroll(float xOffset, float yOffset)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mMouseScrollDelta.x = xOffset;
    mMouseScrollDelta.y = yOffset;
}

bool InputState::IsKeyPressed(KeyCode key) const
{
    KeyCodeIndex index = GetKeyCodeIndex(key);
    if (index < mKeyStates.size())
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mKeyStates[index];
    }
    return false;
}

bool InputState::IsMouseButtonPressed(MouseCode button) const
{
    if (button < mMouseButtonStates.size())
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mMouseButtonStates[button];
    }
    return false;
}

mathutil::Vector2f InputState::GetMousePosition() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mMousePosition;
}

void InputState::PollFromGLFW(void* glfwWindow)
{
    if (!glfwWindow)
    {
        return;
    }

    GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
    std::lock_guard<std::mutex> lock(mMutex);

    // 轮询所有可能键的状态
    // 这里只轮询常用键，可以根据需要扩展
    for (KeyCodeIndex i = 0; i < mKeyStates.size(); ++i)
    {
        int state = glfwGetKey(window, static_cast<int>(i));
        mKeyStates[i] = (state == GLFW_PRESS);
    }

    // 轮询鼠标按钮状态
    for (size_t i = 0; i < mMouseButtonStates.size(); ++i)
    {
        int state = glfwGetMouseButton(window, static_cast<int>(i));
        mMouseButtonStates[i] = (state == GLFW_PRESS);
    }

    // 轮询鼠标位置
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mMousePosition.x = static_cast<float>(xpos);
    mMousePosition.y = static_cast<float>(ypos);
}

void InputState::Clear()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mKeyStates.fill(false);
    mMouseButtonStates.fill(false);
    mMousePosition = {0.0f, 0.0f};
    mMouseScrollDelta = {0.0f, 0.0f};
}

KeyCodeIndex InputState::GetKeyCodeIndex(KeyCode key) const
{
    return static_cast<KeyCodeIndex>(key);
}

NAMESPACE_GNXENGINE_END
