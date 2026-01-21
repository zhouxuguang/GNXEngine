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
    RenderCore::ComputeBufferPtr mHierarchyBuffer = nullptr;
    RenderCore::ComputeBufferPtr mClusterSelectionArgs1 = nullptr;
    RenderCore::ComputeBufferPtr mClusterPageData = nullptr;
    RenderCore::RCTexture2DPtr mVisBuffer = nullptr;
    RenderCore::RCTexture2DPtr mVisBuffer64 = nullptr;
    
    RenderCore::ComputeBufferPtr mRasterBinMeta = nullptr;
    RenderCore::ComputeBufferPtr mRasterBinData = nullptr;
    RenderCore::ComputeBufferPtr mMainAndPostNodeAndClusterBatches = nullptr;
    RenderCore::UniformBufferPtr mGlobalBuffer = nullptr;
    RenderCore::ComputeBufferPtr mWorkArgs[2];
    RenderCore::ComputeBufferPtr mQueueState = nullptr;
    RenderCore::ComputeBufferPtr mVisibleClustersSWHW = nullptr;

    GlobaleData mGlobalData;

    RenderSystem::TransientResources *mTransientResources = nullptr;

private:
    RenderCore::ComputeBufferPtr InitNaniteMeshBuffer();

    RenderCore::RCTexture2DPtr InitVisualizeBuffer();

    RenderCore::RCTexture2DPtr InitVisBuffer64();
    
    bool OnKeyUp(GNXEngine::KeyReleasedEvent& e);
};

#endif /* NaniteFrameWork_hpp */
