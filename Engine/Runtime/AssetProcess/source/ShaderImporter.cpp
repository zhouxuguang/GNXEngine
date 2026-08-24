#include "ShaderImporter.h"
#include "Runtime/AssetManager/include/ShaderPackageBuilder.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>
#include <sstream>

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

    // Step 1: HLSL -> target format (CompileShader dispatches by ShaderFormat)
    LOG_INFO("ShaderImporter: compiling %s ...", mSourcePath.c_str());
    shader_compiler::CompiledShaderInfoPtr result =
        shader_compiler::CompileShader(mSourcePath, mShaderStage, mTargetFormat);

    if (!result || !result->shaderSource || result->shaderSource->empty())
    {
        LOG_ERROR("ShaderImporter: compile failed for %s", mSourcePath.c_str());
        return false;
    }
    mProgress = 0.6f;

    // Step 2: compute source hash (FNV-1a 64)
    uint64_t sourceHash = 0;
    {
        std::ifstream srcFile(mSourcePath, std::ios::binary);
        if (srcFile.is_open())
        {
            std::stringstream ss;
            ss << srcFile.rdbuf();
            std::string content = ss.str();
            const uint8_t* data = (const uint8_t*)content.data();
            size_t size = content.size();
            uint64_t h = 1469598103934665603ULL;
            for (size_t i = 0; i < size; ++i)
            {
                h ^= data[i];
                h *= 1099511628211ULL;
            }
            sourceHash = h;
        }
    }

    // Step 3: assemble via ShaderPackageBuilder (no pb exposure)
    AssetManager::ShaderPackageBuilder builder;
    builder.SetFormat(mTargetFormat);
    if (!mShaderName.empty())
    {
        builder.SetShaderName(mShaderName);
    }
    else
    {
        // default shaderName from output path (strip .{format}.gnxasset)
        std::string name = mOutputPath;
        std::string suffix = std::string(".") + RenderCore::ShaderFormatToString(mTargetFormat) + ".gnxasset";
        if (name.size() > suffix.size() &&
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            name = name.substr(0, name.size() - suffix.size());
        }
        builder.SetShaderName(name);
    }

    const char* entryPoint = "VS";
    switch (mShaderStage)
    {
        case RenderCore::ShaderStage_Vertex:   entryPoint = "VS"; break;
        case RenderCore::ShaderStage_Fragment: entryPoint = "PS"; break;
        case RenderCore::ShaderStage_Compute:  entryPoint = "CS"; break;
        case RenderCore::ShaderStage_Task:     entryPoint = "TS"; break;
        case RenderCore::ShaderStage_Mesh:     entryPoint = "MS"; break;
        default: break;
    }

    builder.AddStage(mShaderStage,
                     entryPoint,
                     *result->shaderSource,
                     result->vertexDescriptor,
                     result->threadgroupSizeX,
                     result->threadgroupSizeY,
                     result->threadgroupSizeZ,
                     result->pushConstants,
                     sourceHash);

    mProgress = 0.9f;

    bool ok = builder.Save(mOutputPath);
    mProgress = ok ? 1.0f : 0.0f;
    return ok;
}

NS_ASSETPROCESS_END
