//
//  DX12Pipeline.h
//  rendercore
//
//  D3D12 管线对象（Graphics / Compute / Mesh）与 PSO 变体缓存。
//
//  ── 与 Vulkan 的关键差异：D3D12 几乎没有“动态状态” ──
//  Vulkan 后端大量依赖 VK_EXT_extended_dynamic_state 在运行时改
//  polygonMode / depthBias / blend 等状态。D3D12 除了 primitive topology、
//  scissor/viewport、stencil reference 之外，这些状态全部是 PSO 静态的。
//  因此这里采用 **PSO 变体缓存**：把运行时可变的状态纳入变体键，
//  在 Encoder 调用 SetFillMode / SetDepthBias 时惰性生成并切换变体。
//
//  变体键 = (fillMode, depthBias.factor, depthBias.unite)
//  实际使用中每个 pass 的深度偏移是固定的，变体数量很少。
//

#ifndef GNX_ENGINE_DX12_PIPELINE_INCLUDE_JHGSDF
#define GNX_ENGINE_DX12_PIPELINE_INCLUDE_JHGSDF

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "DX12RootSignature.h"
#include "DX12ShaderFunction.h"
#include "GraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

/// 管线创建所需的渲染目标格式（对应 Vulkan 后端的 RenderPassFormat）
struct DX12RenderPassFormat
{
    std::vector<DXGI_FORMAT> colorFormats;
    DXGI_FORMAT depthFormat   = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT stencilFormat = DXGI_FORMAT_UNKNOWN;
    uint32_t    sampleCount   = 1;

    bool operator==(const DX12RenderPassFormat& other) const
    {
        return colorFormats == other.colorFormats &&
               depthFormat == other.depthFormat &&
               stencilFormat == other.stencilFormat &&
               sampleCount == other.sampleCount;
    }
};

/// PSO 变体键
struct DX12PSOVariantKey
{
    FillMode fillMode = FillModeSolid;
    float    depthBiasFactor = 0.0f;
    float    depthBiasUnits  = 0.0f;

    bool operator==(const DX12PSOVariantKey& other) const
    {
        return fillMode == other.fillMode &&
               depthBiasFactor == other.depthBiasFactor &&
               depthBiasUnits == other.depthBiasUnits;
    }
    bool operator<(const DX12PSOVariantKey& other) const
    {
        if (fillMode != other.fillMode) return fillMode < other.fillMode;
        if (depthBiasFactor != other.depthBiasFactor) return depthBiasFactor < other.depthBiasFactor;
        return depthBiasUnits < other.depthBiasUnits;
    }
};

// ============================================================================
// 图形管线
// ============================================================================
class DX12GraphicsPipeline : public GraphicsPipeline
{
public:
    DX12GraphicsPipeline(const DX12ContextPtr& context, DX12RootSignature* rootSignature,
                         const GraphicsPipelineDesc& des);

    ~DX12GraphicsPipeline() override = default;

    void AttachVertexShader(ShaderFunctionPtr shaderFunction) override;
    void AttachFragmentShader(ShaderFunctionPtr shaderFunction) override;
    void AttachGraphicsShader(GraphicsShaderPtr graphicsShader) override;
    void AttachTaskShader(ShaderFunctionPtr shaderFunction) override;
    void AttachMeshShader(ShaderFunctionPtr shaderFunction) override;

    const uint32_t* GetMeshThreadgroupSize() const override { return mMeshThreadgroupSize; }
    const uint32_t* GetTaskThreadgroupSize() const override { return mTaskThreadgroupSize; }

    /**
     * @brief 取得（必要时创建）指定渲染目标格式与状态变体下的 PSO
     * @return PSO 指针；创建失败返回 nullptr（会打印详细错误）
     */
    ID3D12PipelineState* GetPipelineState(const DX12RenderPassFormat& format,
                                          const DX12PSOVariantKey& variant);

    const DX12GraphicsShaderPtr& GetShader() const { return mShader; }
    DX12RootSignature* GetRootSignature() const { return mRootSignature; }

    bool IsMeshPipeline() const { return mDesc.pipelineType == PipelineType::Mesh; }

    /**
     * @brief 取得顶点缓冲每个 slot 的 stride（供 IASetVertexBuffers 使用）。
     *
     * D3D12 与 Vulkan 在这里有一个关键差异：
     *   - Vulkan 的 stride 属于管线状态（VkVertexInputBindingDescription），绑定时不需要提供；
     *   - D3D12 必须在 IASetVertexBuffers 时给出 StrideInBytes，PSO 推导不出来。
     * 因此绑定顶点缓冲时必须回查管线。引擎采用"平面布局"——每个顶点属性独占一个 slot，
     * 该 slot 的 stride 即属性格式的元素大小。
     */
    uint32_t GetVertexStride(uint32_t slot) const;

    /// 描述符表宽度（按合并后的反射结果）
    uint32_t GetTableWidth(DX12DescriptorClass cls) const;

    /**
     * @brief 描述符块需要的 stage 组数
     *
     * 非 Mesh 管线为 1（根签名的 4 张表对全部 stage 可见）；
     * Mesh 管线为 3（AMPLIFICATION / MESH / PIXEL 三组表，因为各 stage 的
     * HLSL 寄存器独立分配，同一资源名在不同 stage 可能落在不同寄存器）。
     */
    uint32_t GetStageGroupCount() const
    {
        return IsMeshPipeline() ? GNX_DX12_STAGE_GROUP_MAX : 1;
    }

    /// 资源名 → 绑定信息（合并结果，用于日志与普通图形管线）
    const DX12BindInfo* FindBinding(const std::string& name) const;

    /**
     * @brief 按 stage 解析资源名 → 绑定信息
     *
     * Mesh 管线必须使用该重载：合并结果只保留第一个 stage 的寄存器号，
     * 而各 stage 的寄存器号可能不同（见 DX12RenderDefine.h 的 stage 组说明）。
     */
    const DX12BindInfo* FindBinding(const std::string& name, ShaderStage stage) const;

    /// 线框变体是否可用（用于日志）
    bool HasAnyPipeline() const { return !mVariants.empty(); }

private:
    struct VariantKey
    {
        DX12RenderPassFormat format;
        DX12PSOVariantKey    state;

        bool operator<(const VariantKey& other) const
        {
            if (format.colorFormats != other.format.colorFormats)
            {
                return format.colorFormats < other.format.colorFormats;
            }
            if (format.depthFormat != other.format.depthFormat)   return format.depthFormat < other.format.depthFormat;
            if (format.stencilFormat != other.format.stencilFormat) return format.stencilFormat < other.format.stencilFormat;
            if (format.sampleCount != other.format.sampleCount)   return format.sampleCount < other.format.sampleCount;
            return state < other.state;
        }
    };

    ID3D12PipelineState* CreatePipelineState(const DX12RenderPassFormat& format,
                                             const DX12PSOVariantKey& variant);

    void BuildInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& outElements,
                          std::vector<std::string>& outSemanticNames) const;

    DX12ContextPtr mContext;
    DX12RootSignature* mRootSignature = nullptr;
    DX12GraphicsShaderPtr mShader;

    // 变体缓存（按格式 + 状态）
    std::map<VariantKey, ComPtr<ID3D12PipelineState>> mVariants;
    std::map<VariantKey, bool> mFailedVariants;   // 避免重复尝试失败组合并反复刷日志

    uint32_t mMeshThreadgroupSize[3] = { 1, 1, 1 };
    uint32_t mTaskThreadgroupSize[3] = { 1, 1, 1 };
};

using DX12GraphicsPipelinePtr = std::shared_ptr<DX12GraphicsPipeline>;

// ============================================================================
// 计算管线
// ============================================================================
class DX12ComputePipeline : public ComputePipeline
{
public:
    DX12ComputePipeline(const DX12ContextPtr& context, DX12RootSignature* rootSignature,
                        const ShaderCode& shaderCode);
    DX12ComputePipeline(const DX12ContextPtr& context, DX12RootSignature* rootSignature,
                        const ShaderStageData& shader);
    // 注意：ComputePipeline 基类没有虚析构函数，因此这里不能加 override
    ~DX12ComputePipeline() = default;

    void GetThreadGroupSizes(uint32_t& x, uint32_t& y, uint32_t& z) override;

    ID3D12PipelineState* GetPipelineState();
    DX12RootSignature* GetRootSignature() const { return mRootSignature; }
    const DX12ShaderFunctionPtr& GetShader() const { return mShader; }

    uint32_t GetTableWidth(DX12DescriptorClass cls) const;
    const DX12BindInfo* FindBinding(const std::string& name) const;

private:
    bool EnsurePipelineState();

    DX12ContextPtr mContext;
    DX12RootSignature* mRootSignature = nullptr;
    DX12ShaderFunctionPtr mShader;
    ComPtr<ID3D12PipelineState> mPipelineState;
    bool mCreationAttempted = false;
};

using DX12ComputePipelinePtr = std::shared_ptr<DX12ComputePipeline>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_PIPELINE_INCLUDE_JHGSDF */
