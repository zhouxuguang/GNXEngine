#ifndef GNX_ENGINE_SHADER_ASSET_INCLUDE
#define GNX_ENGINE_SHADER_ASSET_INCLUDE

#include "Asset.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include <memory>
#include <vector>
#include <string>

NS_ASSETMANAGER_BEGIN

/**
 * 编译后的 Shader 资源（容器门面）
 *
 * 一个 .gnxasset 文件对应一个 .shader 源文件在当前目标格式下的全部 stage。
 * 公开接口只暴露纯 C++ 值类型（RenderCore::ShaderStageData），
 * 不暴露 nanopb / pb 结构体。
 *
 * 生命周期：GetStage() 返回的指针在 ShaderAsset 存活期间有效，
 * Reload() / Unload() / Release() 后失效，调用方须在 Release 前用完。
 *
 * 使用方式：
 *   ShaderAsset asset;
 *   asset.LoadFromFile("data_asset/Shader/GBufferPBR.spirv.gnxasset");
 *   if (const auto* vs = asset.GetStage(ShaderStage_Vertex)) {
 *       // vs->sourceData / vs->vertexDescriptor / vs->threadgroupSizeX ...
 *   }
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

    // ====== Shader 容器专属 ======

    /** 从 .gnxasset 文件加载 shader 容器资产 */
    bool LoadFromFile(const std::string& filepath);

    /** 获取容器内 shader 名（相对 data_asset/Shader，如 "vt/GBufferVTPBR"） */
    const std::string& GetShaderName() const { return mShaderName; }

    /** 获取容器目标格式 */
    RenderCore::ShaderFormat GetFormat() const { return mFormat; }

    /** 按阶段获取编译数据（无该 stage 返回 nullptr；指针生命周期见类注释） */
    const RenderCore::ShaderStageData* GetStage(RenderCore::ShaderStage stage) const;

    /** 是否包含指定阶段 */
    bool HasStage(RenderCore::ShaderStage stage) const;

    /** 获取全部 stage 数据 */
    const std::vector<RenderCore::ShaderStageData>& GetStages() const { return mStages; }

private:
    std::string mFilePath;
    std::string mName;
    std::string mGUID;
    std::string mShaderName;
    RenderCore::ShaderFormat mFormat = RenderCore::ShaderFormat_SPIRV;
    std::vector<RenderCore::ShaderStageData> mStages;  // 解码即转换后的常驻值类型
    bool mLoaded = false;
    bool mOnGPU = false;
};

NS_ASSETMANAGER_END

#endif // GNX_ENGINE_SHADER_ASSET_INCLUDE
