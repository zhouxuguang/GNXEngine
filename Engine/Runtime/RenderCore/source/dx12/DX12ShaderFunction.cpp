//
//  DX12ShaderFunction.cpp
//  rendercore
//

#include "DX12ShaderFunction.h"

#include <algorithm>

NAMESPACE_RENDERCORE_BEGIN

const char* DX12GetEntryPointName(ShaderStage stage)
{
    // DXIL preserves the engine's VS/PS/CS/TS/MS entry-point names.
    //
    // 注意：D3D12 创建图形/计算 PSO 时并不需要指定入口点名（DXIL 里自带），
    // 这里只用于日志与调试标识。
    switch (stage)
    {
        case ShaderStage_Vertex:   return "VS";
        case ShaderStage_Fragment: return "PS";
        case ShaderStage_Compute:  return "CS";
        case ShaderStage_Task:     return "TS";
        case ShaderStage_Mesh:     return "MS";
        default:                   return "PS";
    }
}

namespace
{
bool IsDXILContainer(const ShaderCode& bytecode)
{
    constexpr uint32_t kDxbc = 0x43425844u;
    constexpr uint32_t kDxil = 0x4C495844u;
    if (bytecode.size() < 32)
        return false;

    uint32_t magic = 0;
    uint32_t partCount = 0;
    memcpy(&magic, bytecode.data(), sizeof(magic));
    memcpy(&partCount, bytecode.data() + 28, sizeof(partCount));
    if (magic != kDxbc || partCount > (bytecode.size() - 32) / sizeof(uint32_t))
        return false;

    for (uint32_t i = 0; i < partCount; ++i)
    {
        uint32_t partOffset = 0;
        memcpy(&partOffset, bytecode.data() + 32 + i * sizeof(uint32_t), sizeof(partOffset));
        if (partOffset <= bytecode.size() - sizeof(uint32_t))
        {
            uint32_t partFourCC = 0;
            memcpy(&partFourCC, bytecode.data() + partOffset, sizeof(partFourCC));
            if (partFourCC == kDxil)
                return true;
        }
    }
    return false;
}

} // namespace

// ============================================================================
// DX12ShaderFunction
// ============================================================================

DX12ShaderFunction::DX12ShaderFunction(const DX12ContextPtr& context)
    : mContext(context)
{
}

ShaderFunctionPtr DX12ShaderFunction::InitWithShaderSource(const ShaderCode& shaderSource,
                                                           ShaderStage shaderStage)
{
    return InitInner(shaderSource, shaderStage);
}

std::shared_ptr<DX12ShaderFunction> DX12ShaderFunction::InitInner(const ShaderCode& shaderSource,
                                                                  ShaderStage shaderStage,
                                                                  const ShaderStageData* metadata)
{
    mStage = shaderStage;
    mEntryName = DX12GetEntryPointName(shaderStage);

    if (shaderSource.empty())
    {
        LOG_ERROR("[DX12] Shader stage %d has empty bytecode", (int)shaderStage);
        return shared_from_this();
    }

    if (!IsDXILContainer(shaderSource))
    {
        LOG_ERROR("[DX12] Shader stage %d rejected: only DXIL containers are supported",
                  (int)shaderStage);
        return shared_from_this();
    }

    // DXIL blob 是字节码，复制一份以便与调用方解耦
    mBytecode = shaderSource;

    Reflect(metadata);

    return shared_from_this();
}

void DX12ShaderFunction::Reflect(const ShaderStageData* metadata)
{
    if (!metadata)
    {
        LOG_ERROR("[DX12] Shader stage %d has no offline reflection metadata", (int)mStage);
        return;
    }

    for (const auto& resource : metadata->resources)
    {
        DX12BindInfo info;
        info.cls = (DX12DescriptorClass)resource.resourceClass;
        info.bindPoint = resource.binding;
        info.bindCount = resource.bindCount;
        info.srvDimension = (D3D12_SRV_DIMENSION)resource.dimension;
        info.stage = mStage;
        info.isRawBuffer = resource.isRawBuffer;
        info.structuredStride = resource.structuredStride;
        mBindings[resource.name] = info;
        if (resource.name.size() > 5 && resource.name.compare(0, 5, "type_") == 0)
            mBindings[resource.name.substr(5)] = info;
        const uint32_t index = (uint32_t)info.cls;
        if (index < (uint32_t)DX12DescriptorClass::Count)
            mMaxBindPoint[index] = std::max(mMaxBindPoint[index], info.bindPoint);
    }

    for (const auto& input : metadata->inputs)
        mInputParams.push_back({input.semanticName, input.semanticIndex, input.registerIndex});
    std::sort(mInputParams.begin(), mInputParams.end(),
              [](const DX12InputParam& a, const DX12InputParam& b) {
                  return a.registerIndex < b.registerIndex;
              });

    mThreadGroupSize[0] = metadata->threadgroupSizeX ? metadata->threadgroupSizeX : 1;
    mThreadGroupSize[1] = metadata->threadgroupSizeY ? metadata->threadgroupSizeY : 1;
    mThreadGroupSize[2] = metadata->threadgroupSizeZ ? metadata->threadgroupSizeZ : 1;
    mReflectionValid = true;
}


const DX12BindInfo* DX12ShaderFunction::FindBinding(const std::string& name) const
{
    auto iter = mBindings.find(name);
    return (iter != mBindings.end()) ? &iter->second : nullptr;
}

const DX12BindInfo* DX12ShaderFunction::FindBindingByRegister(DX12DescriptorClass cls,
                                                              uint32_t bindPoint) const
{
    for (const auto& kv : mBindings)
    {
        if (kv.second.cls == cls && kv.second.bindPoint == bindPoint)
        {
            return &kv.second;
        }
    }
    return nullptr;
}

uint32_t DX12ShaderFunction::GetMaxBindPoint(DX12DescriptorClass cls) const
{
    const uint32_t index = (uint32_t)cls;
    return (index < (uint32_t)DX12DescriptorClass::Count) ? mMaxBindPoint[index] : 0;
}

// ============================================================================
// DX12GraphicsShader
// ============================================================================

DX12GraphicsShader::DX12GraphicsShader(const DX12ContextPtr& context,
                                       const ShaderCode& vertexShader,
                                       const ShaderCode& fragmentShader)
    : mContext(context)
{
    mName = "DX12GraphicsShader(VS+PS)";

    if (!vertexShader.empty())
    {
        auto shader = std::make_shared<DX12ShaderFunction>(context);
        mVertexShader = shader->InitInner(vertexShader, ShaderStage_Vertex);
    }
    if (!fragmentShader.empty())
    {
        auto shader = std::make_shared<DX12ShaderFunction>(context);
        mFragmentShader = shader->InitInner(fragmentShader, ShaderStage_Fragment);
    }

    mName += " vs=" + std::to_string(vertexShader.size()) +
             " ps=" + std::to_string(fragmentShader.size());

    MergeBindings();
}

DX12GraphicsShader::DX12GraphicsShader(const DX12ContextPtr& context,
                                       const ShaderCode& taskShader,
                                       const ShaderCode& meshShader,
                                       const ShaderCode& fragmentShader)
    : mContext(context)
{
    mName = "DX12GraphicsShader(AS+MS+PS)";

    if (!taskShader.empty())
    {
        auto shader = std::make_shared<DX12ShaderFunction>(context);
        mTaskShader = shader->InitInner(taskShader, ShaderStage_Task);
    }
    if (!meshShader.empty())
    {
        auto shader = std::make_shared<DX12ShaderFunction>(context);
        mMeshShader = shader->InitInner(meshShader, ShaderStage_Mesh);
    }
    if (!fragmentShader.empty())
    {
        auto shader = std::make_shared<DX12ShaderFunction>(context);
        mFragmentShader = shader->InitInner(fragmentShader, ShaderStage_Fragment);
    }

    MergeBindings();
}

DX12GraphicsShader::DX12GraphicsShader(const DX12ContextPtr& context,
                                       const ShaderStageData& vertexShader,
                                       const ShaderStageData& fragmentShader)
    : mContext(context)
{
    mName = "DX12GraphicsShader(VS+PS)";
    if (!vertexShader.sourceData.empty())
        mVertexShader = std::make_shared<DX12ShaderFunction>(context)->InitInner(
            vertexShader.sourceData, ShaderStage_Vertex, &vertexShader);
    if (!fragmentShader.sourceData.empty())
        mFragmentShader = std::make_shared<DX12ShaderFunction>(context)->InitInner(
            fragmentShader.sourceData, ShaderStage_Fragment, &fragmentShader);
    MergeBindings();
}

DX12GraphicsShader::DX12GraphicsShader(const DX12ContextPtr& context,
                                       const ShaderStageData& taskShader,
                                       const ShaderStageData& meshShader,
                                       const ShaderStageData& fragmentShader)
    : mContext(context)
{
    mName = "DX12GraphicsShader(AS+MS+PS)";
    if (!taskShader.sourceData.empty())
        mTaskShader = std::make_shared<DX12ShaderFunction>(context)->InitInner(
            taskShader.sourceData, ShaderStage_Task, &taskShader);
    if (!meshShader.sourceData.empty())
        mMeshShader = std::make_shared<DX12ShaderFunction>(context)->InitInner(
            meshShader.sourceData, ShaderStage_Mesh, &meshShader);
    if (!fragmentShader.sourceData.empty())
        mFragmentShader = std::make_shared<DX12ShaderFunction>(context)->InitInner(
            fragmentShader.sourceData, ShaderStage_Fragment, &fragmentShader);
    MergeBindings();
}

void DX12GraphicsShader::MergeBindings()
{
    // 必须同时清空合并结果与表宽，二者是一起重建的。
    //
    // 这里曾在只重置 mTableWidth、不清 mMergedBindings 的情况下重建：
    // MergeBindings 在每个 stage setter 里都会被调用，第二次调用时上一轮已注册的名字
    // 会命中下面的 "已存在则 continue" 分支，于是它们不再贡献表宽——
    // 结果是 mMergedBindings 保有全部绑定，而 mTableWidth 只反映最后一次调用里
    // 新出现的名字，两者系统性不一致。
    //
    // 表宽偏小的后果是绑定越界写：GetCpuHandle 以 registerIndex >= width 判非法并返回
    // 空句柄，最终在 CreateUnorderedAccessView 处报 id=646（handle ptr=0），
    // 并进一步触发设备移除。nanite 崩溃即由此而来（mMergedBindings 有 u1，而表宽计算为 1）。
    for (uint32_t i = 0; i < (uint32_t)DX12DescriptorClass::Count; ++i)
    {
        mTableWidth[i] = 1;
    }
    mMergedBindings.clear();

    std::vector<DX12ShaderFunctionPtr> stages;
    if (mVertexShader)   stages.push_back(mVertexShader);
    if (mFragmentShader) stages.push_back(mFragmentShader);
    if (mTaskShader)     stages.push_back(mTaskShader);
    if (mMeshShader)     stages.push_back(mMeshShader);

    // 用于检测“同一寄存器被不同名字占用”的冲突（跨阶段）
    std::map<std::pair<uint32_t, uint32_t>, std::string> registerOwner;

    for (const auto& shader : stages)
    {
        if (!shader || !shader->IsValid())
        {
            continue;
        }

        for (const auto& kv : shader->GetBindings())
        {
            const std::string& name = kv.first;
            const DX12BindInfo& info = kv.second;

            // 表宽必须取**所有 stage 各自寄存器号**的最大值，而不是合并结果的最大值。
            //
            // 合并表按"第一个出现的 stage 优先"保留寄存器号，在 Mesh 管线上这会丢掉
            // 后出现的 stage 里更大的寄存器号：例如 MeshletDemo 的 Instances 在 TS 是
            // t1、在 MS 是 t5，合并表只留下 t1，于是 SRV 表宽算成 5，
            // 而 MS 实际要写 t5 → 绑定越界（空句柄 → id=646 → 设备移除）。
            // 分组后每一组都要容纳该 stage 的最大寄存器号，因此这里必须按各 stage 独立统计。
            const uint32_t classIndex = (uint32_t)info.cls;
            if (classIndex < (uint32_t)DX12DescriptorClass::Count)
            {
                const uint32_t needed = info.bindPoint + info.bindCount;
                if (needed > mTableWidth[classIndex])
                {
                    mTableWidth[classIndex] = needed;
                }
            }

            auto iter = mMergedBindings.find(name);
            if (iter != mMergedBindings.end())
            {
                // 同名资源在不同阶段解析到不同寄存器：这正是隐式寄存器分配最危险的情形。
                // 普通图形管线（VS+PS）只有一组 SHADER_VISIBILITY_ALL 的表，
                // 无法同时满足两个阶段，因此在管线创建阶段就明确报错。
                // （Mesh 管线有三组表，各 stage 按自己的寄存器解读，不存在该限制。）
                if (iter->second.bindPoint != info.bindPoint ||
                    iter->second.cls != info.cls)
                {
                    LOG_ERROR("[DX12] Register conflict for '%s': stage %d uses (class=%d, reg=%u) "
                              "but stage %d uses (class=%d, reg=%u). "
                              "普通图形管线（VS+PS）的寄存器在各阶段必须一致，"
                              "请在 HLSL 中为该资源显式指定 register()。",
                              name.c_str(),
                              (int)iter->second.stage, (int)iter->second.cls, iter->second.bindPoint,
                              (int)info.stage, (int)info.cls, info.bindPoint);
                }
                continue;
            }

            const auto key = std::make_pair((uint32_t)info.cls, info.bindPoint);
            auto ownerIter = registerOwner.find(key);
            if (ownerIter != registerOwner.end() && ownerIter->second != name)
            {
                const auto existing = mMergedBindings.find(ownerIter->second);
                const bool sameStageAlias = existing != mMergedBindings.end() &&
                                            existing->second.stage == info.stage;
                if (!sameStageAlias)
                {
                    LOG_ERROR("[DX12] Register aliasing: (class=%d, reg=%u) is bound to both '%s' and '%s'. "
                              "两个不同的资源不能占用同一寄存器。",
                              (int)info.cls, info.bindPoint, ownerIter->second.c_str(), name.c_str());
                }
            }
            registerOwner[key] = name;

            mMergedBindings[name] = info;
        }
    }

}

const DX12BindInfo* DX12GraphicsShader::FindBinding(const std::string& name) const
{
    auto iter = mMergedBindings.find(name);
    return (iter != mMergedBindings.end()) ? &iter->second : nullptr;
}

const DX12BindInfo* DX12GraphicsShader::FindBinding(const std::string& name,
                                                    ShaderStage stage) const
{
    DX12ShaderFunctionPtr shader;
    switch (stage)
    {
        case ShaderStage_Task:     shader = mTaskShader;     break;
        case ShaderStage_Mesh:     shader = mMeshShader;     break;
        case ShaderStage_Fragment: shader = mFragmentShader; break;
        case ShaderStage_Vertex:   shader = mVertexShader;   break;
        default:                                             break;
    }

    if (!shader || !shader->IsValid())
    {
        return nullptr;
    }
    return shader->FindBinding(name);
}

uint32_t DX12GraphicsShader::GetTableWidth(DX12DescriptorClass cls) const
{
    const uint32_t index = (uint32_t)cls;
    if (index >= (uint32_t)DX12DescriptorClass::Count)
    {
        return 1;
    }

    // 不高于根签名声明的表宽上限。
    //
    // mTableWidth 由 UpdateTableWidths 遍历本类的 stage 合并得到（graphics shader
    // 的 stage 恰好就是 VS/PS/TS/MS，覆盖完整）。计算着色器不在此容器内，
    // 其表宽由 DX12ComputePipeline::GetTableWidth 单独计算。
    const uint32_t width = (mTableWidth[index] == 0) ? 1 : mTableWidth[index];
    const uint32_t limit = DX12TableWidthLimit(cls);
    return (width > limit) ? limit : width;
}

NAMESPACE_RENDERCORE_END
