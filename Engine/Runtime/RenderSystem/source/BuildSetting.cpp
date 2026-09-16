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

namespace
{
    struct ConfigSynchronizer
    {
        ConfigSynchronizer()
        {
            // 同步 Reverse-Z 配置到 RenderCore 层
            RenderCore::DepthConfig::UseReverseZ = BuildSetting::mUseReverseZ;

            shader_compiler::ShaderCompilerConfig::UseReverseZ = BuildSetting::mUseReverseZ;

            LOG_INFO("Reverse-Z: BuildSetting=%d, DepthConfig=%d, ShaderConfig=%d",
                BuildSetting::mUseReverseZ, RenderCore::DepthConfig::UseReverseZ,
                shader_compiler::ShaderCompilerConfig::UseReverseZ);
        }
    };
    static ConfigSynchronizer sConfigSynchronizer;
}

NS_RENDERSYSTEM_END
