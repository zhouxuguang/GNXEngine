#ifndef GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS
#define GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS

#include "AssetImporter.h"
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
 *   2. 调用 ShaderCompiler::compileHLSLToSPIRV() → SPIR-V
 *   3. 调用 ShaderCompiler::compileToMSL() 或直接打包 SPIR-V
 *   4. 填充 ShaderMessage proto
 *   5. 序列化为 .gnxasset 文件
 *
 * 用法：
 *   ShaderImporter importer;
 *   importer.SetSourcePath("shaders/pbr_vs.hlsl");
 *   importer.SetShaderStage(ShaderStage_Vertex);
 *   importer.SetTargetFormat(ShaderFormat_MSL_iOS);  // 选择目标平台
 *   importer.SetOutputPath("data_asset/shaders/pbr_vs.ios.gnxasset");
 *   importer.Import();
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
    void SetShaderStage(uint32_t stage) { mShaderStage = stage; }

    /** 设置目标格式 */
    void SetTargetFormat(uint32_t format) { mTargetFormat = format; }

    /** 设置输出路径 */
    void SetOutputPath(const std::string& path) { mOutputPath = path; }

    // ====== 保存 ======

    /**
     * 执行导入并保存为 .gnxasset 文件
     * @return 成功/失败
     */
    bool ImportAndSave();

    /**
     * 保存编译完成的 shader 到文件
     * @param shaderInfo 编译结果
     * @param sourceHash HLSL 源码 hash
     * @return 成功/失败
     */
    bool SaveShaderFile(const shader_compiler::CompiledShaderInfoPtr& shaderInfo,
                        uint64_t sourceHash);

private:
    uint32_t mShaderStage = 0;    // ShaderStage proto enum value
    uint32_t mTargetFormat = 0;   // ShaderFormat proto enum value
    std::string mSourcePath;
    std::string mOutputPath;
    float mProgress = 0.0f;
    bool mCancelled = false;
};

NS_ASSETPROCESS_END

#endif // GNX_ENGINE_SHADER_IMPORTER_INCLUDE_FHDS
