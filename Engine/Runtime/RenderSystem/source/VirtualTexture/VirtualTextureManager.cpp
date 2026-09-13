//
//  VirtualTextureManager.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureManager.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/LogService.h"
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

    // Page table point sampler（R32Uint 纹理必须用 nearest filter）
    RenderCore::SamplerDesc pointSamplerDesc;
    pointSamplerDesc.filterMag = RenderCore::MAG_NEAREST;
    pointSamplerDesc.filterMin = RenderCore::MIN_NEAREST;
    pointSamplerDesc.filterMip = RenderCore::MIN_NEAREST_MIPMAP_NEAREST;
    mPageTableSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(pointSamplerDesc);

    // VT feedback uniform buffer
    FeedbackBufferData fbData;
    fbData.FillFromConfig(resolved, feedbackScale);
    mFeedbackUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(FeedbackBufferData));
    mFeedbackUBO->SetData(&fbData, 0, sizeof(FeedbackBufferData));

    // 预加载常驻（最粗糙）mip，保证任意 uv 都有兜底数据可回退
    PreloadPinnedPages();
}

void VirtualTextureManager::Tick()
{
    ProcessCompletedLoads();

    FeedbackResult feedback = mFeedback->ReadbackAndDecode();

    //return;
    DispatchLoadRequests(feedback);

    mPageTable->SyncToGPU();
}

void VirtualTextureManager::DispatchLoadRequests(const FeedbackResult& feedback)
{
    for (const auto& req : feedback.requestedPages)
    {
        // 过滤非法请求：feedback 纹理首帧可能包含未初始化数据，
        // 若直接用来索引 page table 会越界。
        if (!IsValidPageRequest(mConfig, req))
        {
            continue;
        }

        if (mPageTable->IsResident(req))
        {
            mCache->Touch(req);
            continue;
        }

        // 已在加载队列中的 page 不重复请求
        if (mPendingRequests.find(req) != mPendingRequests.end())
        {
            continue;
        }

        if (mPendingLoads.size() >= mConfig.uploadsPerFrame * 2)
        {
            break; // 队列积压保护
        }

        CacheAllocation alloc = mCache->Allocate(req);
        if (!alloc.success)
        {
            continue;
        }

        // 被淘汰的 page：清除其 resident 标记，避免采样到已失效的 slot
        if (alloc.hasEvicted)
        {
            mPageTable->ClearEntry(alloc.evictedRequest);
        }

        RequestPageAsync(req, alloc.slot);
    }
}

void VirtualTextureManager::ProcessCompletedLoads()
{
    const uint32_t slotSizeX = mConfig.slotSize;
    const uint32_t slotSizeY = mConfig.slotSize;
    const uint32_t requiredBytes = slotSizeX * slotSizeY * 4;   // RGBA8
    const uint32_t maxUploads = mConfig.uploadsPerFrame;
    uint32_t uploadedCount = 0;

    // 将处理完的 tile 从队列中移除；此处也是上传 tile 到 atlas、更新 page table 的时机。
    // 每帧上传数量受 uploadsPerFrame 限制，超出的留到下一帧（让 streaming 分帧可见）。
    std::erase_if(mPendingLoads, [&](PageLoadRequest& req)
    {
        if (uploadedCount >= maxUploads)
        {
            return false;
        }

        if (req.future.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
        {
            return false;
        }

        ByteVector result = req.future.get();
        mPendingRequests.erase(req.page);

        if (result.size() < requiredBytes)
        {
            // 加载失败 / 尺寸不符：丢弃，允许后续重试
            return true;
        }

        RenderCore::Rect2D region;
        region.offsetX = slotSizeX * req.targetSlot.atlasX;
        region.offsetY = slotSizeY * req.targetSlot.atlasY;
        region.width = slotSizeX;
        region.height = slotSizeY;
        mAtlasTexture->ReplaceRegion(region, 0, result.data(), slotSizeX * 4);

        const uint32_t entry = 0x1 | ((req.targetSlot.atlasX & 0xFFu) << 1)
                                   | ((req.targetSlot.atlasY & 0xFFu) << 9);

        mPageTable->WriteEntry(req.page, entry);
        mCache->Commit(req.page, req.targetSlot);
        ++uploadedCount;

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

void VirtualTextureManager::PreloadPinnedPages()
{
    if (mConfig.pinnedMipLevels == 0 || !mDataSource)
    {
        return;
    }

    uint32_t pinned = mConfig.pinnedMipLevels;
    if (pinned > mConfig.mipLevels)
    {
        pinned = mConfig.mipLevels;
    }
    const uint32_t startMip = mConfig.mipLevels - pinned;

    uint32_t requested = 0;
    for (uint32_t mip = startMip; mip < mConfig.mipLevels; ++mip)
    {
        const uint32_t gridW = GetPageGridCount(mConfig.virtualWidth,  mConfig.pageSize, mip);
        const uint32_t gridH = GetPageGridCount(mConfig.virtualHeight, mConfig.pageSize, mip);

        for (uint32_t y = 0; y < gridH; ++y)
        {
            for (uint32_t x = 0; x < gridW; ++x)
            {
                PageRequest page{mip, x, y};
                if (mPageTable->IsResident(page) || mPendingRequests.find(page) != mPendingRequests.end())
                {
                    continue;
                }

                CacheAllocation alloc = mCache->Allocate(page);
                if (!alloc.success)
                {
                    return; // 没有空闲 slot（正常情况不会发生）
                }
                if (alloc.hasEvicted)
                {
                    mPageTable->ClearEntry(alloc.evictedRequest);
                }

                RequestPageAsync(page, alloc.slot);
                ++requested;
            }
        }
    }

    LOG_INFO("VT: preloaded %u pinned page(s) (mip %u..%u)", requested, startMip,
             mConfig.mipLevels > 0 ? mConfig.mipLevels - 1 : 0);
}

uint32_t VirtualTextureManager::GetResidentPageCount() const
{
    return mCache ? mCache->GetResidentCount() : 0;
}

uint32_t VirtualTextureManager::GetAtlasSlotCapacity() const
{
    return mCache ? mCache->GetCapacity() : 0;
}

NS_RENDERSYSTEM_END
