#include "ShaderAsset.h"
#include "AssetFileHeader.h"
#include "ShaderMessageUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>

NS_ASSETMANAGER_BEGIN

ShaderAsset::ShaderAsset()
{
    mShaderMessage = ShaderMessage_init_default;
}

ShaderAsset::~ShaderAsset()
{
    Unload();
}

const std::string& ShaderAsset::GetGUID() const { return mGUID; }
const std::string& ShaderAsset::GetName() const { return mName; }
const std::string& ShaderAsset::GetFilePath() const { return mFilePath; }

bool ShaderAsset::Load()
{
    return LoadFromFile(mFilePath);
}

void ShaderAsset::Unload()
{
    ShaderMessageUtil::ReleaseShaderMessage(mShaderMessage);
    mLoaded = false;
    mOnGPU = false;
    mUniformBuffers = nullptr;
    mPushConstants = nullptr;
    mResources = nullptr;
    mVertexInputs = nullptr;
}

bool ShaderAsset::Reload()
{
    Unload();
    return Load();
}

bool ShaderAsset::IsLoaded() const
{
    return mLoaded;
}

bool ShaderAsset::UploadToGPU()
{
    // 当前 Metal / Vulkan 运行时通过 ShaderAsset 暴露的数据自行创建 shader 对象，
    // UploadToGPU 留作将来扩展（如自动创建 MTLFunction / VkShaderModule）
    mOnGPU = true;
    return true;
}

void ShaderAsset::ReleaseFromGPU()
{
    mOnGPU = false;
}

bool ShaderAsset::IsOnGPU() const
{
    return mOnGPU;
}

const uint8_t* ShaderAsset::GetShaderData() const
{
    if (mShaderMessage.compiledShader.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)mShaderMessage.compiledShader.arg;
        return pBytes->bytes;
    }
    return nullptr;
}

uint32_t ShaderAsset::GetShaderDataSize() const
{
    if (mShaderMessage.compiledShader.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)mShaderMessage.compiledShader.arg;
        return pBytes->size;
    }
    return 0;
}

bool ShaderAsset::LoadFromFile(const std::string& filepath)
{
    mFilePath = filepath;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("ShaderAsset: cannot open %s", filepath.c_str());
        return false;
    }

    // 读取 AssetFileHeader（128 字节）
    AssetFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(AssetFileHeader));
    if (file.fail() || !AssetFileHeaderUtil::ValidateHeader(header))
    {
        LOG_ERROR("ShaderAsset: invalid asset header in %s", filepath.c_str());
        return false;
    }

    if (header.fileType != static_cast<uint32_t>(AssetType::Shader))
    {
        LOG_ERROR("ShaderAsset: wrong asset type %u", header.fileType);
        return false;
    }

    // 读取 protobuf 数据
    std::vector<uint8_t> pbData(header.dataSize);
    file.read(reinterpret_cast<char*>(pbData.data()), header.dataSize);
    file.close();

    // 反序列化
    if (!ShaderMessageUtil::DecodeShaderMessage(pbData.data(), header.dataSize, mShaderMessage))
    {
        LOG_ERROR("ShaderAsset: decode ShaderMessage failed for %s", filepath.c_str());
        return false;
    }

    // 提取入口点名称
    if (mShaderMessage.entryPoint.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)mShaderMessage.entryPoint.arg;
        mEntryPoint.assign((const char*)pBytes->bytes, pBytes->size);
    }

    // 提取解码后的 vector 指针（方便外部遍历）
    mUniformBuffers = (std::vector<UniformBufferLayoutMessage>*)mShaderMessage.uniformBuffers.arg;
    mPushConstants   = (std::vector<PushConstantMessage>*)mShaderMessage.pushConstants.arg;
    mResources       = (std::vector<ShaderResourceMessage>*)mShaderMessage.resources.arg;
    mVertexInputs    = (std::vector<VertexInputMessage>*)mShaderMessage.vertexInputs.arg;

    // 提取文件名
    auto pos = filepath.find_last_of("/\\");
    mName = (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
    mGUID = mName;

    mLoaded = true;
    LOG_INFO("ShaderAsset: loaded %s (%u bytes shader data)", filepath.c_str(), GetShaderDataSize());
    return true;
}

NS_ASSETMANAGER_END
