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

// 导出宏：ShaderCompiler 是 OBJECT 库，符号进入 GNXEngine SHARED dll，
// 供 tool/demo 等外部可执行文件链接使用。
// 与 ASSET_MANAGER_API 一致：Windows 下无条件 dllexport（OBJECT 库在 dll 内）。
#if defined(_WIN32) || defined(_WIN64)
    #ifdef __GNUC__
        #define SHADERCOMPILER_API __attribute__((dllexport))
    #else
        #define SHADERCOMPILER_API __declspec(dllexport)
    #endif
#else
    #define SHADERCOMPILER_API
#endif

NAMESPACE_SHADERCOMPILER_BEGIN

// ShaderCompiler 配置
// 这些配置由上层 RenderSystem 的 BuildSetting 在初始化时同步
struct ShaderCompilerConfig
{
    static bool UseReverseZ;
};

NAMESPACE_SHADERCOMPILER_END

#endif /* ShaderCompilerDefine_hpp */
