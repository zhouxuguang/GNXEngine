//
//  SSAOFrameWork.h
//  ssao
//
//  SSAO (Screen Space Ambient Occlusion) Demo
//

#ifndef SSAOFrameWork_h
#define SSAOFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

class SSAOFrameWork : public GNXEngine::AppFrameWork
{
public:
    SSAOFrameWork(const GNXEngine::WindowProps& props);
    
    void Initlize() override;
    
    void Resize(uint32_t width, uint32_t height) override;
    
    void RenderFrame() override;
    
    void OnEvent(GNXEngine::Event& e) override;
    
private:
    bool OnKeyUp(GNXEngine::KeyReleasedEvent& e);

    // 场景只创建一次：Resize 会被窗口尺寸变化反复调用
    bool mSceneCreated = false;
};

#endif /* SSAOFrameWork_h */
