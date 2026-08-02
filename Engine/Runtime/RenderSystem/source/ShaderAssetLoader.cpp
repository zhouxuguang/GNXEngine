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
#include "Runtime/AssetManager/include/ShaderAsset.h"
#include "Runtime/AssetManager/include/ShaderMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

// ============================================================================
// 内部辅助：从 .gnxasset 加载预编译 shader
// ============================================================================

// 根据渲染设备类型返回对应的 ShaderFormat 后缀
static std::string GetShaderFormatSuffix(RenderDeviceType renderType)
{
    switch (renderType)
    {
        case RenderDeviceType::METAL:
#if TARGET_OS_OSX
            return "msl_macos";
#else
            return "msl_ios";
#endif
        case RenderDeviceType::VULKAN:
            return "spirv";
        default:
            return "spirv";
    }
}

// 从 ShaderAsset（.gnxasset 文件）构造 CompiledShaderInfo
static CompiledShaderInfoPtr LoadCompiledShaderFromAsset(const std::string& shaderName,
                                                          uint32_t shaderStage,
                                                          const std::string& formatSuffix)
{
    // 文件命名：{shaderName}.{stage}.{format}.gnxasset
    // 例如: GBufferPBR.vs.spirv.gnxasset / GBufferPBR.vs.msl_ios.gnxasset
    static const char* stageSuffixes[] = { "vs", "ps", "cs", "ts", "ms" };

    std::string filePath = getCompiledShaderDir()
        + shaderName + "."
        + stageSuffixes[shaderStage] + "."
        + formatSuffix + ".gnxasset";

    AssetManager::ShaderAsset shaderAsset;
    if (!shaderAsset.LoadFromFile(filePath))
    {
        LOG_WARN("LoadCompiledShader: asset not found: %s", filePath.c_str());
        return nullptr;
    }

    const ShaderMessage& msg = shaderAsset.GetShaderMessage();

    // 校验阶段匹配
    if (static_cast<uint32_t>(msg.shaderStage) != shaderStage)
    {
        LOG_WARN("LoadCompiledShader: stage mismatch in %s", filePath.c_str());
        return nullptr;
    }

    // 构造 CompiledShaderInfo
    auto info = std::make_shared<CompiledShaderInfo>();
    info->shaderSource = std::make_shared<ShaderCode>();
    info->shaderSource->resize(shaderAsset.GetShaderDataSize());
    memcpy(info->shaderSource->data(), shaderAsset.GetShaderData(), shaderAsset.GetShaderDataSize());

    info->threadgroupSizeX = msg.threadgroupSizeX;
    info->threadgroupSizeY = msg.threadgroupSizeY;
    info->threadgroupSizeZ = msg.threadgroupSizeZ;

    LOG_INFO("LoadCompiledShader: loaded %s (%u bytes)", filePath.c_str(), shaderAsset.GetShaderDataSize());
    return info;
}

// 平台 + 格式后缀映射
// 由具体后端决定：Metal → msl_ios/msl_macos，Vulkan → spirv
// 这里先提供一个默认映射，后续可根据需要扩展

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    
#if 1
    std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
    
    return LoadCustomShaderAsset(shaderFilePath);
#endif

    if (!GetRenderDevice())
    {
        return shaderAssetString;
    }

    RenderDeviceType renderType = GetRenderDevice()->GetRenderDeviceType();
    std::string formatSuffix = GetShaderFormatSuffix(renderType);

    // 全平台统一从 data_asset/Shader/ 加载预编译产物
    shaderAssetString.vertexShader = LoadCompiledShaderFromAsset(shaderName, ShaderStage_Vertex, formatSuffix);
    if (shaderAssetString.vertexShader)
    {
        shaderAssetString.vertexDescriptor = shaderAssetString.vertexShader->vertexDescriptor;
    }

    shaderAssetString.fragmentShader = LoadCompiledShaderFromAsset(shaderName, ShaderStage_Fragment, formatSuffix);
    shaderAssetString.computeShader   = LoadCompiledShaderFromAsset(shaderName, ShaderStage_Compute, formatSuffix);
    shaderAssetString.taskShader      = LoadCompiledShaderFromAsset(shaderName, ShaderStage_Task, formatSuffix);
    shaderAssetString.meshShader      = LoadCompiledShaderFromAsset(shaderName, ShaderStage_Mesh, formatSuffix);

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
    
    CompiledShaderInfoPtr vertexShaderInfo = CompileShader(shaderName, ShaderStage_Vertex, renderType);
    
    if (vertexShaderInfo)
    {
        shaderAssetString.vertexShader = vertexShaderInfo;
        shaderAssetString.vertexDescriptor = vertexShaderInfo->vertexDescriptor;
    }
    
    CompiledShaderInfoPtr fragmentShaderInfo = CompileShader(shaderName, ShaderStage_Fragment, renderType);
    if (fragmentShaderInfo)
    {
        shaderAssetString.fragmentShader = fragmentShaderInfo;
    }
    
    CompiledShaderInfoPtr computeShaderInfo = CompileShader(shaderName, ShaderStage_Compute, renderType);
    if (computeShaderInfo)
    {
        shaderAssetString.computeShader = computeShaderInfo;
    }
    
    CompiledShaderInfoPtr taskShaderInfo = CompileShader(shaderName, ShaderStage_Task, renderType);
    if (taskShaderInfo)
    {
        shaderAssetString.taskShader = taskShaderInfo;
    }
    
    CompiledShaderInfoPtr meshShaderInfo = CompileShader(shaderName, ShaderStage_Mesh, renderType);
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
