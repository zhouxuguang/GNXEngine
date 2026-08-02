//
//  InputSDL.cpp
//  GNXEngine
//
//  SDL2 输入后端（Android / iOS）
//

#if GNX_WINDOW_SDL

#include "Input.h"
#include "InputState.h"
#include <SDL.h>

NAMESPACE_GNXENGINE_BEGIN

bool Input::IsKeyPressed(const KeyCode key)
{
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        inputState.PollFromSDL();
    }

    return inputState.IsKeyPressed(key);
}

bool Input::IsMouseButtonPressed(const MouseCode button)
{
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        inputState.PollFromSDL();
    }

    return inputState.IsMouseButtonPressed(button);
}

mathutil::Vector2f Input::GetMousePosition()
{
    GNXEngine::InputState& inputState = GNXEngine::InputState::GetInstance();

    GNXEngine::InputMode mode = inputState.GetMode();
    if (mode == GNXEngine::InputMode::Poll || mode == GNXEngine::InputMode::Auto)
    {
        inputState.PollFromSDL();
    }

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

#endif // GNX_WINDOW_SDL
