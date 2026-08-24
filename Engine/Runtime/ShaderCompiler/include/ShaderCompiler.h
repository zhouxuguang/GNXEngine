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
#include "Runtime/RenderCore/include/ShaderStageData.h"

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

// CompiledPushConstantInfo moved to RenderCore (ShaderStageData.h).
// Alias keeps shader_compiler::CompiledPushConstantInfo usable so AssetManager
// does not depend on ShaderCompiler.h.
using RenderCore::CompiledPushConstantInfo;

// 编译后的shader信息以及一些反射的元数据信息
struct CompiledShaderInfo
{
    ShaderCodePtr shaderSource = nullptr;
    RenderCore::VertexDesc vertexDescriptor;

    // Target format (records compile target, for packaging/runtime validation)
    RenderCore::ShaderFormat format = RenderCore::ShaderFormat_SPIRV;

    // mesh/task shader 的 threadgroup 大小（来自 SPIR-V LocalSize）
    uint32_t threadgroupSizeX = 0;
    uint32_t threadgroupSizeY = 0;
    uint32_t threadgroupSizeZ = 0;
    
    // 从 UBO 转换为 push constant 的 cbuffer 列表
    // 在 CompileShader 中由 patchUniformToPushConstant 填充
    std::vector<CompiledPushConstantInfo> pushConstants;
};

using CompiledShaderInfoPtr = std::shared_ptr<CompiledShaderInfo>;

SHADERCOMPILER_API ShaderCode compileToESSL30(ShaderCodePtr spirvCode, ShaderStage shaderStage);

SHADERCOMPILER_API CompiledShaderInfoPtr compileToMSL(ShaderCodePtr spirvCode, ShaderStage shaderStage, RenderCore::ShaderFormat targetFormat);

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType);

SHADERCOMPILER_API CompiledShaderInfoPtr CompileShader(const std::string& shaderFile, ShaderStage shaderStage, RenderCore::ShaderFormat targetFormat);

NAMESPACE_SHADERCOMPILER_END

#endif /* SHADER_COMPILER_INCLUDE_SFH */
