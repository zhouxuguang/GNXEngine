//
//  ShaderCompilerDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/10.
//

#ifndef ShaderCompilerDefine_hpp
#define ShaderCompilerDefine_hpp

#include <stdio.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <memory>
#include "Runtime/RenderCore/include/RenderDefine.h"

using namespace RenderCore;

#define NAMESPACE_SHADERCOMPILER_BEGIN        namespace shader_compiler{
/** namespace结束宏 */
#define NAMESPACE_SHADERCOMPILER_END            }

NAMESPACE_SHADERCOMPILER_BEGIN

// ShaderCompiler 配置
// 这些配置由上层 RenderSystem 的 BuildSetting 在初始化时同步
struct ShaderCompilerConfig
{
    static bool UseReverseZ;
};

NAMESPACE_SHADERCOMPILER_END

#endif /* ShaderCompilerDefine_hpp */
