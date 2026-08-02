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

bool ShaderImporter::ImportAndSave()
{
    if (mSourcePath.empty() || mOutputPath.empty())
    {
        LOG_ERROR("ShaderImporter: source or output path not set");
        return false;
    }

    mProgress = 0.0f;
    mCancelled = false;

    // Step 1: HLSL → SPIR-V
    LOG_INFO("ShaderImporter: compiling %s → SPIR-V ...", mSourcePath.c_str());
    RenderCore::ShaderStage nativeStage = static_cast<RenderCore::ShaderStage>(mShaderStage);
    RenderCore::ShaderCodePtr spirv = shader_compiler::compileHLSLToSPIRV(
        mSourcePath, nativeStage,
        (mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_iOS ||
         mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_macOS)
            ? RenderCore::RenderDeviceType::METAL
            : RenderCore::RenderDeviceType::VULKAN);

    if (!spirv || spirv->empty())
    {
        LOG_ERROR("ShaderImporter: HLSL → SPIR-V failed for %s", mSourcePath.c_str());
        return false;
    }
    mProgress = 0.4f;

    // Step 2: SPIR-V → 目标格式
    shader_compiler::CompiledShaderInfoPtr result;
    if (mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_iOS ||
        mTargetFormat == GnxShaderFormat_GnxShaderFormat_MSL_macOS)
    {
        result = shader_compiler::compileToMSL(spirv, nativeStage);
    }
    else
    {
        // SPIRV / DXIL / GLSL — 直接使用 SPIR-V 或后续扩展
        result = std::make_shared<shader_compiler::CompiledShaderInfo>();
        result->shaderSource = spirv;
    }

    if (!result || !result->shaderSource || result->shaderSource->empty())
    {
        LOG_ERROR("ShaderImporter: SPIR-V → target format failed");
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

    // compiledShader bytes
    size_t codeSize = shaderInfo->shaderSource->size();
    auto* codeBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(codeSize));
    codeBytes->size = codeSize;
    memcpy(codeBytes->bytes, shaderInfo->shaderSource->data(), codeSize);
    msg.compiledShader.arg = codeBytes;
    msg.compiledShader.funcs.encode = AssetManager::nanopb_encode_gnx_bytes;

    // 序列化
    ByteVectorPtr encoded = AssetManager::ShaderMessageUtil::EncodeShaderMessage(msg);

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
