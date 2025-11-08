//
//  InputGLFW.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#include "Input.h"
#include "RenderWindow.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

NAMESPACE_GNXENGINE_BEGIN

bool Input::IsKeyPressed(const KeyCode key)
{
    GLFWwindow* window = static_cast<GLFWwindow*>(GetRenderWindow()->GetNativeWindow());
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(const MouseCode button)
{
    GLFWwindow* window = static_cast<GLFWwindow*>(GetRenderWindow()->GetNativeWindow());
    int state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
}

mathutil::Vector2f Input::GetMousePosition()
{
    GLFWwindow* window = static_cast<GLFWwindow*>(GetRenderWindow()->GetNativeWindow());
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return {(float)xpos, (float)ypos};
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
