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

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    
    std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".shader";
    
    return LoadCustomShaderAsset(shaderFilePath);
}

ShaderAssetString LoadCustomShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    
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
        graphicsShaderInfo.graphicsShader = graphicsShader;
        
        // Mesh Pipeline 不需要顶点描述
        graphicsShaderInfo.graphicsPipelineDesc.pipelineType = PipelineType::Mesh;
        
        // 从 mesh shader 的编译信息中提取 threadgroup 大小
        if (shaderAssetString.meshShader->threadgroupSizeX > 0)
        {
            graphicsShaderInfo.graphicsPipelineDesc.meshThreadgroupSizeX = shaderAssetString.meshShader->threadgroupSizeX;
            graphicsShaderInfo.graphicsPipelineDesc.meshThreadgroupSizeY = shaderAssetString.meshShader->threadgroupSizeY;
            graphicsShaderInfo.graphicsPipelineDesc.meshThreadgroupSizeZ = shaderAssetString.meshShader->threadgroupSizeZ;
        }
        
        // 从 task shader 的编译信息中提取 threadgroup 大小
        if (shaderAssetString.taskShader && shaderAssetString.taskShader->threadgroupSizeX > 0)
        {
            graphicsShaderInfo.graphicsPipelineDesc.taskThreadgroupSizeX = shaderAssetString.taskShader->threadgroupSizeX;
            graphicsShaderInfo.graphicsPipelineDesc.taskThreadgroupSizeY = shaderAssetString.taskShader->threadgroupSizeY;
            graphicsShaderInfo.graphicsPipelineDesc.taskThreadgroupSizeZ = shaderAssetString.taskShader->threadgroupSizeZ;
        }
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
