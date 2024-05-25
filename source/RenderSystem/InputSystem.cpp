//
//  InputSystem.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/4/20.
//

#include "InputSystem.h"

NS_RENDERSYSTEM_BEGIN

extern InputSystem *createInputSystem_APPLE();

InputSystem::InputSystem()
{
}

InputSystem* InputSystem::GetInstance()
{
    static InputSystem* instance = nullptr;
    if (nullptr == instance)
    {
        instance = createInputSystem_APPLE();
    }
    return instance;
}

NS_RENDERSYSTEM_END
