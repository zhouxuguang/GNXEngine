//
//  Header.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/11/2.
//

#ifndef GNXENGINE_INPUT_USADAJ_INCLUDE_FJSKJ
#define GNXENGINE_INPUT_USADAJ_INCLUDE_FJSKJ

#include "KeyCodes.h"
#include "Runtime/MathUtil/include/Vector2.h"

NAMESPACE_GNXENGINE_BEGIN

// 输入系统，处理系统的键盘和鼠标消息
class Input
{
public:
    static bool IsKeyPressed(KeyCode key);
    static bool IsMouseButtonPressed(MouseCode button);
    static mathutil::Vector2f GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};

NAMESPACE_GNXENGINE_END

#endif /* GNXENGINE_INPUT_USADAJ_INCLUDE_FJSKJ */
