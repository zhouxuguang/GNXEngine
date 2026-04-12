//
//  PBRFrameWork.h
//  pbr
//
//  PBR (Physically Based Rendering) Demo
//  Demonstrates PBR material rendering with the DamagedHelmet model,
//  multiple material spheres (metal/roughness variations),
//  and a textured ground plane.
//

#ifndef PBRFrameWork_h
#define PBRFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

class PBRFrameWork : public GNXEngine::AppFrameWork
{
public:
    PBRFrameWork(const GNXEngine::WindowProps& props);
    
    virtual void Initlize() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void RenderFrame() override;
    virtual void OnEvent(GNXEngine::Event& e) override;
    
private:
    bool OnKeyUp(GNXEngine::KeyReleasedEvent& e);
};

#endif /* PBRFrameWork_h */
