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
#include "Runtime/RenderCore/include/RCTexture.h"

// 前置声明（避免引入完整头文件）
NS_RENDERSYSTEM_BEGIN
class SkyBox;
class SkyBoxNode;
NS_RENDERSYSTEM_END

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
    
    // IBL 资源（BRDF LUT 在启动时预计算一次）
    RenderCore::RCTexturePtr mBRDFLUT = nullptr;
    
    // 天空盒（默认渐变天空，可后续替换为真实环境贴图）
    RenderSystem::SkyBox* mSkyBox = nullptr;
};

#endif /* PBRFrameWork_h */
