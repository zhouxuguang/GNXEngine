//
//  DX12ComputeEncoder.h
//  rendercore
//
//  D3D12 计算编码器。
//
//  与图形编码器共用命令缓冲区的描述符环形堆；区别是计算管线使用
//  DX12RootSignature 的 Compute 变体（4 张表、ALL 可见性），
//  UAV 目标必须在 Dispatch 前插入 UAV barrier。
//

#ifndef GNX_ENGINE_DX12_COMPUTE_ENCODER_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_COMPUTE_ENCODER_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12CommandBuffer.h"
#include "DX12Pipeline.h"
#include "ComputeEncoder.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12ComputeEncoder : public ComputeEncoder
{
public:
    explicit DX12ComputeEncoder(const DX12CommandBufferPtr& commandBuffer);
    ~DX12ComputeEncoder() override;

    void EndEncode() override;

    void SetComputePipeline(ComputePipelinePtr computePipeline) override;

    void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;
    void SetStorageBuffer(RCBufferPtr buffer, uint32_t index) override;
    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer) override;

    void SetTexture(RCTexturePtr texture, uint32_t index) override;
    void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    void SetTexture(const std::string& resourceName, RCTexturePtr texture) override;
    void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;

    void SetOutTexture(RCTexturePtr texture, uint32_t index) override;
    void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture) override;
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;

    void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) override;

private:
    bool EnsureBlock();
    const DX12BindInfo* ResolveBinding(const std::string& name) const;

    void WriteCBV(uint32_t reg, UniformBufferPtr buffer);
    void WriteSRVBuffer(uint32_t reg, RCBufferPtr buffer, bool asUAV,
                        bool isRawBuffer, uint32_t structuredStride);
    void WriteSRVTexture(uint32_t reg, RCTexturePtr texture, uint32_t mipLevel, bool asUAV);

    /// 记录 UAV 写入目标，Dispatch 前统一插入 UAV barrier
    void TrackUAVResource(ID3D12Resource* resource);

    DX12CommandBufferPtr mCommandBuffer;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    DX12Context* mContext = nullptr;

    DX12ComputePipeline* mComputePipeline = nullptr;
    bool mEncoding = false;
    bool mDispatchedSinceLastBarrier = false;
    std::vector<ComPtr<ID3D12Resource>> mPendingUAVBarriers;
};

using DX12ComputeEncoderPtr = std::shared_ptr<DX12ComputeEncoder>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_COMPUTE_ENCODER_INCLUDE_JHGSD */
