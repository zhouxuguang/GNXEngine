#ifndef GNX_ENGINE_SHADER_ASSET_INCLUDE
#define GNX_ENGINE_SHADER_ASSET_INCLUDE

#include "Asset.h"
#include "ShaderMessage.pb.h"
#include <vector>
#include <string>

NS_ASSETMANAGER_BEGIN

/**
 * 编译后的 Shader 资源
 *
 * 运行时从 .gnxasset 文件加载，提供：
 *   - 编译后的 shader 二进制（MSL 文本 / SPIR-V 二进制）
 *   - 反射元数据（uniform buffer 布局、采样器、顶点输入）
 *   - threadgroup 大小（compute/mesh/task）
 *
 * 使用方式：
 *   ShaderAsset asset;
 *   asset.LoadFromFile("data_asset/shaders/pbr_vs.ios.gnxasset");
 *   const auto* msg = asset.GetShaderMessage();
 *   // msg->compiledShader  → Metal MSL 文本 或 SPIR-V 二进制
 *   // msg->uniformBuffers  → UBO 布局
 *   // msg->vertexInputs    → 顶点输入布局
 */
class ASSET_MANAGER_API ShaderAsset : public Asset
{
public:
    ShaderAsset();
    virtual ~ShaderAsset();

    // ====== Asset 接口 ======

    virtual AssetType GetType() const override { return AssetType::Shader; }
    virtual const std::string& GetGUID() const override;
    virtual const std::string& GetName() const override;
    virtual const std::string& GetFilePath() const override;

    virtual bool Load() override;
    virtual void Unload() override;
    virtual bool Reload() override;
    virtual bool IsLoaded() const override;

    virtual bool UploadToGPU() override;
    virtual void ReleaseFromGPU() override;
    virtual bool IsOnGPU() const override;

    // ====== Shader 专属 ======

    /** 获取反序列化后的 ShaderMessage */
    const ShaderMessage& GetShaderMessage() const { return mShaderMessage; }

    /** 获取编译后的 shader 数据指针 */
    const uint8_t* GetShaderData() const;

    /** 获取编译后的 shader 数据大小 */
    uint32_t GetShaderDataSize() const;

    /** 获取 shader 阶段 */
    GnxShaderStage GetShaderStage() const { return mShaderMessage.shaderStage; }

    /** 获取 shader 格式 */
    GnxShaderFormat GetShaderFormat() const { return mShaderMessage.shaderFormat; }

    /** 获取入口点名称 */
    const char* GetEntryPoint() const { return mEntryPoint.c_str(); }

private:
    bool LoadFromFile(const std::string& filepath);

    std::string mFilePath;
    std::string mName;
    std::string mGUID;
    std::string mEntryPoint;
    ShaderMessage mShaderMessage;
    bool mLoaded = false;
    bool mOnGPU = false;

    // 持有解码后的动态数据，确保生命周期
    std::vector<UniformBufferLayoutMessage>* mUniformBuffers = nullptr;
    std::vector<PushConstantMessage>* mPushConstants = nullptr;
    std::vector<ShaderResourceMessage>* mResources = nullptr;
    std::vector<VertexInputMessage>* mVertexInputs = nullptr;
};

NS_ASSETMANAGER_END

#endif // GNX_ENGINE_SHADER_ASSET_INCLUDE
