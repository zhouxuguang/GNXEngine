//
//  VirtualTextureManager.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureManager.h"

NS_RENDERSYSTEM_BEGIN

VirtualTextureManager::VirtualTextureManager()
{
}

void VirtualTextureManager::Initialize(const VirtualTextureConfig& config, uint32_t feedbackScale)
{
    VirtualTextureConfig resolved = config;
    resolved.ResolveDerived();

    mConfig = resolved;

    mPageTable = std::make_shared<VirtualTexturePageTable>(resolved);
    mCache     = std::make_shared<VirtualTextureCache>(resolved);
    mFeedback  = std::make_shared<VirtualTextureFeedback>(feedbackScale, resolved);

    // TODO: 预填充最低几级 mip 的所有 page（pinned）。
}

void VirtualTextureManager::Tick()
{
    ProcessCompletedLoads();

    FeedbackResult feedback = mFeedback->ReadbackAndDecode();
    DispatchLoadRequests(feedback);

    mPageTable->SyncToGPU();
}

void VirtualTextureManager::DispatchLoadRequests(const FeedbackResult& feedback)
{
    for (const auto& req : feedback.requestedPages)
    {
        // 递归添加父级 page（parent fallback）
        PageRequest parent = req;
        while (parent.mipLevel > 0)
        {
            parent.mipLevel--;
            parent.pageX >>= 1;
            parent.pageY >>= 1;
            if (mPageTable->IsResident(parent)) break; // 父级已加载，更高层自然也已加载
        }
        // 父级检查完后回到原始 request
        PageRequest current = req;

        if (mPageTable->IsResident(current))
        {
            mCache->Touch(current);
            continue;
        }

        if (mPendingLoads.size() >= mConfig.uploadsPerFrame * 2) break; // 队列积压保护

        CacheAllocation alloc = mCache->Allocate(current);
        if (!alloc.success) continue;

        if (alloc.hasEvicted)
        {
            mPageTable->ClearEntry(alloc.evictedRequest);
        }

        mPageTable->WriteEntry(current, 0);
        RequestPageAsync(current, alloc.slot);
    }
}

void VirtualTextureManager::ProcessCompletedLoads()
{
    uint32_t uploaded = 0;
    for (auto it = mPendingLoads.begin(); it != mPendingLoads.end() && uploaded < mConfig.uploadsPerFrame; )
    {
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto data = it->future.get();
            // TODO: 将 data 上传到物理 atlas 的 it->targetSlot 位置。
            //       使用 RHI UpdateSubRegion or Blit.
            // TODO: 更新 page table entry:
            //       mPageTable->WriteEntry(it->page,
            //           EncodePageTableEntry(it->targetSlot.atlasX, it->targetSlot.atlasY));
            it = mPendingLoads.erase(it);
            uploaded++;
        }
        else
        {
            ++it;
        }
    }
}

void VirtualTextureManager::RequestPageAsync(const PageRequest& page, const PageSlot& slot)
{
    mPendingLoads.push_back({page, {}, false, slot});
}

NS_RENDERSYSTEM_END
