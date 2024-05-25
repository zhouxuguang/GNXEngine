//
//  InputSystem_APPLE.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/4/20.
//

#ifndef InputSystem_APPLE_hpp
#define InputSystem_APPLE_hpp

#include "RSDefine.h"
#include <NotificationCenter/NotificationCenter.h>
#include <set>
#include "InputSystem.h"

NS_RENDERSYSTEM_BEGIN

class InputSystem_APPLE : public InputSystem
{
public:
    InputSystem_APPLE();
    
    ~InputSystem_APPLE();
    
private:
    NSNotificationCenter *center;
    std::set<signed long> keysPressed;
    
};


NS_RENDERSYSTEM_END

#endif /* InputSystem_APPLE_hpp */
