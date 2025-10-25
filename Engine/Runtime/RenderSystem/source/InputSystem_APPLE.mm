//
//  InputSystem_APPLE.mm
//  GNXEditor
//
//  Created by zhouxuguang on 2024/4/20.
//

#include "InputSystem_APPLE.h"
#include <GameController/GameController.h>

NS_RENDERSYSTEM_BEGIN

InputSystem_APPLE::InputSystem_APPLE()
{
    mouseDelta.x = 0;
    mouseDelta.y = 0;
    
    mouseScroll.x = 0;
    mouseScroll.y = 0;
    
    center = [NSNotificationCenter defaultCenter];
    
    //注册键盘按键的观察者
    [center addObserverForName:GCKeyboardDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification *notification)
     {
         // Get the connected keyboard object
         GCKeyboard *keyboard = notification.object;
         
         // Set the key changed handler for the keyboard input
         keyboard.keyboardInput.keyChangedHandler = ^(GCKeyboardInput *keyboard, GCDeviceButtonInput *key, GCKeyCode keyCode, BOOL pressed)
         {
             if (pressed)
             {
                 // Add the pressed key code to the keysPressed set
                keysPressed.insert(keyCode);
                 //[self.keysPressed addObject:buttonWhichChanged];
             }
             else
             {
                 // Remove the released key code from the keysPressed set
                keysPressed.erase(keyCode);
             }
         };
     }];
    
#if TARGET_OS_MAC
    id eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp handler:^NSEvent * (NSEvent *event) {
        return nil;
    }];
#endif
    
    //注册鼠标事件
    [center addObserverForName:GCMouseDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification *notification)
     {
         // Get the connected keyboard object
         GCMouse *mouse = notification.object;
     
     //鼠标左键
     mouse.mouseInput.leftButton.pressedChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed)
     {
        leftMouseDown = pressed;
     };
     
     //鼠标移动
     mouse.mouseInput.mouseMovedHandler = ^(GCMouseInput * _Nonnull mouse, float deltaX, float deltaY) {
        mouseDelta = MousePoint(deltaX, deltaY);
     };
     
     //鼠标滚轮
     mouse.mouseInput.scroll.valueChangedHandler = ^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
        mouseScroll.x = xValue;
        mouseScroll.y = yValue;
     };
     
     }];
}

InputSystem_APPLE::~InputSystem_APPLE()
{
    //
}

InputSystem *createInputSystem_APPLE()
{
    return new InputSystem_APPLE();
}


NS_RENDERSYSTEM_END
