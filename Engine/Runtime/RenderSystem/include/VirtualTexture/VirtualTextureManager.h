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
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include <vector>
#include <queue>
#include <set>
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
                    const mathutil::Vector2i& viewSize,
                    uint32_t feedbackScale = 16);

    /// 每帧执行 VT 管线。
    void Tick();

    /// 主视口变化时同步 feedback target 尺寸。
    void Resize(const mathutil::Vector2i& viewSize);

    /// Feedback pass 已加入本帧渲染，允许下一帧 Tick 读回。
    void NotifyFeedbackRendered();

    // 纹理绑定接口
    RCTexturePtr GetPageTableTexture() const { return mPageTable->GetGPUTexture(); }
    RCTexturePtr GetAtlasTexture()    const { return mAtlasTexture; }
    RCTexturePtr GetFeedbackTarget()   const { return mFeedback->GetFeedbackTarget(); }
    RCTexturePtr GetFeedbackDepthTarget() const { return mFeedback->GetDepthTarget(); }

    /// Page table 采样器（R32Uint 纹理必须用 point filter）
    TextureSamplerPtr GetPageTableSampler() const { return mPageTableSampler; }

    /// VT info uniform buffer（pageGrid / tileSize / atlasSize），供 shader 采样时使用
    UniformBufferPtr GetVTInfoUBO() const { return mVTInfoUBO; }

    /// VT feedback uniform buffer（vtSize / pageGrid / minMaxMipLevel / bufferScreenRatio）
    UniformBufferPtr GetFeedbackUBO() const { return mFeedbackUBO; }

    /// 获取当前配置（只读）。
    const VirtualTextureConfig& GetConfig() const { return mConfig; }

    /// 每帧最大上传数。
    void SetUploadsPerFrame(uint32_t count);

    // ── 调试/统计接口 ──
    uint32_t GetResidentPageCount() const;
    uint32_t GetAtlasSlotCapacity() const;
    uint32_t GetPendingLoadCount()   const { return static_cast<uint32_t>(mPendingLoads.size()); }
    uint32_t GetPendingRequestCount() const { return static_cast<uint32_t>(mPendingRequests.size()); }
    uint32_t GetLastFeedbackPageCount() const { return mLastFeedbackPageCount; }
    uint64_t GetTotalUploadedPageCount() const { return mTotalUploadedPageCount; }
    uint64_t GetFailedPageLoadCount() const { return mFailedPageLoadCount; }

private:
    VirtualTextureConfig mConfig;

    VirtualTexturePageTablePtr mPageTable;
    VirtualTextureCachePtr     mCache;
    VirtualTextureFeedbackPtr  mFeedback;
    std::shared_ptr<IVirtualTextureDataSource> mDataSource;
    RCTexture2DPtr mAtlasTexture;
    UniformBufferPtr mVTInfoUBO;
    UniformBufferPtr mFeedbackUBO;
    TextureSamplerPtr mPageTableSampler;

    std::vector<PageLoadRequest> mPendingLoads;      //当前请求的结果
    std::set<PageRequest>      mPendingRequests;   //请求队列
    uint32_t mLastFeedbackPageCount = 0;
    uint64_t mTotalUploadedPageCount = 0;
    uint64_t mFailedPageLoadCount = 0;

    void DispatchLoadRequests(const FeedbackResult& feedback);
    void ProcessCompletedLoads();
    void RequestPageAsync(const PageRequest& page, const PageSlot& slot);

    /// 把 page 加入异步加载队列（分配 slot 已由调用方完成）。
    void EnqueuePage(const PageRequest& page, const PageSlot& slot);

    /// 预加载所有常驻（最粗糙）mip 的 page，作为 mip 回退的兜底数据。
    void PreloadPinnedPages();
};

using VirtualTextureManagerPtr = std::shared_ptr<VirtualTextureManager>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_MANAGER_H */
