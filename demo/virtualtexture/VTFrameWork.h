//
//  VTFrameWork.h
//  virtualtexture
//
//  Virtual Texture Demo
//

#ifndef VTFrameWork_h
#define VTFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderSystem/include/VirtualTexture/VirtualTextureManager.h"

class VTFrameWork : public GNXEngine::AppFrameWork
{
public:
    VTFrameWork(const GNXEngine::WindowProps& props);

    virtual void Initlize() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void RenderFrame() override;
    virtual void OnEvent(GNXEngine::Event& e) override;

private:
    uint32_t mVTIndex = UINT32_MAX;  // SceneManager 中 VT 管理器的索引
};

#endif /* VTFrameWork_h */