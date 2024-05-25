//
//  ShaderCompiler.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/9.
//

#ifndef SHADER_COMPILER_INCLUDE_SFH
#define SHADER_COMPILER_INCLUDE_SFH

#include "ShaderCompilerDefine.h"
#include "RenderCore/RenderDescriptor.h"

NAMESPACE_SHADERCOMPILER_BEGIN

typedef std::shared_ptr<std::vector<uint32_t>> ShaderCodePtr;

//uniform buffer每一个成员的布局
struct UniformMember
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    VertexFormat format;
};

struct UniformLayout
{
    std::vector<UniformMember> members;   //所有uniform成员的布局
    uint32_t dataSize;                 //uniform buffer的大小，字节
    std::string name;                  //名称
};

typedef std::vector<UniformLayout> UniformBuffersLayout;

struct CompiledShaderInfo
{
    std::string shaderSource;
    RenderCore::VertexDescriptor vertexDescriptor;
    UniformBuffersLayout vertexUniformBufferLayout;
    UniformBuffersLayout fragmentUniformBufferLayout;
};

std::string compileToESSL30(const std::vector<uint32_t>& spirvCode, ShaderStage shaderStage);

CompiledShaderInfo compileToMSL(const std::vector<uint32_t>& spirvCode, ShaderStage shaderStage);

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage);

NAMESPACE_SHADERCOMPILER_END

#endif /* SHADER_COMPILER_INCLUDE_SFH */