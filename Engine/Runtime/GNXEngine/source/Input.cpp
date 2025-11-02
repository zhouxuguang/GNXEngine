//
//  Input.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#include "Input.h"

NAMESPACE_GNXENGINE_BEGIN

bool Input::IsKeyPressed(const KeyCode key)
{
    return false;
}

bool Input::IsMouseButtonPressed(const MouseCode button)
{
    return false;
}

mathutil::Vector2f Input::GetMousePosition()
{
    return mathutil::Vector2f();
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
