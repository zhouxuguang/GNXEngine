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

private:
    void SetupImGui();
    void SetupScene();
    void BuildImGuiPanel();
    void UpdateCameraLens(uint32_t width, uint32_t height);

    uint32_t mVTIndex = UINT32_MAX;  // SceneManager 中 VT 管理器的索引
    RenderSystem::VirtualTextureManagerPtr mVTManager = nullptr;

    // 物理 atlas（与 VT manager 共享所有权，供 ImGui 预览使用）
    RCTexturePtr mAtlasTexture = nullptr;

    uint32_t mWindowWidth = 1280;
    uint32_t mWindowHeight = 720;
    uint32_t mUploadsPerFrame = 8;

    bool  mShowPanel = true;
    bool  mShowAtlasPreview = true;
    float mAtlasPreviewSize = 256.0f;
};

#endif /* VTFrameWork_h */
