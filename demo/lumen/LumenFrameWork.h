//
//  LumenFrameWork.h
//  lumen
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef LumenFrameWork_hpp
#define LumenFrameWork_hpp

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

class LumenFrameWork : public GNXEngine::AppFrameWork
{
public:
    LumenFrameWork(const GNXEngine::WindowProps& props);
    
    virtual void Initlize();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual void RenderFrame();
    
    virtual void OnEvent(GNXEngine::Event& e);
    
private:
    
    bool OnKeyUp(GNXEngine::KeyReleasedEvent& e);
};

#endif /* LumenFrameWork_hpp */
