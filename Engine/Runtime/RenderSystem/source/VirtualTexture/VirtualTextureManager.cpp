//
//  VirtualTextureManager.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureManager.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include <chrono>

NS_RENDERSYSTEM_BEGIN

VirtualTextureManager::VirtualTextureManager()
{
}

void VirtualTextureManager::Initialize(const VirtualTextureConfig& config, 
                                        std::shared_ptr<IVirtualTextureDataSource> dataSource,
                                       const mathutil::Vector2i& viewSize,
                                        uint32_t feedbackScale)
{
    VirtualTextureConfig resolved = config;
    resolved.ResolveDerived();

    mConfig = resolved;
    mDataSource = std::move(dataSource);

    mPageTable = std::make_shared<VirtualTexturePageTable>(resolved);
    mCache     = std::make_shared<VirtualTextureCache>(resolved);
    mFeedback  = std::make_shared<VirtualTextureFeedback>(viewSize, feedbackScale, resolved);

    mAtlasTexture = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatSRGB8_ALPHA8,
                RenderCore::TextureUsage::TextureUsageShaderRead, resolved.atlasWidth, resolved.atlasHeight, 1);

    // VT info uniform buffer
    VTInfoBufferData vtInfoData;
    vtInfoData.FillFromConfig(resolved);
    mVTInfoUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(VTInfoBufferData));
    mVTInfoUBO->SetData(&vtInfoData, 0, sizeof(VTInfoBufferData));
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
    const uint32_t slotSizeX = mConfig.slotSize;
    const uint32_t slotSizeY = mConfig.slotSize;

    // 将处理完的tile从队列中移除，此时也是上传tile到缓存，以及更新pagetable的时机
    std::erase_if(mPendingLoads, [this, slotSizeX, slotSizeY](PageLoadRequest& req)
    {
        if (req.future.wait_for(std::chrono::microseconds(0)) != std::future_status::ready) 
        {
            return false;
        }

        ByteVector result = req.future.get();
        if (result.empty())
        {
            mPendingRequests.erase(req.page);
            return true;
        }

        RenderCore::Rect2D region;
        region.offsetX = slotSizeX * req.targetSlot.atlasX;
        region.offsetY = slotSizeY * req.targetSlot.atlasY;
        region.width = slotSizeX;
        region.height = slotSizeY;
        mAtlasTexture->ReplaceRegion(region, 0, result.data(), slotSizeX * 4);   //先假定是RGBA8的图像

        uint32_t entry = 0x1 | ((req.targetSlot.atlasX & 0xFFu) << 1) | ((req.targetSlot.atlasY & 0xFFu) << 9);

        mPageTable->WriteEntry(req.page, entry);
        mCache->Commit(req.page, req.targetSlot);
        mPendingRequests.erase(req.page);

        return true;
    });
}

void VirtualTextureManager::RequestPageAsync(const PageRequest& page, const PageSlot& slot)
{
    mPendingRequests.insert(page);

    if (!mDataSource)
    {
        mPendingLoads.push_back({page, {}, false, slot});
        return;
    }

    auto future = mDataSource->RequestTile(page);
    mPendingLoads.push_back({page, std::move(future), false, slot});
}

NS_RENDERSYSTEM_END
