//
//  BuildSetting.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/20.
//

#include "BuildSetting.h"
#include "Runtime/RenderCore/include/RenderDefine.h"
#include "Runtime/ShaderCompiler/include/ShaderCompilerDefine.h"
#include "Runtime/BaseLib/include/LogService.h"

NS_RENDERSYSTEM_BEGIN

// 静态初始化：将 BuildSetting 的配置同步到底层模块
// 这确保了正确的依赖方向：RenderSystem -> RenderCore / ShaderCompiler
namespace
{
    struct ConfigSynchronizer
    {
        ConfigSynchronizer()
        {
            // 同步 Reverse-Z 配置到 RenderCore 层
            RenderCore::DepthConfig::UseReverseZ = BuildSetting::mUseReverseZ;
            
            // 同步 Reverse-Z 配置到 ShaderCompiler 层
            shader_compiler::ShaderCompilerConfig::UseReverseZ = BuildSetting::mUseReverseZ;

            LOG_INFO("Reverse-Z: BuildSetting=%d, DepthConfig=%d, ShaderConfig=%d",
                BuildSetting::mUseReverseZ,
                RenderCore::DepthConfig::UseReverseZ,
                shader_compiler::ShaderCompilerConfig::UseReverseZ);
        }
    };
    static ConfigSynchronizer sConfigSynchronizer;
}

NS_RENDERSYSTEM_END
