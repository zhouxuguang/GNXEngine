//
//  NaniteFrameWork.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef NaniteFrameWork_hpp
#define NaniteFrameWork_hpp

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

struct GlobaleData
{
	mathutil::Matrix4x4f modelMatrix;
	uint32_t misc0[4];
	float Nanite_ViewOrigin[4];
	float Nanite_ViewForward[4];
};

class NaniteFrameWork : public GNXEngine::AppFrameWork
{
public:
    NaniteFrameWork(const GNXEngine::WindowProps& props);
    
    virtual void Initlize();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual void RenderFrame();
    
    virtual void OnEvent(GNXEngine::Event& e);
    
private:
    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    RenderCore::RCBufferPtr mHierarchyBuffer = nullptr;
    RenderCore::RCBufferPtr mClusterPageData = nullptr;
    
    RenderCore::UniformBufferPtr mGlobalBuffer = nullptr;

    GlobaleData mGlobalData;

    RenderSystem::TransientResources *mTransientResources = nullptr;

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;

private:
    RenderCore::RCBufferPtr InitNaniteMeshBuffer();
    
    bool OnKeyUp(GNXEngine::KeyReleasedEvent& e);
};

#endif /* NaniteFrameWork_hpp */
