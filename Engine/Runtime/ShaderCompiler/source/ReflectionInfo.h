//
//  ReflectionInfo.h
//  shadercompiler
//
//  Created by zhouxuguang on 2023/9/24.
//

#ifndef GNX_ENGINE_RELECTION_INFO_INCLUDE_H
#define GNX_ENGINE_RELECTION_INFO_INCLUDE_H

#include "ShaderCompilerDefine.h"
#include "RenderCore/RenderDescriptor.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_msl.hpp"
#include "ShaderCompiler.h"

NAMESPACE_SHADERCOMPILER_BEGIN

UniformBuffersLayout GetMetalUniformReflectionInfo(const spirv_cross::CompilerMSL& msl, const spirv_cross::ShaderResources& resources);

VertexDescriptor GetMetalReflectionInfo(const spirv_cross::CompilerMSL& msl, const spirv_cross::ShaderResources& resources);

NAMESPACE_SHADERCOMPILER_END

#endif /* GNX_ENGINE_RELECTION_INFO_INCLUDE_H */
