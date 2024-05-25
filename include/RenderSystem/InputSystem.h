//
//  InputSystem.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/4/20.
//

#ifndef INPUT_SYSTEM_INCLUDE_H
#define INPUT_SYSTEM_INCLUDE_H

#include "RSDefine.h"

NS_RENDERSYSTEM_BEGIN

struct MousePoint
{
    MousePoint(float x = 0.0, float y = 0.0)
    {
        this->x = x;
        this->y = y;
    }
    
    float x;
    float y;
};

class InputSystem
{
public:
    
    static InputSystem* GetInstance();
    
    bool leftMouseDown = false;
    
    MousePoint mouseDelta;
    MousePoint mouseScroll;
    
protected:
    InputSystem();
};

NS_RENDERSYSTEM_END

#endif /* INPUT_SYSTEM_INCLUDE_H */
