#include "ShaderImporter.h"
#include "Runtime/AssetManager/include/ShaderMessageUtil.h"
#include "Runtime/AssetManager/include/ShaderMessage.pb.h"
#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/HashFunction.h"
#include "Runtime/AssetManager/source/PBUtils.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

NS_ASSETPROCESS_BEGIN

ShaderImporter::ShaderImporter() = default;
ShaderImporter::~ShaderImporter() = default;

// 从 RenderCore::VertexDesc 构造 proto vertexInputs
// Metal 的 GetMetalReflectionInfo 为每个 stage_input 生成一个 attribute 和一个 layout，
// attributes[i] 与 layouts[i] 一一对应（index = attribute location）
static void FillVertexInputsFromVertexDesc(const RenderCore::VertexDesc& desc,
                                           std::vector<VertexInputMessage>& out)
{
    out.clear();
    out.reserve(desc.attributes.size());

    for (size_t i = 0; i < desc.attributes.size(); ++i)
    {
        const auto& attr = desc.attributes[i];

        VertexInputMessage vi = VertexInputMessage_init_default;

        // semantic 名（Metal 不需要精确语义名，保留空；location 才是关键）
        // 这里用空字符串即可，nanopb string 需要回调 encode
        // 注意：VertexInputMessage.semantic 是 pb_callback_t，无法直接赋值；
        // 但由于 nanopb 在 encode 时对 null 回调会跳过该字段，留空即可。
        vi.location = attr.index;
        vi.format   = static_cast<::VertexDataFormat>(static_cast<uint32_t>(attr.format));
        vi.offset   = attr.offset;

        // 该 attribute 对应 layout 的 stride（Metal 每 attribute 一个 layout）
        if (i < desc.layouts.size())
        {
            vi.stride = desc.layouts[i].stride;
        }
        else
        {
            vi.stride = 0;
        }

        out.push_back(vi);
    }
}

// 从 CompiledShaderInfo 填充反射 repeated 字段
static void FillEncodeDataFromShaderInfo(const shader_compiler::CompiledShaderInfoPtr& shaderInfo,
                                         AssetManager::ShaderMessageEncodeData& out)
{
    // vertexInputs（由 vertexDescriptor 转换，Metal 重建 VertexDesc 必需）
    if (shaderInfo->vertexDescriptor.attributes.size() > 0)
    {
        FillVertexInputsFromVertexDesc(shaderInfo->vertexDescriptor, out.vertexInputs);
    }

    // pushConstants（当前恒空：patch 被禁用；为将来启用预留）
    out.pushConstants.clear();
    for (const auto& pc : shaderInfo->pushConstants)
    {
        PushConstantMessage pcm = PushConstantMessage_init_default;
        pcm.size    = pc.size;
        pcm.set     = pc.set;
        pcm.binding = pc.binding;
        // name 是 pb_callback_t，留空（nanopb 对 null 回调跳过 string 字段）
        out.pushConstants.push_back(pcm);
    }
}

// 字符串编码回调（entryPoint 字段，pb_callback_t 需要）
static bool shader_string_encode_callback(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    const std::string* str = static_cast<const std::string*>(*arg);
    if (!str || str->empty())
    {
        return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)"", 0);
    }
    return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)str->c_str(), str->length());
}

bool ShaderImporter::ImportAndSave()
{
    if (mSourcePath.empty() || mOutputPath.empty())
    {
        LOG_ERROR("ShaderImporter: source or output path not set");
        return false;
    }

    mProgress = 0.0f;
    mCancelled = false;

    // Step 1: HLSL → 目标格式（复用 CompileShader，含 mesh payload patch + threadgroup 提取）
    LOG_INFO("ShaderImporter: compiling %s ...", mSourcePath.c_str());
    RenderCore::ShaderStage nativeStage = static_cast<RenderCore::ShaderStage>(mShaderStage);
    RenderCore::RenderDeviceType deviceType =
        (mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_iOS ||
         mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_macOS)
            ? RenderCore::RenderDeviceType::METAL
            : RenderCore::RenderDeviceType::VULKAN;

    shader_compiler::CompiledShaderInfoPtr result =
        shader_compiler::CompileShader(mSourcePath, nativeStage, deviceType);

    if (!result || !result->shaderSource || result->shaderSource->empty())
    {
        LOG_ERROR("ShaderImporter: compile failed for %s", mSourcePath.c_str());
        return false;
    }
    mProgress = 0.6f;

    // Step 3: 计算源码 hash
    uint64_t sourceHash = 0;
    {
        std::ifstream srcFile(mSourcePath, std::ios::binary | std::ios::ate);
        if (srcFile.is_open())
        {
            size_t fileSize = srcFile.tellg();
            srcFile.seekg(0);
            std::vector<char> srcData(fileSize);
            srcFile.read(srcData.data(), fileSize);
            sourceHash = baselib::HashFunction(srcData.data(), fileSize);
        }
    }

    // Step 4: 保存
    bool ok = SaveShaderFile(result, sourceHash);
    mProgress = ok ? 1.0f : 0.0f;
    return ok;
}

bool ShaderImporter::SaveShaderFile(const shader_compiler::CompiledShaderInfoPtr& shaderInfo,
                                    uint64_t sourceHash)
{
    if (!shaderInfo)
        return false;

    // 构建 ShaderMessage
    ShaderMessage msg = ShaderMessage_init_default;

    msg.shaderStage    = static_cast<::GnxShaderStage>(mShaderStage);
    msg.shaderFormat   = static_cast<::GnxShaderFormat>(mTargetFormat);
    msg.sourceHash     = sourceHash;
    msg.threadgroupSizeX = shaderInfo->threadgroupSizeX;
    msg.threadgroupSizeY = shaderInfo->threadgroupSizeY;
    msg.threadgroupSizeZ = shaderInfo->threadgroupSizeZ;

    // entryPoint（Metal 需要函数名 VS/PS/CS/TS/MS）
    // pb_callback_t 需要回调编码，这里用一个静态 helper + 局部字符串
    std::string entryPoint;
    switch (mShaderStage)
    {
        case GnxShaderStage_GnxShaderStage_Vertex:   entryPoint = "VS"; break;
        case GnxShaderStage_GnxShaderStage_Fragment: entryPoint = "PS"; break;
        case GnxShaderStage_GnxShaderStage_Compute:  entryPoint = "CS"; break;
        case GnxShaderStage_GnxShaderStage_Task:     entryPoint = "TS"; break;
        case GnxShaderStage_GnxShaderStage_Mesh:     entryPoint = "MS"; break;
        default: break;
    }
    msg.entryPoint.funcs.encode = shader_string_encode_callback;
    msg.entryPoint.arg = &entryPoint;

    // compiledShader bytes
    size_t codeSize = shaderInfo->shaderSource->size();
    auto* codeBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(codeSize));
    codeBytes->size = codeSize;
    memcpy(codeBytes->bytes, shaderInfo->shaderSource->data(), codeSize);
    msg.compiledShader.arg = codeBytes;
    msg.compiledShader.funcs.encode = AssetManager::nanopb_encode_gnx_bytes;

    // 反射 repeated 字段
    AssetManager::ShaderMessageEncodeData encodeData;
    FillEncodeDataFromShaderInfo(shaderInfo, encodeData);

    // 序列化
    ByteVectorPtr encoded = AssetManager::ShaderMessageUtil::EncodeShaderMessage(msg, encodeData);

    // 清理 bytes 数组
    free(codeBytes);
    msg.compiledShader.arg = nullptr;

    if (!encoded || encoded->empty())
    {
        LOG_ERROR("ShaderImporter: protobuf encode failed");
        return false;
    }

    // 创建 AssetFileHeader
    uint64_t hash = AssetManager::AssetFileHeaderUtil::ComputeHash(
        encoded->data(), encoded->size());
    AssetManager::AssetFileHeader header = AssetManager::AssetFileHeaderUtil::CreateHeader(
        AssetManager::AssetType::Shader,
        mSourcePath,
        hash,
        encoded->size(),
        AssetManager::AssetFileFlags::NONE);

    // 确保输出目录存在
    fs::path outPath(mOutputPath);
    fs::path parentDir = outPath.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir))
    {
        fs::create_directories(parentDir);
    }

    // 写入文件
    std::ofstream outFile(mOutputPath, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("ShaderImporter: cannot open output file %s", mOutputPath.c_str());
        return false;
    }

    if (!AssetManager::AssetFileHeaderUtil::WriteHeader(outFile, header))
    {
        LOG_ERROR("ShaderImporter: write header failed");
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(encoded->data()), encoded->size());
    outFile.close();

    LOG_INFO("ShaderImporter: saved %s (%zu bytes)", mOutputPath.c_str(), encoded->size());
    return true;
}

NS_ASSETPROCESS_END
