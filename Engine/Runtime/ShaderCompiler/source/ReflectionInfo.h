//
//  ReflectionInfo.h
//  shadercompiler
//
//  Created by zhouxuguang on 2023/9/24.
//

#ifndef GNX_ENGINE_RELECTION_INFO_INCLUDE_H
#define GNX_ENGINE_RELECTION_INFO_INCLUDE_H

#include "ShaderCompilerDefine.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_msl.hpp"
#include "ShaderCompiler.h"

NAMESPACE_SHADERCOMPILER_BEGIN

UniformBuffersLayout GetMetalUniformReflectionInfo(const spirv_cross::CompilerMSL& msl, const spirv_cross::ShaderResources& resources);

// 顶点描述反射：只使用 spirv_cross::Compiler 的基类接口，
// 因此 MSL 与 HLSL 后端共用同一份实现（HLSL 是 DX12 链路的一环）。
VertexDesc GetMetalReflectionInfo(const spirv_cross::Compiler& msl, const spirv_cross::ShaderResources& resources);

NAMESPACE_SHADERCOMPILER_END

#endif /* GNX_ENGINE_RELECTION_INFO_INCLUDE_H */
