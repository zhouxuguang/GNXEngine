//
//  InputGLFW.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#include "Input.h"
#include "InputState.h"
#include "RenderWindow.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

NAMESPACE_GNXENGINE_BEGIN

bool Input::IsKeyPressed(const KeyCode key)
{
    // 从 InputState 获取输入状态，而不是直接轮询 GLFW
    // InputState 会根据当前模式自动选择合适的数据源
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    // 如果输入模式是 Poll 或 Auto，尝试从 GLFW 轮询
    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        // 在 Poll 模式下，尝试从 GLFW 窗口轮询并更新状态
        void* nativeWindow = GetRenderWindow()->GetNativeWindow();
        if (nativeWindow)
        {
            inputState.PollFromGLFW(nativeWindow);
        }
    }

    // 从 InputState 返回状态
    return inputState.IsKeyPressed(key);
}

bool Input::IsMouseButtonPressed(const MouseCode button)
{
    // 从 InputState 获取输入状态
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    // 如果输入模式是 Poll 或 Auto，尝试从 GLFW 轮询
    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        void* nativeWindow = GetRenderWindow()->GetNativeWindow();
        if (nativeWindow)
        {
            inputState.PollFromGLFW(nativeWindow);
        }
    }

    // 从 InputState 返回状态
    return inputState.IsMouseButtonPressed(button);
}

mathutil::Vector2f Input::GetMousePosition()
{
    // 从 InputState 获取输入状态
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    // 如果输入模式是 Poll 或 Auto，尝试从 GLFW 轮询
    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        void* nativeWindow = GetRenderWindow()->GetNativeWindow();
        if (nativeWindow)
        {
            inputState.PollFromGLFW(nativeWindow);
        }
    }

    // 从 InputState 返回状态
    return inputState.GetMousePosition();
}

float Input::GetMouseX()
{
    return GetMousePosition().x;
}

float Input::GetMouseY()
{
    return GetMousePosition().y;
}

NAMESPACE_GNXENGINE_END
