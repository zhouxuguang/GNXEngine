#include "ShaderAsset.h"
#include "AssetFileHeader.h"
#include "ShaderMessageUtil.h"
#include "ShaderMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>

NS_ASSETMANAGER_BEGIN

// ====================================================================
// 内部辅助：pb ShaderMessage → RenderCore::ShaderStageData（解码即转换）
// ====================================================================

static void FillVertexDescFromPb(const ShaderMessage& msg, RenderCore::VertexDesc& out)
{
    out.attributes.clear();
    out.layouts.clear();

    if (!msg.vertexInputs.arg)
        return;

    const auto* pInputs = (std::vector<VertexInputMessage>*)msg.vertexInputs.arg;
    if (!pInputs || pInputs->empty())
        return;

    out.attributes.reserve(pInputs->size());
    out.layouts.reserve(pInputs->size());
    for (const auto& vi : *pInputs)
    {
        RenderCore::VertextAttributesDesc attr;
        attr.index  = vi.location;
        attr.format = static_cast<RenderCore::VertexFormat>(static_cast<uint32_t>(vi.format));
        attr.offset = vi.offset;
        out.attributes.push_back(attr);

        RenderCore::VertexBufferLayoutDesc layout;
        layout.stride = vi.stride;
        layout.stepRate = 1;
        layout.stepFunction = RenderCore::VertexStepFunctionPerVertex;
        out.layouts.push_back(layout);
    }
}

static void FillPushConstantsFromPb(const ShaderMessage& msg,
                                    std::vector<RenderCore::CompiledPushConstantInfo>& out)
{
    out.clear();
    if (!msg.pushConstants.arg)
        return;

    const auto* pList = (std::vector<PushConstantMessage>*)msg.pushConstants.arg;
    if (!pList)
        return;

    for (const auto& pcm : *pList)
    {
        RenderCore::CompiledPushConstantInfo pc;
        pc.size    = pcm.size;
        pc.set     = pcm.set;
        pc.binding = pcm.binding;
        out.push_back(pc);
    }
}

static RenderCore::ShaderStageData ConvertStageFromPb(const ShaderMessage& msg)
{
    RenderCore::ShaderStageData data;
    data.stage = static_cast<RenderCore::ShaderStage>(static_cast<uint32_t>(msg.shaderStage));
    data.format = static_cast<RenderCore::ShaderFormat>(static_cast<uint32_t>(msg.shaderFormat));
    data.sourceHash = msg.sourceHash;
    data.threadgroupSizeX = msg.threadgroupSizeX;
    data.threadgroupSizeY = msg.threadgroupSizeY;
    data.threadgroupSizeZ = msg.threadgroupSizeZ;

    // entryPoint
    if (msg.entryPoint.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)msg.entryPoint.arg;
        data.entryPoint.assign((const char*)pBytes->bytes, pBytes->size);
    }

    // compiledShader bytes
    if (msg.compiledShader.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)msg.compiledShader.arg;
        data.sourceData.assign(pBytes->bytes, pBytes->bytes + pBytes->size);
    }

    // 反射
    FillVertexDescFromPb(msg, data.vertexDescriptor);
    FillPushConstantsFromPb(msg, data.pushConstants);

    return data;
}

ShaderAsset::ShaderAsset() = default;
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
    // mStages 是值类型，自动释放；mImpl 若持有 pb 临时对象由 pimpl 析构释放
    mStages.clear();
    mShaderName.clear();
    mFormat = RenderCore::ShaderFormat_SPIRV;
    mLoaded = false;
    mOnGPU = false;
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

const RenderCore::ShaderStageData* ShaderAsset::GetStage(RenderCore::ShaderStage stage) const
{
    for (const auto& s : mStages)
    {
        if (s.stage == stage)
            return &s;
    }
    return nullptr;
}

bool ShaderAsset::HasStage(RenderCore::ShaderStage stage) const
{
    return GetStage(stage) != nullptr;
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

    // 反序列化容器 ShaderPackageMessage
    ShaderPackageMessage pkg = ShaderPackageMessage_init_default;
    if (!ShaderMessageUtil::DecodeShaderPackage(pbData.data(), header.dataSize, pkg))
    {
        LOG_ERROR("ShaderAsset: decode ShaderPackageMessage failed for %s", filepath.c_str());
        return false;
    }

    // 解码即转换：pb → ShaderStageData，随后释放 pb 容器
    mStages.clear();
    if (pkg.stages.arg)
    {
        auto* pStages = (std::vector<ShaderMessage>*)pkg.stages.arg;
        mStages.reserve(pStages->size());
        for (const auto& stageMsg : *pStages)
        {
            mStages.push_back(ConvertStageFromPb(stageMsg));
        }
    }

    // shaderName
    if (pkg.shaderName.arg)
    {
        auto* pBytes = (pb_bytes_array_t*)pkg.shaderName.arg;
        mShaderName.assign((const char*)pBytes->bytes, pBytes->size);
    }
    if (mShaderName.empty())
    {
        auto pos = filepath.find_last_of("/\\");
        std::string base = (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
        // 去掉 .{format}.gnxasset 后缀，还原 shader 名
        mShaderName = base;
        for (const char* fmt : { "spirv", "msl_ios", "msl_macos", "dxil", "glsl" })
        {
            std::string suffix = std::string(".") + fmt + ".gnxasset";
            if (mShaderName.size() > suffix.size() &&
                mShaderName.compare(mShaderName.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                mShaderName = mShaderName.substr(0, mShaderName.size() - suffix.size());
                break;
            }
        }
    }

    mFormat = static_cast<RenderCore::ShaderFormat>(static_cast<uint32_t>(pkg.format));

    // 提取文件名
    auto pos = filepath.find_last_of("/\\");
    mName = (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
    mGUID = mName;

    // 释放 pb 容器（解码即转换完成）
    ShaderMessageUtil::ReleaseShaderPackage(pkg);

    mLoaded = true;
    LOG_INFO("ShaderAsset: loaded %s (%zu stages, format %d)", filepath.c_str(), mStages.size(), (int)mFormat);
    return true;
}

NS_ASSETMANAGER_END
