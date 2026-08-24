//
//  ShaderPackageBuilder.h
//  GNXEngine
//
//  公开的 shader 容器构建 API。
//
//  用途：供离线工具（tool/shader_compile）将编译好的各 stage shader
//  组装成单个 {name}.{format}.gnxasset 文件。内部完成 pb 序列化与
//  AssetFileHeader 写入，调用方无需接触 nanopb / pb 结构体。
//
//  使用示例：
//    ShaderPackageBuilder builder;
//    builder.SetShaderName("vt/GBufferVTPBR");
//    builder.SetFormat(ShaderFormat_SPIRV);
//    builder.AddStage(ShaderStage_Vertex, "VS", spirvData, vertexDesc, tgX, tgY, tgZ, pushConstants, sourceHash);
//    builder.AddStage(ShaderStage_Fragment, "PS", spirvData, ..., sourceHash);
//    builder.Save("data_asset/Shader/vt/GBufferVTPBR.spirv.gnxasset");
//

#ifndef GNX_ENGINE_SHADER_PACKAGE_BUILDER_INCLUDE
#define GNX_ENGINE_SHADER_PACKAGE_BUILDER_INCLUDE

#include "AssetDefine.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include <string>
#include <vector>

NS_ASSETMANAGER_BEGIN

class ASSET_MANAGER_API ShaderPackageBuilder
{
public:
    ShaderPackageBuilder();
    ~ShaderPackageBuilder();

    // ====== 设置 ======

    /** 设置 shader 名（相对 data_asset/Shader 的路径，如 "vt/GBufferVTPBR"） */
    void SetShaderName(const std::string& name) { mShaderName = name; }

    /** 设置目标格式 */
    void SetFormat(RenderCore::ShaderFormat format) { mFormat = format; }

    /** 添加一个 stage 的编译结果 */
    void AddStage(RenderCore::ShaderStage stage,
                  const std::string& entryPoint,
                  const RenderCore::ShaderCode& sourceData,
                  const RenderCore::VertexDesc& vertexDesc,
                  uint32_t threadgroupSizeX,
                  uint32_t threadgroupSizeY,
                  uint32_t threadgroupSizeZ,
                  const std::vector<RenderCore::CompiledPushConstantInfo>& pushConstants,
                  uint64_t sourceHash);

    /** 返回已添加的 stage 数 */
    size_t GetStageCount() const { return mStages.size(); }

    // ====== 保存 ======

    /**
     * 序列化为 .gnxasset 文件（含 AssetFileHeader）
     * @param outputPath 输出文件路径
     * @return 成功返回 true
     */
    bool Save(const std::string& outputPath);

private:
    struct StageEntry
    {
        RenderCore::ShaderStage stage;
        std::string entryPoint;
        RenderCore::ShaderCode sourceData;
        RenderCore::VertexDesc vertexDesc;
        uint32_t threadgroupSizeX = 0;
        uint32_t threadgroupSizeY = 0;
        uint32_t threadgroupSizeZ = 0;
        std::vector<RenderCore::CompiledPushConstantInfo> pushConstants;
        uint64_t sourceHash = 0;
    };

    std::string mShaderName;
    RenderCore::ShaderFormat mFormat = RenderCore::ShaderFormat_SPIRV;
    std::vector<StageEntry> mStages;
};

NS_ASSETMANAGER_END

#endif // GNX_ENGINE_SHADER_PACKAGE_BUILDER_INCLUDE
