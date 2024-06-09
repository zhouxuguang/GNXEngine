//
//  ShaderAssetLoader.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#ifndef GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H
#define GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H

#include "RSDefine.h"
#include "RenderCore/RenderDescriptor.h"
#include "RenderCore/ShaderFunction.h"
#include "ShaderCompiler.h"

NS_RENDERSYSTEM_BEGIN

using namespace shader_compiler;

struct ShaderString
{
    ShaderCode vertexShader;
    ShaderCode fragmentShader;
    ShaderCode computeShader;
};

struct ShaderAssetString
{
    CompiledShaderInfoPtr vertexShader;
    CompiledShaderInfoPtr fragmentShader;
    CompiledShaderInfoPtr computeShader;
//    ShaderString gles31Shader;
//    ShaderString gles32Shader;
//    ShaderString metalShader;
//    ShaderString vulkanShader;
    
    VertexDescriptor vertexDescriptor;                             // 顶点描述
    shader_compiler::UniformBuffersLayout vertexUniformBufferLayout;  //顶点ubo信息
    shader_compiler::UniformBuffersLayout fragmentUniformBufferLayout; //片元ubo信息
};

ShaderAssetString LoadShaderAsset(const std::string &shaderName);

ShaderAssetString LoadCustomShaderAsset(const std::string &shaderName);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H */
