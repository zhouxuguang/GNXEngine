//
//  MTLCommandBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE
#define GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE

#include "MTLRenderDefine.h"
#include "CommandBuffer.h"
#include <vector>
#include <unordered_map>

NAMESPACE_RENDERCORE_BEGIN

struct FenceOp
{
    enum Type { Update, Wait };
    Type        type;
    uint64_t    resourceId;    // 纹理/Buffer 的指针哈希
    MTLRenderStages stage;     // 推导自 ResourceAccessType
};

// 命令缓冲区
class MTLCommandBuffer : public CommandBuffer
{
public:
    MTLCommandBuffer(id<MTLCommandQueue> commandQueue, CAMetalLayer *metalLayer,
                     id<MTLTexture> depthTexture, id<MTLTexture> stencilTexture, id<MTLTexture> depthStencilTexture);
    
    ~MTLCommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    RenderEncoderPtr CreateDefaultRenderEncoder(const ClearColor& clearColor = MakeClearColor(0, 0, 0, 1)) const override;
    
    RenderEncoderPtr CreateRenderEncoder(const RenderPass& renderPass) const override;
    
    // 创建计算着色器的encoder
    ComputeEncoderPtr CreateComputeEncoder() const override;
    
    // 创建Blit encoder，用于资源拷贝操作
    BlitEncoderPtr CreateBlitEncoder() const override;
    
    //呈现到屏幕上，上屏
    void PresentFrameBuffer() override;
    
    //等待命令缓冲区执行完成
    void WaitUntilCompleted() override;
    
    //提交命令缓冲区（用于计算命令缓冲区）
    void Submit() override;
    
    // 开始调试标记
    void BeginDebugGroup(const char* name, const float color[4]) override;
    
    // 结束调试标记
    void EndDebugGroup() override;
    
    /**
     * 通知命令缓冲区纹理资源即将被访问
     */
    virtual void ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType) override;

    /**
     * 通知命令缓冲区统一缓冲区资源即将被访问
     */
    virtual void ResourceBarrier(RCBufferPtr buffer, ResourceAccessType accessType) override;

    /// Metal 后端内部：通知当前 encoder 即将结束，刷新 pending update fence
    void OnEncoderEnding(id<MTLRenderCommandEncoder> encoder) const;

private:
    /// 获取或创建与 resourceId 关联的 MTLFence
    id<MTLFence> GetOrCreateFence(uint64_t resourceId) const;
    
    /// 将 ResourceAccessType 映射到 MTLRenderStages
    static MTLRenderStages AccessTypeToStage(ResourceAccessType access);

    id<MTLCommandBuffer> mCommandBuffer;
    CAMetalLayer *mMetalLayer = nil;
    mutable id<CAMetalDrawable> mCurrentDrawable = nil;
    
    id<MTLTexture> mDepthTexture = nil;
    id<MTLTexture> mStencilTexture = nil;
    id<MTLTexture> mDepthStencilTexture = nil;

    // Metal Fence 管理（Untracked 资源需要显式 fence）
    mutable std::vector<FenceOp> mPendingFenceOps;             // 待处理的 fence op
    mutable std::unordered_map<uint64_t, id<MTLFence>> mFenceMap;  // resourceId → MTLFence
    mutable id<MTLDevice> mMetalDevice = nil;
    
    // 仅用于 pending 判断：上一轮帧的 fence op 是否已在当前 encoder 上生效
    mutable bool mFenceOpsDirty = false;
};

typedef std::shared_ptr<MTLCommandBuffer> MTLCommandBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE */
