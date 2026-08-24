#ifndef GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS
#define GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS

#include "AssetImporter.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
#include <string>
#include <vector>
#include <memory>

NS_ASSETPROCESS_BEGIN

/**
 * Shader 离线导入器
 *
 * 流程：
 *   1. 读取 HLSL 源文件
 *   2. 调用 ShaderCompiler::CompileShader(..., ShaderFormat) 编译目标格式
 *   3. 通过 ShaderPackageBuilder 组装 ShaderPackageMessage 容器
 *   4. 序列化为 .gnxasset 文件（含 AssetFileHeader）
 *
 * 用法：
 *   ShaderImporter importer;
 *   importer.SetSourcePath("shaders/pbr_vs.hlsl");
 *   importer.SetShaderStage(RenderCore::ShaderStage_Vertex);
 *   importer.SetTargetFormat(RenderCore::ShaderFormat_MSL_iOS);
 *   importer.SetOutputPath("data_asset/shaders/pbr_vs.ios.gnxasset");
 *   importer.ImportAndSave();
 */
class ASSET_PROCESS_API ShaderImporter : public AssetImporter
{
public:
    ShaderImporter();
    virtual ~ShaderImporter();

    // ====== 设置 ======

    /** 设置 HLSL 源文件路径 */
    void SetSourcePath(const std::string& path) { mSourcePath = path; }

    /** 设置 shader 阶段 */
    void SetShaderStage(RenderCore::ShaderStage stage) { mShaderStage = stage; }

    /** 设置目标格式 */
    void SetTargetFormat(RenderCore::ShaderFormat format) { mTargetFormat = format; }

    /** 设置输出路径 */
    void SetOutputPath(const std::string& path) { mOutputPath = path; }

    /** 设置 shader 名（相对 data_asset/Shader 的路径，如 "vt/GBufferVTPBR"） */
    void SetShaderName(const std::string& name) { mShaderName = name; }

    // ====== 保存 ======

    /**
     * 执行导入并保存为 .gnxasset 文件
     * @return 成功/失败
     */
    bool ImportAndSave();

private:
    RenderCore::ShaderStage mShaderStage = RenderCore::ShaderStage_Vertex;
    RenderCore::ShaderFormat mTargetFormat = RenderCore::ShaderFormat_SPIRV;
    std::string mSourcePath;
    std::string mOutputPath;
    std::string mShaderName;
    float mProgress = 0.0f;
    bool mCancelled = false;
};

NS_ASSETPROCESS_END

#endif // GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS
