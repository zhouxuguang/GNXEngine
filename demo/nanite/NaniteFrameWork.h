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

class NaniteFrameWork : public GNXEngine::AppFrameWork
{
public:
    NaniteFrameWork(const GNXEngine::WindowProps& props);
    
    virtual void Initlize();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual void RenderFrame();
    
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

private:
    RenderCore::ComputeBufferPtr InitNaniteMeshBuffer();

    RenderCore::RCTexture2DPtr InitVisualizeBuffer();

    RenderCore::RCTexture2DPtr InitVisBuffer64();
};

#endif /* NaniteFrameWork_hpp */
