//
//  BuildSetting.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/20.
//

#include "BuildSetting.h"
#include "Runtime/RenderCore/include/RenderDefine.h"

NS_RENDERSYSTEM_BEGIN

// 静态初始化：将 BuildSetting 的配置同步到 RenderCore 层
// 这确保了正确的依赖方向：RenderSystem -> RenderCore
namespace
{
    struct ConfigSynchronizer
    {
        ConfigSynchronizer()
        {
            // 同步 Reverse-Z 配置到底层
            RenderCore::DepthConfig::UseReverseZ = BuildSetting::mUseReverseZ;
        }
    };
    static ConfigSynchronizer sConfigSynchronizer;
}

NS_RENDERSYSTEM_END
