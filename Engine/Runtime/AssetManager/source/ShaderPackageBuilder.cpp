//
//  ShaderPackageBuilder.cpp
//  GNXEngine
//
//  公开 shader 容器构建 API 的实现。
//  内部将 ShaderStageData（RenderCore 值类型）转换为 pb ShaderMessage，
//  组装 ShaderPackageMessage 容器，序列化并写入带 AssetFileHeader 的 .gnxasset。
//  pb / nanopb 仅在此 .cpp 内使用。
//

#include "ShaderPackageBuilder.h"
#include "ShaderMessageUtil.h"
#include "ShaderMessage.pb.h"
#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/HashFunction.h"
#include "PBUtils.h"

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

NS_ASSETMANAGER_BEGIN

ShaderPackageBuilder::ShaderPackageBuilder() = default;
ShaderPackageBuilder::~ShaderPackageBuilder() = default;

void ShaderPackageBuilder::AddStage(RenderCore::ShaderStage stage,
                                    const std::string& entryPoint,
                                    const RenderCore::ShaderCode& sourceData,
                                    const RenderCore::VertexDesc& vertexDesc,
                                    uint32_t threadgroupSizeX,
                                    uint32_t threadgroupSizeY,
                                    uint32_t threadgroupSizeZ,
                                    const std::vector<RenderCore::CompiledPushConstantInfo>& pushConstants,
                                    uint64_t sourceHash)
{
    StageEntry entry;
    entry.stage = stage;
    entry.entryPoint = entryPoint;
    entry.sourceData = sourceData;
    entry.vertexDesc = vertexDesc;
    entry.threadgroupSizeX = threadgroupSizeX;
    entry.threadgroupSizeY = threadgroupSizeY;
    entry.threadgroupSizeZ = threadgroupSizeZ;
    entry.pushConstants = pushConstants;
    entry.sourceHash = sourceHash;
    mStages.push_back(std::move(entry));
}

// ====================================================================
// 内部辅助：ShaderStageData / StageEntry → pb ShaderMessage
// ====================================================================

// 从 RenderCore::VertexDesc 构造 proto vertexInputs
static void FillVertexInputsFromVertexDesc(const RenderCore::VertexDesc& desc,
                                           std::vector<VertexInputMessage>& out)
{
    out.clear();
    out.reserve(desc.attributes.size());

    for (size_t i = 0; i < desc.attributes.size(); ++i)
    {
        const auto& attr = desc.attributes[i];

        VertexInputMessage vi = VertexInputMessage_init_default;

        // semantic 名留空（Metal 不需要精确语义名，location 是关键）
        vi.location = attr.index;
        vi.format   = static_cast<::VertexDataFormat>(static_cast<uint32_t>(attr.format));
        vi.offset   = attr.offset;

        // 该 attribute 对应 layout 的 stride
        vi.stride = (i < desc.layouts.size()) ? desc.layouts[i].stride : 0;

        out.push_back(vi);
    }
}

// 从 stage 反射数据填充 encode data（反射 repeated 字段）
static void FillEncodeDataFromStage(const RenderCore::VertexDesc& vertexDesc,
                                    const std::vector<RenderCore::CompiledPushConstantInfo>& pushConstants,
                                    ShaderMessageEncodeData& out)
{
    // vertexInputs（由 vertexDescriptor 转换，Metal 重建 VertexDesc 必需）
    if (vertexDesc.attributes.size() > 0)
    {
        FillVertexInputsFromVertexDesc(vertexDesc, out.vertexInputs);
    }

    // pushConstants
    out.pushConstants.clear();
    for (const auto& pc : pushConstants)
    {
        PushConstantMessage pcm = PushConstantMessage_init_default;
        pcm.size    = pc.size;
        pcm.set     = pc.set;
        pcm.binding = pc.binding;
        out.pushConstants.push_back(pcm);
    }
}

// 字符串编码回调（entryPoint 字段）
static bool shader_string_encode_callback(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    const std::string* str = static_cast<const std::string*>(*arg);
    if (!str || str->empty())
    {
        return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)"", 0);
    }
    return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)str->c_str(), str->length());
}

bool ShaderPackageBuilder::Save(const std::string& outputPath)
{
    if (mStages.empty())
    {
        LOG_ERROR("ShaderPackageBuilder: no stages added");
        return false;
    }
    if (mShaderName.empty())
    {
        LOG_ERROR("ShaderPackageBuilder: shaderName not set");
        return false;
    }

    // 组装 ShaderPackageMessage
    ShaderPackageMessage pkg = ShaderPackageMessage_init_default;

    // shaderName（EncodeShaderPackage 期望 arg 指向 std::string，生命周期覆盖 pb_encode 调用）
    std::string shaderName = mShaderName;
    pkg.shaderName.arg = &shaderName;

    pkg.format = static_cast<::GnxShaderFormat>(mFormat);

    // 逐 stage 构造 ShaderStageEncodeEntry（msg 标量 + bytes + 反射 encodeData）
    // 所有 vector 在本函数栈帧内，生命周期覆盖 EncodeShaderPackage 的 pb_encode 调用。
    using StageEntry = ShaderMessageUtil::ShaderStageEncodeEntry;
    std::vector<StageEntry> stageEntries;
    stageEntries.reserve(mStages.size());

    std::vector<std::string> entryPoints;   // 持有 entryPoint 字符串，供回调引用
    entryPoints.reserve(mStages.size());

    for (const auto& entry : mStages)
    {
        StageEntry sEntry;
        ShaderMessage& msg = sEntry.msg;
        msg = ShaderMessage_init_default;

        msg.shaderStage    = static_cast<::GnxShaderStage>(entry.stage);
        msg.shaderFormat   = static_cast<::GnxShaderFormat>(mFormat);
        msg.sourceHash     = entry.sourceHash;
        msg.threadgroupSizeX = entry.threadgroupSizeX;
        msg.threadgroupSizeY = entry.threadgroupSizeY;
        msg.threadgroupSizeZ = entry.threadgroupSizeZ;
        msg.vertexStride = (entry.vertexDesc.layouts.empty()) ? 0 : entry.vertexDesc.layouts[0].stride;

        // entryPoint（Metal 需要函数名 VS/PS/CS/TS/MS）
        entryPoints.push_back(entry.entryPoint);
        msg.entryPoint.funcs.encode = shader_string_encode_callback;
        msg.entryPoint.arg = &entryPoints.back();

        // compiledShader bytes
        size_t codeSize = entry.sourceData.size();
        auto* codeBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(codeSize));
        if (!codeBytes)
        {
            LOG_ERROR("ShaderPackageBuilder: malloc failed for stage data");
            return false;
        }
        codeBytes->size = codeSize;
        memcpy(codeBytes->bytes, entry.sourceData.data(), codeSize);
        msg.compiledShader.arg = codeBytes;
        msg.compiledShader.funcs.encode = nanopb_encode_gnx_bytes;

        // 反射 repeated 字段（由 stages 编码回调统一挂 encode 函数）
        FillEncodeDataFromStage(entry.vertexDesc, entry.pushConstants, sEntry.encodeData);

        stageEntries.push_back(std::move(sEntry));
    }

    ByteVectorPtr encoded = ShaderMessageUtil::EncodeShaderPackage(pkg, stageEntries);
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("ShaderPackageBuilder: package encode failed");
        return false;
    }

    // 计算 AssetFileHeader
    uint64_t hash = AssetFileHeaderUtil::ComputeHash(encoded->data(), encoded->size());
    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(
        AssetType::Shader, mShaderName, hash, encoded->size(), AssetFileFlags::NONE);

    // 确保输出目录存在
    fs::path outPath(outputPath);
    fs::path parentDir = outPath.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir))
    {
        fs::create_directories(parentDir);
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("ShaderPackageBuilder: cannot open output file %s", outputPath.c_str());
        return false;
    }

    if (!AssetFileHeaderUtil::WriteHeader(outFile, header))
    {
        LOG_ERROR("ShaderPackageBuilder: write header failed");
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(encoded->data()), encoded->size());
    outFile.close();

    LOG_INFO("ShaderPackageBuilder: saved %s (%zu bytes)", outputPath.c_str(), encoded->size());
    return true;
}

NS_ASSETMANAGER_END
