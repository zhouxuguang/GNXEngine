//
//  FXAAPass.h
//  GNXEngine
//
//  FXAA 抗锯齿后处理 Pass
//

#ifndef GNX_ENGINE_FXAA_PASS_INCLUDE_H
#define GNX_ENGINE_FXAA_PASS_INCLUDE_H

#include "../RSDefine.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/RenderCore/include/TextureSampler.h"

NS_RENDERSYSTEM_BEGIN

/**
 * 输入为场景 HDR 颜色，shader 内部先做色调映射到 SDR 显示空间再执行 FXAA，
 * 最终绘制到调用方当前编码的目标（Present 阶段即交换链图像）。
 */
class RENDERSYSTEM_API FXAAPass
{
public:
    FXAAPass();

    ~FXAAPass();

    /// 创建管线与采样器，可重复调用
    bool Initialize();

    bool IsInitialized() const { return mInitialized; }

    /// @param renderEncoder 目标编码器（其附件即输出）
    /// @param inputTexture  场景 HDR 颜色纹理
    void Process(const RenderEncoderPtr& renderEncoder, RCTexturePtr inputTexture);

private:
    GraphicsPipelinePtr mPipeline = nullptr;
    TextureSamplerPtr mSampler = nullptr;
    bool mInitialized = false;
};

typedef std::shared_ptr<FXAAPass> FXAAPassPtr;

NS_RENDERSYSTEM_END

#endif // GNX_ENGINE_FXAA_PASS_INCLUDE_H
