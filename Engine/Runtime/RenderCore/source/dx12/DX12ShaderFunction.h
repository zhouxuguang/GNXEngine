//
//  DX12ShaderFunction.h
//  rendercore
//
//  DXIL shader 对象与反射。
//
//  ── 绑定模型 ──
//  引擎的 HLSL 使用 b / t / u / s 四类寄存器；其中公共 cbuffer（如
//  GNXEngineVariables.hlsl 里的 cbPerCamera / cbPerObject）是**隐式分配**的，
//  寄存器号由 DXC 按“引用顺序”决定，且 VS 与 PS 各自独立编译，号可能不同。
//  因此离线编译阶段按名字提取寄存器号，并随 DXIL 一起写入 shader 资产。
//
//  反射结果 (名字 → 类别 + 寄存器号) 直接就是根签名里 4 张描述符表的槽位下标。
//

#ifndef GNX_ENGINE_DX12_SHADER_FUNCTION_INCLUDE_HGDS
#define GNX_ENGINE_DX12_SHADER_FUNCTION_INCLUDE_HGDS

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "ShaderFunction.h"
#include "ShaderStageData.h"

NAMESPACE_RENDERCORE_BEGIN

/// 描述符类别，同时用作根签名里 4 张描述符表的下标
enum class DX12DescriptorClass : uint32_t
{
    CBV     = 0,
    SRV     = 1,
    UAV     = 2,
    Sampler = 3,
    Count   = 4,
};

/// 某类描述符表在根签名里声明的上限（超出该范围的寄存器无法被寻址）
inline uint32_t DX12TableWidthLimit(DX12DescriptorClass cls)
{
    switch (cls)
    {
        case DX12DescriptorClass::CBV:     return GNX_DX12_CBV_TABLE_WIDTH;
        case DX12DescriptorClass::SRV:     return GNX_DX12_SRV_TABLE_WIDTH;
        case DX12DescriptorClass::UAV:     return GNX_DX12_UAV_TABLE_WIDTH;
        case DX12DescriptorClass::Sampler: return GNX_DX12_SAMPLER_TABLE_WIDTH;
        default:                           return 1;
    }
}

/// shader 入口函数名（与 ShaderCompiler 的 -E 参数保持一致）
const char* DX12GetEntryPointName(ShaderStage stage);

/// 单条资源绑定的反射结果
struct DX12BindInfo
{
    DX12DescriptorClass  cls = DX12DescriptorClass::SRV;
    uint32_t             bindPoint = 0;      // HLSL 寄存器号
    uint32_t             bindCount = 1;
    D3D12_SRV_DIMENSION  srvDimension = D3D12_SRV_DIMENSION_UNKNOWN;
    ShaderStage          stage = ShaderStage_Vertex;

    /// 缓冲区是否为 Raw（字节寻址）视图。
    ///
    /// 这直接决定 SRV/UAV 的创建方式，必须从反射里取，不能猜：
    ///   为真 → HLSL 声明是 ByteAddressBuffer / RWByteAddressBuffer
    ///          → 必须用 R32_TYPELESS + D3D12_BUFFER_*_FLAG_RAW 创建，**不需要步长**
    ///   为假 → HLSL 声明是 StructuredBuffer / RWStructuredBuffer<T>
    ///          → 必须用 StructureByteStride = sizeof(T) 创建
    ///
    /// Raw buffers use no stride; structured buffers require one.
    bool isRawBuffer = false;

    /// 结构化缓冲的元素步长（仅在 isRawBuffer == false 时有意义）
    uint32_t structuredStride = 0;
};

/// 顶点输入签名的一项（用于构造 PSO 的输入布局）
//
// 刻意从 shader 的输入签名反查语义名，而不是在引擎里硬编码 "POSITION"/"TEXCOORD"，
// 这样 shader 改动语义名时不需要同步改后端代码。映射依据是 input register 下标，
// 与引擎 VertexDesc 中 attributes 的 index 一一对应。
struct DX12InputParam
{
    std::string semanticName;
    uint32_t    semanticIndex = 0;
    uint32_t    registerIndex = 0;
};

// ============================================================================
// 单个 shader 阶段
// ============================================================================
class DX12ShaderFunction : public ShaderFunction,
                           public std::enable_shared_from_this<DX12ShaderFunction>
{
public:
    explicit DX12ShaderFunction(const DX12ContextPtr& context);
    ~DX12ShaderFunction() override = default;

    ShaderFunctionPtr InitWithShaderSource(const ShaderCode& shaderSource,
                                           ShaderStage shaderStage) override;

    std::shared_ptr<DX12ShaderFunction> InitInner(const ShaderCode& shaderSource,
                                                  ShaderStage shaderStage,
                                                  const ShaderStageData* metadata = nullptr);

    ShaderStage GetShaderStage() const override { return mStage; }

    bool IsValid() const { return !mBytecode.empty(); }

    D3D12_SHADER_BYTECODE GetBytecode() const
    {
        D3D12_SHADER_BYTECODE bytecode = {};
        bytecode.pShaderBytecode = mBytecode.empty() ? nullptr : mBytecode.data();
        bytecode.BytecodeLength = mBytecode.size();
        return bytecode;
    }

    const std::string& GetEntryName() const { return mEntryName; }
    const uint32_t* GetThreadGroupSize() const { return mThreadGroupSize; }

    const std::unordered_map<std::string, DX12BindInfo>& GetBindings() const { return mBindings; }
    const DX12BindInfo* FindBinding(const std::string& name) const;

    /// 按寄存器号反查绑定信息（用于按 index 绑定的入口，需要拿到 raw/步长等标志）
    const DX12BindInfo* FindBindingByRegister(DX12DescriptorClass cls, uint32_t bindPoint) const;

    /// 该阶段使用的最大寄存器号（无绑定返回 0）
    uint32_t GetMaxBindPoint(DX12DescriptorClass cls) const;

    /// 反射是否成功（失败时管线创建应直接报错而不是静默产出黑屏）
    bool IsReflectionValid() const { return mReflectionValid; }

    /// 顶点输入签名（按 input register 排序）
    const std::vector<DX12InputParam>& GetInputParams() const { return mInputParams; }

private:
    void Reflect(const ShaderStageData* metadata);

    DX12ContextPtr mContext;
    ShaderCode mBytecode;
    ShaderStage mStage = ShaderStage_Vertex;
    std::string mEntryName = "PS";
    uint32_t mThreadGroupSize[3] = { 1, 1, 1 };

    std::unordered_map<std::string, DX12BindInfo> mBindings;
    std::vector<DX12InputParam> mInputParams;
    uint32_t mMaxBindPoint[(size_t)DX12DescriptorClass::Count] = { 0, 0, 0, 0 };
    bool mReflectionValid = false;
};

using DX12ShaderFunctionPtr = std::shared_ptr<DX12ShaderFunction>;

// ============================================================================
// 图形着色器（VS+PS 或 AS(可选)+MS+PS）
// ============================================================================
class DX12GraphicsShader : public GraphicsShader
{
public:
    DX12GraphicsShader(const DX12ContextPtr& context,
                       const ShaderCode& vertexShader, const ShaderCode& fragmentShader);

    DX12GraphicsShader(const DX12ContextPtr& context,
                       const ShaderCode& taskShader, const ShaderCode& meshShader,
                       const ShaderCode& fragmentShader);
    DX12GraphicsShader(const DX12ContextPtr& context,
                       const ShaderStageData& vertexShader,
                       const ShaderStageData& fragmentShader);
    DX12GraphicsShader(const DX12ContextPtr& context,
                       const ShaderStageData& taskShader,
                       const ShaderStageData& meshShader,
                       const ShaderStageData& fragmentShader);

    ~DX12GraphicsShader() override = default;

    std::string GetName() const override { return mName; }

    bool IsMeshShader() const { return mMeshShader != nullptr && mMeshShader->IsValid(); }
    bool HasTaskShader() const { return mTaskShader != nullptr && mTaskShader->IsValid(); }

    const DX12ShaderFunctionPtr& GetVertexShader() const { return mVertexShader; }
    const DX12ShaderFunctionPtr& GetFragmentShader() const { return mFragmentShader; }
    const DX12ShaderFunctionPtr& GetMeshShader() const { return mMeshShader; }
    const DX12ShaderFunctionPtr& GetTaskShader() const { return mTaskShader; }

    /// 合并各阶段反射：名字 → 绑定信息
    /// （同名资源在不同阶段落到不同寄存器时只保留第一个阶段的结果，
    ///   因此 Mesh 管线必须用下面按 stage 查询的重载）
    const DX12BindInfo* FindBinding(const std::string& name) const;

    /**
     * @brief 按 stage 查询资源绑定
     *
     * 各 stage 的 HLSL 寄存器是独立分配的（同一个名字在 TS 与 MS 可能落在不同
     * 寄存器），所以 Mesh 管线的绑定必须逐 stage 解析，再写进对应的 stage 组
     * （见 DX12RenderDefine.h 的 stage 组说明）。
     */
    const DX12BindInfo* FindBinding(const std::string& name, ShaderStage stage) const;

    /// 描述符表宽度（所有阶段该类别最大寄存器号 + 1），并渲染出实际需要的偏移
    uint32_t GetTableWidth(DX12DescriptorClass cls) const;

    const std::unordered_map<std::string, DX12BindInfo>& GetMergedBindings() const
    {
        return mMergedBindings;
    }

private:
    void MergeBindings();

    DX12ContextPtr mContext;
    DX12ShaderFunctionPtr mVertexShader;
    DX12ShaderFunctionPtr mFragmentShader;
    DX12ShaderFunctionPtr mMeshShader;
    DX12ShaderFunctionPtr mTaskShader;

    std::unordered_map<std::string, DX12BindInfo> mMergedBindings;
    uint32_t mTableWidth[(size_t)DX12DescriptorClass::Count] = { 1, 1, 1, 1 };
    std::string mName;
};

using DX12GraphicsShaderPtr = std::shared_ptr<DX12GraphicsShader>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_SHADER_FUNCTION_INCLUDE_HGDS */
