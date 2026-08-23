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
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/AssetManager/include/ShaderMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>

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
// 优先通过 AssetManager 统一加载（缓存 + 引用计数，用后 Release）
// 若 AssetManager 未初始化（如 SceneManager 早期初始化时），回退局部加载保证功能

// 从已加载的 ShaderAsset 构造 CompiledShaderInfo（shaderSource + threadgroup + 反射）
static CompiledShaderInfoPtr BuildCompiledShaderInfo(const AssetManager::ShaderAsset& shaderAsset,
                                                     uint32_t shaderStage)
{
    const ShaderMessage& msg = shaderAsset.GetShaderMessage();

    // 校验阶段匹配
    if (static_cast<uint32_t>(msg.shaderStage) != shaderStage)
    {
        LOG_WARN("LoadCompiledShader: stage mismatch (expected %u, got %u)", shaderStage, (uint32_t)msg.shaderStage);
        return nullptr;
    }

    // 构造 CompiledShaderInfo
    auto info = std::make_shared<CompiledShaderInfo>();
    info->shaderSource = std::make_shared<ShaderCode>();
    if (shaderAsset.GetShaderData() && shaderAsset.GetShaderDataSize() > 0)
    {
        info->shaderSource->resize(shaderAsset.GetShaderDataSize());
        memcpy(info->shaderSource->data(), shaderAsset.GetShaderData(), shaderAsset.GetShaderDataSize());
    }

    info->threadgroupSizeX = msg.threadgroupSizeX;
    info->threadgroupSizeY = msg.threadgroupSizeY;
    info->threadgroupSizeZ = msg.threadgroupSizeZ;

    // 反序列化顶点描述符（Metal 重建 MTLVertexDescriptor 必需）
    // 从 .gnxasset 的 vertexInputs（含 stride）重建 RenderCore::VertexDesc
    const auto* vertexInputs = shaderAsset.GetVertexInputs();
    if (vertexInputs && !vertexInputs->empty())
    {
        RenderCore::VertexDesc& vd = info->vertexDescriptor;
        vd.attributes.clear();
        vd.layouts.clear();
        vd.attributes.reserve(vertexInputs->size());
        vd.layouts.reserve(vertexInputs->size());

        for (const auto& vi : *vertexInputs)
        {
            RenderCore::VertextAttributesDesc attr;
            attr.index  = vi.location;
            attr.format = static_cast<RenderCore::VertexFormat>(static_cast<uint32_t>(vi.format));
            attr.offset = vi.offset;
            vd.attributes.push_back(attr);

            RenderCore::VertexBufferLayoutDesc layout;
            layout.stride = vi.stride;
            layout.stepRate = 1;
            layout.stepFunction = RenderCore::VertexStepFunctionPerVertex;
            vd.layouts.push_back(layout);
        }
    }

    // 反序列化 push constants（Vulkan 管线布局需要；当前预编译产物恒空，为将来启用预留）
    const auto* pushConstants = shaderAsset.GetPushConstants();
    if (pushConstants)
    {
        info->pushConstants.clear();
        for (const auto& pcm : *pushConstants)
        {
            CompiledPushConstantInfo pc;
            pc.name    = "";   // nanopb string 解码为 pb_bytes，暂不保留 name（运行时按 set/binding 路由）
            pc.size    = pcm.size;
            pc.set     = pcm.set;
            pc.binding = pcm.binding;
            info->pushConstants.push_back(pc);
        }
    }

    return info;
}

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

    AssetManager::AssetManager* assetMgr = AssetManager::AssetManager::GetInstance();
    if (!assetMgr)
    {
        LOG_WARN("LoadCompiledShader: AssetManager not initialized, local fallback: %s", filePath.c_str());
        AssetManager::ShaderAsset localAsset;
        if (!localAsset.LoadFromFile(filePath))
        {
            LOG_WARN("LoadCompiledShader: asset not found: %s", filePath.c_str());
            return nullptr;
        }
        return BuildCompiledShaderInfo(localAsset, shaderStage);
    }

    AssetManager::ShaderAsset* shaderAsset = assetMgr->LoadShader(filePath);
    if (!shaderAsset)
    {
        LOG_WARN("LoadCompiledShader: asset not found: %s", filePath.c_str());
        return nullptr;
    }

    CompiledShaderInfoPtr info = BuildCompiledShaderInfo(*shaderAsset, shaderStage);

    LOG_INFO("LoadCompiledShader: loaded %s (%u bytes)", filePath.c_str(), shaderAsset->GetShaderDataSize());

    // 数据已拷贝到 CompiledShaderInfo，ShaderAsset 不再需要长期持有
    // Release 使引用计数归零，可由 AssetManager::UnloadUnusedAssets 回收
    shaderAsset->Release();

    return info;
}

// 平台 + 格式后缀映射
// 由具体后端决定：Metal → msl_ios/msl_macos，Vulkan → spirv

// 检查一个 shader 的必需 stage 预编译产物是否齐全
// 传统管线需要 vs+ps；mesh 管线需要 ms+ps（TerrainMS/TerrainMSDepth/MeshShaderDemo）
// 纯 compute 管线只需 cs（HiZGeneration/TerrainCull/TestADD 等）
// 任一必需 stage 缺失则整体回退运行时编译（避免混合来源）
static bool HasRequiredGraphicsStages(const std::string& shaderName, const std::string& formatSuffix)
{
    static const char* stageSuffixes[] = { "vs", "ps", "cs", "ts", "ms" };

    // mesh 管线：ms + ps
    std::string msFile = getCompiledShaderDir() + shaderName + ".ms." + formatSuffix + ".gnxasset";
    std::ifstream msF(msFile, std::ios::binary);
    if (msF.good())
    {
        // mesh 管线要求 ms + ps
        std::string psFile = getCompiledShaderDir() + shaderName + ".ps." + formatSuffix + ".gnxasset";
        std::ifstream psF(psFile, std::ios::binary);
        return psF.good();
    }

    // 传统管线：vs + ps
    std::string vsFile = getCompiledShaderDir() + shaderName + ".vs." + formatSuffix + ".gnxasset";
    std::ifstream vsF(vsFile, std::ios::binary);
    if (vsF.good())
    {
        std::string psFile = getCompiledShaderDir() + shaderName + ".ps." + formatSuffix + ".gnxasset";
        std::ifstream psF(psFile, std::ios::binary);
        return psF.good();
    }

    // 纯 compute 管线：只需 cs（无 vs/ps/ms）
    std::string csFile = getCompiledShaderDir() + shaderName + ".cs." + formatSuffix + ".gnxasset";
    std::ifstream csF(csFile, std::ios::binary);
    return csF.good();
}

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;

    if (!GetRenderDevice())
    {
        return shaderAssetString;
    }

    RenderDeviceType renderType = GetRenderDevice()->GetRenderDeviceType();
    std::string formatSuffix = GetShaderFormatSuffix(renderType);

    // 全平台统一从 data_asset/Shader/ 加载预编译产物
    // fallback: 若必需图形 stage 预编译产物缺失（开发期尚未跑批量编译），整体回退运行时编译
    if (!HasRequiredGraphicsStages(shaderName, formatSuffix))
    {
        LOG_WARN("LoadShaderAsset: precompiled shader missing for %s (format %s), falling back to runtime compile",
                 shaderName.c_str(), formatSuffix.c_str());
        std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
        return LoadCustomShaderAsset(shaderFilePath);
    }

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
