//
//  ShaderCompiler.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/9.
//

#ifndef SHADER_COMPILER_INCLUDE_SFH
#define SHADER_COMPILER_INCLUDE_SFH

#include "ShaderCompilerDefine.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"

NAMESPACE_SHADERCOMPILER_BEGIN

//typedef std::shared_ptr<std::vector<uint32_t>> ShaderCodePtr;

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

// 单个 push constant block 的元数据（从 SPIR-V 编译阶段收集）
struct CompiledPushConstantInfo
{
    std::string name;     // cbuffer 名称
    uint32_t size = 0;    // padded_size（已对齐）
    uint32_t set = 0;     // 原始 descriptor set（用于按 index 绑定时的反向查表）
    uint32_t binding = 0; // 原始 descriptor binding
};

// 编译后的shader信息以及一些反射的元数据信息
struct CompiledShaderInfo
{
    ShaderCodePtr shaderSource = nullptr;
    RenderCore::VertexDesc vertexDescriptor;

    // mesh/task shader 的 threadgroup 大小（来自 SPIR-V LocalSize）
    uint32_t threadgroupSizeX = 0;
    uint32_t threadgroupSizeY = 0;
    uint32_t threadgroupSizeZ = 0;
    
    // 从 UBO 转换为 push constant 的 cbuffer 列表
    // 在 CompileShader 中由 patchUniformToPushConstant 填充
    std::vector<CompiledPushConstantInfo> pushConstants;
};

using CompiledShaderInfoPtr = std::shared_ptr<CompiledShaderInfo>;

ShaderCode compileToESSL30(ShaderCodePtr spirvCode, ShaderStage shaderStage);

CompiledShaderInfoPtr compileToMSL(ShaderCodePtr spirvCode, ShaderStage shaderStage);

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType);

CompiledShaderInfoPtr CompileShader(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType);

NAMESPACE_SHADERCOMPILER_END

#endif /* SHADER_COMPILER_INCLUDE_SFH */
