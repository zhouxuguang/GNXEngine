//
//  DX12BlitEncoder.h
//  rendercore
//
//  D3D12 拷贝编码器（Blit）。
//
//  D3D12 没有 blit/resolve 命令，所有拷贝都通过 CopyBufferRegion /
//  CopyTextureRegion / CopyResource 完成。资源状态转换（COPY_SRC ↔ COPY_DST）
//  在每个操作内部按需插入，结束后恢复为 SHADER_RESOURCE，
//  与 Vulkan 后端“拷贝前后自动转换”的行为保持一致。
//
//  mipmap 生成：D3D12 没有 vkCmdBlitImage 的等价物，必须自己写降采样 compute
//  pass。这里通过一个内置的 mipmap 生成 shader 完成（见 DX12BlitEncoder.cpp）。
//

#ifndef GNX_ENGINE_DX12_BLIT_ENCODER_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_BLIT_ENCODER_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12CommandBuffer.h"
#include "BlitEncoder.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12BlitEncoder : public BlitEncoder
{
public:
    explicit DX12BlitEncoder(const DX12CommandBufferPtr& commandBuffer);
    ~DX12BlitEncoder() override;

    // ---- Buffer 操作 ----
    void CopyBuffer(RCBufferPtr source, uint64_t sourceOffset,
                    RCBufferPtr destination, uint64_t destinationOffset,
                    uint64_t size) override;
    void FillBuffer(RCBufferPtr destination, uint64_t destinationOffset,
                    const void* data, uint64_t dataSize) override;
    void CopyTextureToBuffer(RCTexturePtr source, uint32_t sourceSlice, uint32_t sourceMipLevel,
                             const mathutil::Vector2i& sourceOffset,
                             const mathutil::Vector2i& sourceSize,
                             RCBufferPtr destination, uint64_t destinationOffset,
                             uint64_t destinationBytesPerRow,
                             uint64_t destinationBytesPerImage) override;
    void CopyBufferToTexture(RCBufferPtr source, uint64_t sourceOffset,
                             uint64_t sourceBytesPerRow, uint64_t sourceBytesPerImage,
                             RCTexturePtr destination, uint32_t destinationSlice,
                             uint32_t destinationMipLevel,
                             const mathutil::Vector2i& destinationOffset,
                             const mathutil::Vector2i& destinationSize) override;

    // ---- Texture 到 Texture ----
    void CopyTextureToTexture(RCTexturePtr source, uint32_t sourceSlice, uint32_t sourceMipLevel,
                              const mathutil::Vector2i& sourceOffset,
                              const mathutil::Vector2i& sourceSize,
                              RCTexturePtr destination, uint32_t destinationSlice,
                              uint32_t destinationMipLevel,
                              const mathutil::Vector2i& destinationOffset,
                              const mathutil::Vector2i& destinationSize) override;

    // ---- Mipmap ----
    void GenerateMipmaps(RCTexturePtr texture, uint32_t slice) override;
    void GenerateMipmapsForRange(RCTexturePtr texture, uint32_t slice,
                                 uint32_t baseMipLevel, uint32_t levelCount) override;

    void EndEncode() override;

private:
    /// 把 buffer 拷贝到纹理（按 mip/slice 定位子资源，处理行距对齐）
    void DoBufferToTexture(ID3D12Resource* source, uint64_t sourceOffset,
                           uint64_t sourceBytesPerRow,
                           DX12TextureBase* destination, uint32_t slice, uint32_t mip,
                           const mathutil::Vector2i& offset, const mathutil::Vector2i& size);

    /// 把纹理拷贝到 buffer（需要中间 staging 缓冲处理行距对齐）
    void DoTextureToBuffer(DX12TextureBase* source, uint32_t slice, uint32_t mip,
                           const mathutil::Vector2i& offset, const mathutil::Vector2i& size,
                           ID3D12Resource* destination, uint64_t destinationOffset,
                           uint64_t destinationBytesPerRow);

    DX12CommandBufferPtr mCommandBuffer;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    DX12Context* mContext = nullptr;
    bool mEncoding = false;
};

using DX12BlitEncoderPtr = std::shared_ptr<DX12BlitEncoder>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_BLIT_ENCODER_INCLUDE_JHGSD */
