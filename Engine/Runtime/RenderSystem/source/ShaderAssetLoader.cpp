//
//  ShaderAssetLoader.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#include "ShaderAssetLoader.h"
#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
#include "RenderEngine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include "Runtime/AssetManager/include/ShaderAsset.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

// ============================================================================
// 内部辅助：从 .gnxasset 加载预编译 shader
// ============================================================================

// Map render device type to ShaderFormat (explicit parameterization source at runtime)
static RenderCore::ShaderFormat GetShaderFormat(RenderDeviceType renderType)
{
    switch (renderType)
    {
        case RenderDeviceType::METAL:
#if TARGET_OS_OSX
            return RenderCore::ShaderFormat_MSL_macOS;
#else
            return RenderCore::ShaderFormat_MSL_iOS;
#endif
        case RenderDeviceType::VULKAN:
        default:
            return RenderCore::ShaderFormat_SPIRV;
    }
}

// Format suffix (used in file naming)
static std::string GetShaderFormatSuffix(RenderCore::ShaderFormat format)
{
    return RenderCore::ShaderFormatToString(format);
}

// Build CompiledShaderInfo from ShaderStageData (RenderCore value type, not pb)
static CompiledShaderInfoPtr BuildCompiledShaderInfo(const RenderCore::ShaderStageData& stageData,
                                                     ShaderStage shaderStage)
{
    // Validate stage match
    if (stageData.stage != shaderStage)
    {
        LOG_WARN("LoadCompiledShader: stage mismatch (expected %u, got %u)",
                 (uint32_t)shaderStage, (uint32_t)stageData.stage);
        return nullptr;
    }

    // Build CompiledShaderInfo
    auto info = std::make_shared<CompiledShaderInfo>();
    info->format = stageData.format;
    info->shaderSource = std::make_shared<ShaderCode>();
    if (!stageData.sourceData.empty())
    {
        info->shaderSource->resize(stageData.sourceData.size());
        memcpy(info->shaderSource->data(), stageData.sourceData.data(), stageData.sourceData.size());
    }

    info->threadgroupSizeX = stageData.threadgroupSizeX;
    info->threadgroupSizeY = stageData.threadgroupSizeY;
    info->threadgroupSizeZ = stageData.threadgroupSizeZ;

    // Vertex descriptor (already decoded by ShaderAsset from pb vertexInputs)
    info->vertexDescriptor = stageData.vertexDescriptor;

    // Push constants (already decoded)
    info->pushConstants = stageData.pushConstants;

    return info;
}

// Load a single stage compile result from a container file
// File naming: {shaderName}.{format}.gnxasset (one file per shader, all stages inside)
static CompiledShaderInfoPtr LoadCompiledShaderFromAsset(AssetManager::ShaderAsset* shaderAsset,
                                                          ShaderStage shaderStage,
                                                          const std::string& formatSuffix)
{
    // Get stage from container (ShaderAsset already decoded to ShaderStageData value type)
    const RenderCore::ShaderStageData* stageData = shaderAsset->GetStage(shaderStage);
    if (!stageData)
    {
        // Stage not present in container (normal: e.g. pure compute has no vs/ps)
        return nullptr;
    }

    CompiledShaderInfoPtr info = BuildCompiledShaderInfo(*stageData, shaderStage);

    LOG_INFO("LoadCompiledShader: loaded stage %u from %s.%s.gnxasset (%zu bytes)",
             (uint32_t)shaderStage, shaderAsset->GetShaderName().c_str(),
             formatSuffix.c_str(), stageData->sourceData.size());

    return info;
}

// Check container file existence ({name}.{format}.gnxasset)
// Stage completeness is checked via GetStage in LoadShaderAsset after loading
static bool HasContainerFile(const std::string& shaderName, const std::string& formatSuffix)
{
    std::string filePath = getCompiledShaderDir() + shaderName + "." + formatSuffix + ".gnxasset";
    std::ifstream f(filePath, std::ios::binary);
    return f.good();
}

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;

    if (!GetRenderDevice())
    {
        return shaderAssetString;
    }

    RenderDeviceType renderType = GetRenderDevice()->GetRenderDeviceType();
    RenderCore::ShaderFormat format = GetShaderFormat(renderType);
    std::string formatSuffix = GetShaderFormatSuffix(format);

    // Load precompiled asset from data_asset/Shader/ ({name}.{format}.gnxasset)
    // fallback: if container file missing, fall back to runtime compile
    if (!HasContainerFile(shaderName, formatSuffix))
    {
        LOG_WARN("LoadShaderAsset: precompiled shader missing for %s (format %s), falling back to runtime compile",
                 shaderName.c_str(), formatSuffix.c_str());
        std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
        return LoadCustomShaderAsset(shaderFilePath);
    }

    std::string filePath = getCompiledShaderDir() + shaderName + "." + formatSuffix + ".gnxasset";

    AssetManager::AssetManager* assetMgr = AssetManager::AssetManager::GetInstance();
    AssetManager::ShaderAsset* shaderAsset = nullptr;
    AssetManager::ShaderAsset localAsset;

    if (!assetMgr)
    {
        LOG_WARN("LoadShaderAsset: AssetManager not initialized, local fallback: %s", filePath.c_str());
        if (!localAsset.LoadFromFile(filePath))
        {
            LOG_WARN("LoadShaderAsset: asset not found: %s", filePath.c_str());
            std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
            return LoadCustomShaderAsset(shaderFilePath);
        }
        shaderAsset = &localAsset;
    }
    else
    {
        shaderAsset = assetMgr->LoadShader(filePath);
        if (!shaderAsset)
        {
            LOG_WARN("LoadShaderAsset: asset not found: %s", filePath.c_str());
            std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
            return LoadCustomShaderAsset(shaderFilePath);
        }
    }

    // Validate container format matches expected (prevent "right filename, wrong content")
    if (shaderAsset->GetFormat() != format)
    {
        LOG_WARN("LoadShaderAsset: format mismatch for %s (expected %d, got %d), falling back to runtime compile",
                 shaderName.c_str(), (int)format, (int)shaderAsset->GetFormat());
        if (assetMgr && shaderAsset) shaderAsset->Release();
        std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
        return LoadCustomShaderAsset(shaderFilePath);
    }

    // Required-stage check via GetStage: mesh pipeline needs ms+ps; traditional needs vs+ps;
    // pure compute needs cs. Equivalent to old HasRequiredGraphicsStages but container-based.
    bool hasRequired = false;
    if (shaderAsset->HasStage(ShaderStage_Mesh))
    {
        hasRequired = shaderAsset->HasStage(ShaderStage_Fragment);
    }
    else if (shaderAsset->HasStage(ShaderStage_Vertex))
    {
        hasRequired = shaderAsset->HasStage(ShaderStage_Fragment);
    }
    else
    {
        hasRequired = shaderAsset->HasStage(ShaderStage_Compute);
    }

    if (!hasRequired)
    {
        LOG_WARN("LoadShaderAsset: required stages missing in container for %s, falling back to runtime compile",
                 shaderName.c_str());
        if (assetMgr && shaderAsset) shaderAsset->Release();
        std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
        return LoadCustomShaderAsset(shaderFilePath);
    }

    shaderAssetString.vertexShader = LoadCompiledShaderFromAsset(shaderAsset, ShaderStage_Vertex, formatSuffix);
    if (shaderAssetString.vertexShader)
    {
        shaderAssetString.vertexDescriptor = shaderAssetString.vertexShader->vertexDescriptor;
    }

    shaderAssetString.fragmentShader = LoadCompiledShaderFromAsset(shaderAsset, ShaderStage_Fragment, formatSuffix);
    shaderAssetString.computeShader   = LoadCompiledShaderFromAsset(shaderAsset, ShaderStage_Compute, formatSuffix);
    shaderAssetString.taskShader      = LoadCompiledShaderFromAsset(shaderAsset, ShaderStage_Task, formatSuffix);
    shaderAssetString.meshShader      = LoadCompiledShaderFromAsset(shaderAsset, ShaderStage_Mesh, formatSuffix);

    // Data already copied into CompiledShaderInfo; ShaderAsset no longer needed long-term
    if (assetMgr && shaderAsset)
    {
        shaderAsset->Release();
    }

    return shaderAssetString;
}

ShaderAssetString LoadCustomShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    if (!GetRenderDevice())
    {
        return shaderAssetString;
    }
    
    RenderDeviceType renderType = GetRenderDevice()->GetRenderDeviceType();
    RenderCore::ShaderFormat format = GetShaderFormat(renderType);
    
    CompiledShaderInfoPtr vertexShaderInfo = CompileShader(shaderName, ShaderStage_Vertex, format);
    
    if (vertexShaderInfo)
    {
        shaderAssetString.vertexShader = vertexShaderInfo;
        shaderAssetString.vertexDescriptor = vertexShaderInfo->vertexDescriptor;
    }
    
    CompiledShaderInfoPtr fragmentShaderInfo = CompileShader(shaderName, ShaderStage_Fragment, format);
    if (fragmentShaderInfo)
    {
        shaderAssetString.fragmentShader = fragmentShaderInfo;
    }
    
    CompiledShaderInfoPtr computeShaderInfo = CompileShader(shaderName, ShaderStage_Compute, format);
    if (computeShaderInfo)
    {
        shaderAssetString.computeShader = computeShaderInfo;
    }
    
    CompiledShaderInfoPtr taskShaderInfo = CompileShader(shaderName, ShaderStage_Task, format);
    if (taskShaderInfo)
    {
        shaderAssetString.taskShader = taskShaderInfo;
    }
    
    CompiledShaderInfoPtr meshShaderInfo = CompileShader(shaderName, ShaderStage_Mesh, format);
    if (meshShaderInfo)
    {
        shaderAssetString.meshShader = meshShaderInfo;
    }
    
    return shaderAssetString;
}

GraphicsShaderInfo CreateGraphicsShaderInfo(const std::string& shaderName)
{
    ShaderAssetString shaderAssetString = LoadShaderAsset(shaderName);
    
    GraphicsShaderInfo graphicsShaderInfo;
    
    // 判断是否为 Mesh Shader 管线（有 mesh shader 但没有 vertex shader）
    bool isMeshShader = shaderAssetString.meshShader && !shaderAssetString.vertexShader;
    
    if (isMeshShader)
    {
        // Mesh Shader 管线：Task(可选) + Mesh + Fragment
        ShaderCodePtr taskShader = shaderAssetString.taskShader ? shaderAssetString.taskShader->shaderSource : ShaderCodePtr();
        ShaderCodePtr meshShader = shaderAssetString.meshShader->shaderSource;
        ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
        
        ShaderCode emptyTask; // 空 task shader
        GraphicsShaderPtr graphicsShader = GetRenderDevice()->CreateMeshGraphicsShader(
            taskShader ? *taskShader : emptyTask,
            *meshShader,
            *fragmentShader);

        // Pass SPIR-V threadgroup sizes (from HLSL [numthreads]) to the graphics shader,
        // so the pipeline can use them instead of hardcoded defaults.
        if (shaderAssetString.taskShader->threadgroupSizeX > 0)
        {
            graphicsShader->SetMeshThreadgroupSize(
                shaderAssetString.taskShader->threadgroupSizeX,
                shaderAssetString.taskShader->threadgroupSizeY,
                shaderAssetString.taskShader->threadgroupSizeZ);
        }
        
        if (shaderAssetString.meshShader->threadgroupSizeX > 0)
        {
            graphicsShader->SetMeshThreadgroupSize(
                shaderAssetString.meshShader->threadgroupSizeX,
                shaderAssetString.meshShader->threadgroupSizeY,
                shaderAssetString.meshShader->threadgroupSizeZ);
        }
        if (shaderAssetString.taskShader && shaderAssetString.taskShader->threadgroupSizeX > 0)
        {
            graphicsShader->SetTaskThreadgroupSize(
                shaderAssetString.taskShader->threadgroupSizeX,
                shaderAssetString.taskShader->threadgroupSizeY,
                shaderAssetString.taskShader->threadgroupSizeZ);
        }

        graphicsShaderInfo.graphicsShader = graphicsShader;
        
        // Mesh Pipeline 不需要顶点描述
        graphicsShaderInfo.graphicsPipelineDesc.pipelineType = PipelineType::Mesh;
    }
    else
    {
        // 传统图形管线：Vertex + Fragment
        ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
        ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
        
        GraphicsShaderPtr graphicsShader = GetRenderDevice()->CreateGraphicsShader(*vertexShader, *fragmentShader);
        graphicsShaderInfo.graphicsShader = graphicsShader;
        
        graphicsShaderInfo.graphicsPipelineDesc.vertexDescriptor = std::move(shaderAssetString.vertexDescriptor);
    }
    
    return graphicsShaderInfo;
}

NS_RENDERSYSTEM_END
