//
//  DX12Pipeline.cpp
//  rendercore
//

#include "DX12Pipeline.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

NAMESPACE_RENDERCORE_BEGIN

namespace
{
D3D12_BLEND_DESC BuildBlendDesc(const GraphicsPipelineDesc& desc)
{
    D3D12_BLEND_DESC blend = {};
    blend.AlphaToCoverageEnable  = FALSE;

    const uint32_t count = (desc.renderTargetCount > 0) ? desc.renderTargetCount : 1;

    // 必须开启独立混合：引擎是按渲染目标逐个给出 ColorAttachmentDesc 的，
    // 而大气散射的 LUT 累积正依赖"只对目标 1 使用加法混合"（见
    // AtmosphereRenderer 中的 EnableAdditiveBlend(desc, 1)）。
    // 若为 FALSE，D3D12 会忽略目标 1..N 的设置、全部套用目标 0（覆盖式写入），
    // 于是 LUT 永远累积不起来：辐照度/散射 LUT 接近全零，天空整体偏暗，
    // 而且不产生任何 D3D12 校验错误。
    blend.IndependentBlendEnable = (count > 1) ? TRUE : FALSE;
    for (uint32_t i = 0; i < count && i < MAX_COLOR_ATTACHMENT_COUNT; ++i)
    {
        const ColorAttachmentDesc& src = desc.colorAttachmentDescriptors[i];
        D3D12_RENDER_TARGET_BLEND_DESC& dst = blend.RenderTarget[i];

        dst.BlendEnable           = src.blendingEnabled ? TRUE : FALSE;
        dst.LogicOpEnable         = FALSE;
        dst.SrcBlend              = DX12Util::ConvertBlendFactor(src.sourceRGBBlendFactor);
        dst.DestBlend             = DX12Util::ConvertBlendFactor(src.destinationRGBBlendFactor);
        dst.BlendOp               = DX12Util::ConvertBlendEquation(src.rgbBlendOperation);
        dst.SrcBlendAlpha         = DX12Util::ConvertBlendFactor(src.sourceAlphaBlendFactor);
        dst.DestBlendAlpha        = DX12Util::ConvertBlendFactor(src.destinationAplhaBlendFactor);
        dst.BlendOpAlpha          = DX12Util::ConvertBlendEquation(src.aplhaBlendOperation);
        dst.LogicOp               = D3D12_LOGIC_OP_NOOP;
        dst.RenderTargetWriteMask = DX12Util::ConvertColorWriteMask(src.writeMask);
    }

    return blend;
}

D3D12_RASTERIZER_DESC BuildRasterizerDesc(const GraphicsPipelineDesc& desc,
                                          const DX12PSOVariantKey& variant)
{
    D3D12_RASTERIZER_DESC raster = {};
    raster.FillMode              = DX12Util::ConvertFillMode(variant.fillMode);
    raster.CullMode              = DX12Util::ConvertCullMode(desc.cullMode);
    // 引擎资产与 Vulkan 后端都把逆时针定义为正面。D3D12 默认则是
    // 顺时针，若保持默认值，CullModeBack 会把所有可见几何当成背面剔除。
    raster.FrontCounterClockwise = TRUE;
    // 深度偏移：D3D12 是 PSO 静态状态，所以纳入变体键（见头文件说明）
    raster.DepthBias             = (INT)variant.depthBiasUnits;
    raster.DepthBiasClamp        = 0.0f;
    raster.SlopeScaledDepthBias  = variant.depthBiasFactor;
    raster.DepthClipEnable       = TRUE;
    raster.MultisampleEnable     = FALSE;
    raster.AntialiasedLineEnable = FALSE;
    raster.ForcedSampleCount     = 0;
    raster.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return raster;
}

D3D12_DEPTH_STENCIL_DESC BuildDepthStencilDesc(const GraphicsPipelineDesc& desc)
{
    const DepthStencilDesc& src = desc.depthStencilDescriptor;
    const StencilDesc& stencilSrc = src.stencil;

    D3D12_DEPTH_STENCIL_DESC ds = {};
    // 与 Vulkan 后端判断一致：compare == Always 视为关闭深度测试
    ds.DepthEnable    = (src.depthCompareFunction != CompareFunctionAlways) ? TRUE : FALSE;
    ds.DepthWriteMask = src.depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL
                                              : D3D12_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc      = DX12Util::ConvertCompareFunction(src.depthCompareFunction);

    ds.StencilEnable    = stencilSrc.stencilEnable ? TRUE : FALSE;
    ds.StencilReadMask  = (UINT8)stencilSrc.readMask;
    ds.StencilWriteMask = (UINT8)stencilSrc.writeMask;

    // D3D12 的 FrontFace 对应“正面”，引擎未区分正反面模板，两侧保持一致
    ds.FrontFace.StencilFailOp      = DX12Util::ConvertStencilOperation(stencilSrc.stencilFailureOperation);
    ds.FrontFace.StencilDepthFailOp = DX12Util::ConvertStencilOperation(stencilSrc.depthFailureOperation);
    ds.FrontFace.StencilPassOp      = DX12Util::ConvertStencilOperation(stencilSrc.depthStencilPassOperation);
    ds.FrontFace.StencilFunc        = DX12Util::ConvertCompareFunction(stencilSrc.stencilCompareFunction);

    ds.BackFace = ds.FrontFace;

    return ds;
}

/// 合并 depth / stencil 格式（D3D12 只有一个 DSVFormat 字段）
DXGI_FORMAT ResolveDepthStencilFormat(const DX12RenderPassFormat& format)
{
    if (format.stencilFormat != DXGI_FORMAT_UNKNOWN)
    {
        return format.stencilFormat;
    }
    return format.depthFormat;
}
} // namespace

// ============================================================================
// DX12GraphicsPipeline
// ============================================================================

DX12GraphicsPipeline::DX12GraphicsPipeline(const DX12ContextPtr& context,
                                           DX12RootSignature* rootSignature,
                                           const GraphicsPipelineDesc& des)
    : GraphicsPipeline(des)
    , mContext(context)
    , mRootSignature(rootSignature)
{
}

void DX12GraphicsPipeline::AttachVertexShader(ShaderFunctionPtr shaderFunction)
{
    mShader = std::dynamic_pointer_cast<DX12GraphicsShader>(shaderFunction);
}

void DX12GraphicsPipeline::AttachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    mShader = std::dynamic_pointer_cast<DX12GraphicsShader>(shaderFunction);
}

void DX12GraphicsPipeline::AttachGraphicsShader(GraphicsShaderPtr graphicsShader)
{
    mShader = std::dynamic_pointer_cast<DX12GraphicsShader>(graphicsShader);
    if (!mShader)
    {
        LOG_ERROR("[DX12] AttachGraphicsShader received a non-DX12 GraphicsShader");
        return;
    }

    // 与 Vulkan 后端一致：Mesh / Task 的线程组大小由管线对外暴露
    const uint32_t* meshSize = mShader->IsMeshShader() ? mShader->GetMeshShader()->GetThreadGroupSize() : nullptr;
    if (meshSize)
    {
        mMeshThreadgroupSize[0] = meshSize[0];
        mMeshThreadgroupSize[1] = meshSize[1];
        mMeshThreadgroupSize[2] = meshSize[2];
    }

    const uint32_t* taskSize = mShader->HasTaskShader() ? mShader->GetTaskShader()->GetThreadGroupSize() : nullptr;
    if (taskSize)
    {
        mTaskThreadgroupSize[0] = taskSize[0];
        mTaskThreadgroupSize[1] = taskSize[1];
        mTaskThreadgroupSize[2] = taskSize[2];
    }
}

void DX12GraphicsPipeline::AttachTaskShader(ShaderFunctionPtr shaderFunction)
{
    // 入口已在 AttachGraphicsShader 中统一处理
    (void)shaderFunction;
}

void DX12GraphicsPipeline::AttachMeshShader(ShaderFunctionPtr shaderFunction)
{
    (void)shaderFunction;
}

uint32_t DX12GraphicsPipeline::GetTableWidth(DX12DescriptorClass cls) const
{
    return mShader ? mShader->GetTableWidth(cls) : 1;
}

const DX12BindInfo* DX12GraphicsPipeline::FindBinding(const std::string& name) const
{
    return mShader ? mShader->FindBinding(name) : nullptr;
}

const DX12BindInfo* DX12GraphicsPipeline::FindBinding(const std::string& name,
                                                     ShaderStage stage) const
{
    return mShader ? mShader->FindBinding(name, stage) : nullptr;
}

void DX12GraphicsPipeline::BuildInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& outElements,
                                            std::vector<std::string>& outSemanticNames) const
{
    outElements.clear();
    outSemanticNames.clear();

    const VertexDesc& vertexDesc = mDesc.vertexDescriptor;
    if (vertexDesc.attributes.empty())
    {
        return;
    }

    // 语义名从顶点着色器的输入签名取（按 input register 映射），避免硬编码
    const std::vector<DX12InputParam>* inputParams = nullptr;
    if (mShader && mShader->GetVertexShader())
    {
        inputParams = &mShader->GetVertexShader()->GetInputParams();
    }

    outSemanticNames.reserve(vertexDesc.attributes.size());

    for (const VertextAttributesDesc& attr : vertexDesc.attributes)
    {
        std::string semanticName = "TEXCOORD";
        uint32_t semanticIndex = attr.index;

        if (inputParams != nullptr)
        {
            const DX12InputParam* matched = nullptr;
            for (const DX12InputParam& param : *inputParams)
            {
                if (param.registerIndex == attr.index)
                {
                    matched = &param;
                    break;
                }
            }
            if (matched != nullptr)
            {
                semanticName  = matched->semanticName;
                semanticIndex = matched->semanticIndex;
            }
            else
            {
                LOG_WARN("[DX12] Vertex attribute at input register %u has no matching shader input signature; "
                         "falling back to %s%u", attr.index, semanticName.c_str(), semanticIndex);
            }
        }

        // 语义名字符串必须保活到 CreateGraphicsPipelineState 调用结束
        outSemanticNames.push_back(semanticName);

        D3D12_INPUT_ELEMENT_DESC element = {};
        element.SemanticName         = outSemanticNames.back().c_str();
        element.SemanticIndex        = semanticIndex;
        element.Format               = DX12Util::ConvertVertexFormat(attr.format);
        element.InputSlot            = attr.index;
        element.AlignedByteOffset    = attr.offset;
        element.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element.InstanceDataStepRate = 0;

        outElements.push_back(element);
    }
}

uint32_t DX12GraphicsPipeline::GetVertexStride(uint32_t slot) const
{
    const VertexDesc& vertexDesc = mDesc.vertexDescriptor;

    // 优先使用 VertexDesc.layouts 中声明的 stride（该数组按 slot / buffer 索引）
    if (slot < vertexDesc.layouts.size() && vertexDesc.layouts[slot].stride != 0)
    {
        return vertexDesc.layouts[slot].stride;
    }

    // 兜底：平面布局下，slot 的 stride 就是该 slot 内属性格式的元素大小。
    // 与 Vulkan 后端的做法一致（VKGraphicsPipeline.cpp 中
    // binding.stride = VulkanBufferUtil::GetVertexFormatSize(attr.format)）。
    for (const VertextAttributesDesc& attr : vertexDesc.attributes)
    {
        if (attr.index == slot)
        {
            return DX12Util::GetVertexFormatSize(attr.format);
        }
    }

    return 0;
}

ID3D12PipelineState* DX12GraphicsPipeline::GetPipelineState(const DX12RenderPassFormat& format,
                                                            const DX12PSOVariantKey& variant)
{
    VariantKey key;
    key.format = format;
    key.state  = variant;

    auto iter = mVariants.find(key);
    if (iter != mVariants.end())
    {
        return iter->second.Get();
    }

    if (mFailedVariants.find(key) != mFailedVariants.end())
    {
        return nullptr;
    }

    ID3D12PipelineState* pso = CreatePipelineState(format, variant);
    if (pso == nullptr)
    {
        mFailedVariants[key] = true;
        return nullptr;
    }

    mVariants[key] = pso;
    return mVariants[key].Get();
}

ID3D12PipelineState* DX12GraphicsPipeline::CreatePipelineState(const DX12RenderPassFormat& format,
                                                               const DX12PSOVariantKey& variant)
{
    if (!mShader)
    {
        LOG_ERROR("[DX12] CreatePipelineState: no graphics shader attached");
        return nullptr;
    }
    if (mRootSignature == nullptr)
    {
        LOG_ERROR("[DX12] CreatePipelineState: root signature is null");
        return nullptr;
    }

    const bool isMesh = IsMeshPipeline();
    ID3D12RootSignature* rootSig = isMesh ? mRootSignature->GetMeshRootSignature()
                                          : mRootSignature->GetGraphicsRootSignature();
    if (rootSig == nullptr)
    {
        LOG_ERROR("[DX12] CreatePipelineState: %s root signature unavailable",
                  isMesh ? "mesh" : "graphics");
        return nullptr;
    }

    const D3D12_BLEND_DESC blendDesc = BuildBlendDesc(mDesc);
    const D3D12_RASTERIZER_DESC rasterDesc = BuildRasterizerDesc(mDesc, variant);
    const D3D12_DEPTH_STENCIL_DESC depthStencilDesc = BuildDepthStencilDesc(mDesc);
    const DXGI_FORMAT dsFormat = ResolveDepthStencilFormat(format);

    ComPtr<ID3D12PipelineState> pso;

    if (!isMesh)
    {
        // ---- 传统 VS + PS 管线 ----
        std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
        std::vector<std::string> semanticNames;
        BuildInputLayout(elements, semanticNames);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature        = rootSig;
        desc.VS                    = mShader->GetVertexShader() ? mShader->GetVertexShader()->GetBytecode()
                                                                : D3D12_SHADER_BYTECODE{};
        desc.PS                    = mShader->GetFragmentShader() ? mShader->GetFragmentShader()->GetBytecode()
                                                                  : D3D12_SHADER_BYTECODE{};
        desc.BlendState            = blendDesc;
        desc.SampleMask            = UINT_MAX;
        desc.RasterizerState       = rasterDesc;
        desc.DepthStencilState     = depthStencilDesc;
        desc.InputLayout           = { elements.empty() ? nullptr : elements.data(), (UINT)elements.size() };
        desc.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets      = (UINT)format.colorFormats.size();
        desc.DSVFormat             = dsFormat;
        desc.SampleDesc.Count      = format.sampleCount;
        desc.SampleDesc.Quality    = 0;
        desc.NodeMask              = 0;
        desc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

        for (size_t i = 0; i < format.colorFormats.size() && i < 8; ++i)
        {
            desc.RTVFormats[i] = format.colorFormats[i];
        }

        const HRESULT hr = mContext->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        if (FAILED(hr))
        {
            LOG_ERROR("[DX12] CreateGraphicsPipelineState failed: %s (rtCount=%u, dsFormat=%d, "
                      "vs=%d, ps=%d, inputElements=%u)",
                      DX12HResultToString(hr), (unsigned)format.colorFormats.size(), (int)dsFormat,
                      desc.VS.pShaderBytecode ? 1 : 0, desc.PS.pShaderBytecode ? 1 : 0,
                      (unsigned)elements.size());
            return nullptr;
        }
    }
    else
    {
        // ---- Mesh 管线（AS 可选 + MS + PS）：必须使用流水线状态流 ----
        ComPtr<ID3D12Device2> device2;
        if (FAILED(mContext->device.As(&device2)) || !device2)
        {
            LOG_ERROR("[DX12] ID3D12Device2 is unavailable; mesh pipelines require Windows 10 1703+");
            return nullptr;
        }

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC meshDesc = {};
        meshDesc.pRootSignature = rootSig;
        if (mShader->HasTaskShader())
        {
            meshDesc.AS = mShader->GetTaskShader()->GetBytecode();
        }
        meshDesc.MS = mShader->GetMeshShader()->GetBytecode();
        if (mShader->GetFragmentShader())
        {
            meshDesc.PS = mShader->GetFragmentShader()->GetBytecode();
        }
        meshDesc.BlendState = blendDesc;
        meshDesc.SampleMask = UINT_MAX;
        meshDesc.RasterizerState = rasterDesc;
        meshDesc.DepthStencilState = depthStencilDesc;
        meshDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        meshDesc.NumRenderTargets = (UINT)format.colorFormats.size();
        for (size_t i = 0; i < format.colorFormats.size() && i < 8; ++i)
        {
            meshDesc.RTVFormats[i] = format.colorFormats[i];
        }
        meshDesc.DSVFormat = dsFormat;
        meshDesc.SampleDesc = {format.sampleCount, 0};
        meshDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        const CD3DX12_PIPELINE_STATE_STREAM2 pipelineStream(meshDesc);
        const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {
            sizeof(pipelineStream), const_cast<CD3DX12_PIPELINE_STATE_STREAM2*>(&pipelineStream)};
        const HRESULT hr = device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr))
        {
            LOG_ERROR("[DX12] Mesh CreatePipelineState failed: %s (hasTask=%d, dsFormat=%d)",
                      DX12HResultToString(hr), mShader->HasTaskShader() ? 1 : 0, (int)dsFormat);
            return nullptr;
        }
    }

    return pso.Detach();
}

// ============================================================================
// DX12ComputePipeline
// ============================================================================

DX12ComputePipeline::DX12ComputePipeline(const DX12ContextPtr& context,
                                         DX12RootSignature* rootSignature,
                                         const ShaderCode& shaderCode)
    : ComputePipeline(nullptr)
    , mContext(context)
    , mRootSignature(rootSignature)
{
    auto shader = std::make_shared<DX12ShaderFunction>(context);
    mShader = shader->InitInner(shaderCode, ShaderStage_Compute);

    if (!mShader || !mShader->IsValid())
    {
        LOG_ERROR("[DX12] DX12ComputePipeline: failed to create compute shader");
    }
}

DX12ComputePipeline::DX12ComputePipeline(const DX12ContextPtr& context,
                                         DX12RootSignature* rootSignature,
                                         const ShaderStageData& shaderData)
    : ComputePipeline(nullptr)
    , mContext(context)
    , mRootSignature(rootSignature)
{
    auto shader = std::make_shared<DX12ShaderFunction>(context);
    mShader = shader->InitInner(shaderData.sourceData, ShaderStage_Compute, &shaderData);
    if (!mShader || !mShader->IsValid())
        LOG_ERROR("[DX12] DX12ComputePipeline: failed to create compute shader");
}

void DX12ComputePipeline::GetThreadGroupSizes(uint32_t& x, uint32_t& y, uint32_t& z)
{
    if (mShader)
    {
        const uint32_t* size = mShader->GetThreadGroupSize();
        x = size[0];
        y = size[1];
        z = size[2];
        return;
    }
    x = 1;
    y = 1;
    z = 1;
}

uint32_t DX12ComputePipeline::GetTableWidth(DX12DescriptorClass cls) const
{
    if (!mShader)
    {
        return 1;
    }

    // 与 DX12GraphicsShader::GetTableWidth 同理：表宽必须以绑定表为权威来源。
    // GetMaxBindPoint 与绑定表曾出现不一致（绑定表含 UAV u1，而表宽仍为 1），
    // 导致绑定写入越界、GetCpuHandle 返回空句柄、Create*View 报 id=646。
    uint32_t width = mShader->GetMaxBindPoint(cls) + 1;

    for (const auto& kv : mShader->GetBindings())
    {
        if (kv.second.cls != cls)
        {
            continue;
        }
        const uint32_t needed = kv.second.bindPoint + kv.second.bindCount;
        if (needed > width)
        {
            width = needed;
        }
    }

    const uint32_t limit = DX12TableWidthLimit(cls);
    return (width > limit) ? limit : width;
}

const DX12BindInfo* DX12ComputePipeline::FindBinding(const std::string& name) const
{
    return mShader ? mShader->FindBinding(name) : nullptr;
}

bool DX12ComputePipeline::EnsurePipelineState()
{
    if (mPipelineState)
    {
        return true;
    }
    if (mCreationAttempted)
    {
        return false;
    }
    mCreationAttempted = true;

    if (!mShader || !mShader->IsValid() || mRootSignature == nullptr)
    {
        LOG_ERROR("[DX12] Compute pipeline create skipped (shader=%d, rootSignature=%d)",
                  mShader ? 1 : 0, mRootSignature ? 1 : 0);
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = mRootSignature->GetComputeRootSignature();
    desc.CS             = mShader->GetBytecode();
    desc.NodeMask       = 0;
    desc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;

    const HRESULT hr = mContext->device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPipelineState));
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] CreateComputePipelineState failed: %s", DX12HResultToString(hr));
        return false;
    }

    return true;
}

ID3D12PipelineState* DX12ComputePipeline::GetPipelineState()
{
    return EnsurePipelineState() ? mPipelineState.Get() : nullptr;
}

NAMESPACE_RENDERCORE_END
