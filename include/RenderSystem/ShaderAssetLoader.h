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
#include "ShaderCompiler.h"

NS_RENDERSYSTEM_BEGIN

struct ShaderString
{
    std::string vertexShaderStr;
    std::string fragmentShaderStr;
    std::string computeShaderStr;
};

struct ShaderAssetString
{
    ShaderString gles30Shader;
    ShaderString gles31Shader;
    ShaderString gles32Shader;
    ShaderString metalShader;
    
    VertexDescriptor vertexDescriptor;
    
    shader_compiler::UniformBuffersLayout vertexUniformBufferLayout;
    shader_compiler::UniformBuffersLayout fragmentUniformBufferLayout;
};

ShaderAssetString LoadShaderAsset(const std::string &shaderName);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H */
