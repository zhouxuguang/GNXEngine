//
//  MotionBlurPass.h
//  GNXEngine
//
//  Motion Blur Post-Processing Pass
//

#ifndef GNXENGINE_MOTION_BLUR_PASS_H
#define GNXENGINE_MOTION_BLUR_PASS_H

#include "RSDefine.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include <memory>

NS_RENDERSYSTEM_BEGIN

struct MotionBlurParams
{
    uint32_t width;
    uint32_t height;
    
    FrameGraphResource colorTexture;
    FrameGraphResource depthTexture;
    
    UniformBufferPtr cameraUBO = nullptr;
};

struct MotionBlurOutput
{
    FrameGraphResource result;
};

struct MotionBlurConfig
{
};

class RENDERSYSTEM_API MotionBlurPass
{
public:
    MotionBlurPass();
    ~MotionBlurPass();
    
    bool Initialize(const MotionBlurConfig& config);
    
    MotionBlurOutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const MotionBlurParams& params);
    
    bool IsInitialized() const { return mInitialized; }

private:
    void CreateMotionBlurPipeline();

private:
    MotionBlurConfig mConfig;
    
    GraphicsPipelinePtr mMotionBlurPipeline = nullptr;
    
    TextureSamplerPtr mColorSampler = nullptr;
    TextureSamplerPtr mDepthSampler = nullptr;
    
    bool mInitialized = false;
    
    uint32_t mWidth = 1;
    uint32_t mHeight = 1;
};

typedef std::shared_ptr<MotionBlurPass> MotionBlurPassPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MOTION_BLUR_PASS_H
