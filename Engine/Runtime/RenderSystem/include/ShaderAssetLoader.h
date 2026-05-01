//
//  ShaderAssetLoader.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#ifndef GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H
#define GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"
#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"

NS_RENDERSYSTEM_BEGIN

using namespace shader_compiler;

struct ShaderString
{
    ShaderCode vertexShader;
    ShaderCode fragmentShader;
    ShaderCode computeShader;
    ShaderCode taskShader;
    ShaderCode meshShader;
};

struct ShaderAssetString
{
    CompiledShaderInfoPtr vertexShader;
    CompiledShaderInfoPtr fragmentShader;
    CompiledShaderInfoPtr computeShader;
    CompiledShaderInfoPtr taskShader;
    CompiledShaderInfoPtr meshShader;

    VertexDesc vertexDescriptor;                             // 顶点描述
    shader_compiler::UniformBuffersLayout vertexUniformBufferLayout;  //顶点ubo信息
    shader_compiler::UniformBuffersLayout fragmentUniformBufferLayout; //片元ubo信息
};

struct GraphicsShaderInfo
{
    RenderCore::GraphicsPipelineDesc graphicsPipelineDesc;
    RenderCore::GraphicsShaderPtr graphicsShader;
};

RENDERSYSTEM_API ShaderAssetString LoadShaderAsset(const std::string &shaderName);

RENDERSYSTEM_API ShaderAssetString LoadCustomShaderAsset(const std::string &shaderName);

RENDERCORE_API GraphicsShaderInfo CreateGraphicsShaderInfo(const std::string& shaderName);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H */
