//
//  VirtualTextureManager.h
//  GNXEngine
//
//  Virtual Texture 系统的顶层协调器。
//  统筹 page table、物理 cache、feedback pass、异步 streaming。
//  所有可配参数通过 VirtualTextureConfig 传入并分发到各子组件。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_MANAGER_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_MANAGER_H

#include "VirtualTextureDefines.h"
#include "VirtualTextureDataSource.h"
#include "VirtualTexturePageTable.h"
#include "VirtualTextureCache.h"
#include "VirtualTextureFeedback.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include <vector>
#include <queue>
#include <future>
#include <memory>

NS_RENDERSYSTEM_BEGIN

/// 异步 page 加载请求状态。
struct PageLoadRequest
{
    PageRequest   page;
    std::future<std::vector<uint8_t>> future;
    bool          isReady;
    PageSlot      targetSlot;
};

/// VT 系统顶层管理器。
class RENDERSYSTEM_API VirtualTextureManager
{
public:
    VirtualTextureManager();
    ~VirtualTextureManager() = default;

VirtualTextureManager(const VirtualTextureManager&) = delete;
    VirtualTextureManager& operator=(const VirtualTextureManager&) = delete;

    /// 初始化 VT 系统，传入所有运行时参数及数据源。
    void Initialize(const VirtualTextureConfig& config,
                    std::shared_ptr<IVirtualTextureDataSource> dataSource,
                    uint32_t feedbackScale = 4);

    /// 每帧执行 VT 管线。
    void Tick();

    // 纹理绑定接口
    const RCTexturePtr& GetPageTableTexture() const { return mPageTable->GetGPUTexture(); }
    const RCTexturePtr& GetAtlasTexture()    const { return mCache->GetAtlasTexture(); }
    const RCTexturePtr& GetFeedbackTarget()   const { return mFeedback->GetFeedbackTarget(); }
    const RCTexturePtr& GetFeedbackDepthTarget() const { return mFeedback->GetDepthTarget(); }

    /// 获取当前配置（只读）。
    const VirtualTextureConfig& GetConfig() const { return mConfig; }

    /// 每帧最大上传数。
    void SetUploadsPerFrame(uint32_t count) { mCache->SetUploadsPerFrame(count); }

private:
    VirtualTextureConfig mConfig;

    VirtualTexturePageTablePtr mPageTable;
    VirtualTextureCachePtr     mCache;
    VirtualTextureFeedbackPtr  mFeedback;
    std::shared_ptr<IVirtualTextureDataSource> mDataSource;

    std::vector<PageLoadRequest> mPendingLoads;      //当前请求的结果
    std::set<PageRequest>      mPendingRequests;   //请求队列

    void DispatchLoadRequests(const FeedbackResult& feedback);
    void ProcessCompletedLoads();
    void RequestPageAsync(const PageRequest& page, const PageSlot& slot);
};

using VirtualTextureManagerPtr = std::shared_ptr<VirtualTextureManager>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_MANAGER_H */