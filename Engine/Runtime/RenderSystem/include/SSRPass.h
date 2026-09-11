//
//  SSRPass.h
//  GNXEngine
//
//  Screen-space reflection tracing and composition pass.
//

#ifndef GNXENGINE_SSR_PASS_H
#define GNXENGINE_SSR_PASS_H

#include "RSDefine.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include <memory>

NS_RENDERSYSTEM_BEGIN

struct SSRConfig
{
    float maxRayDistance = 80.0f;
    float stepLength = 0.35f;
    float thickness = 0.35f;
    int maxSteps = 128;
    int binarySearchSteps = 5;
    float edgeFade = 0.12f;
    float maxRoughness = 0.8f;
    float intensity = 1.0f;
};

struct SSRParams
{
    uint32_t width = 1;
    uint32_t height = 1;
    FrameGraphResource sceneColor = -1;
    FrameGraphResource gBufferA = -1; // encoded world normal + roughness
    FrameGraphResource gBufferB = -1; // metallic + perceptual roughness
    FrameGraphResource gBufferC = -1; // albedo
    FrameGraphResource depthTexture = -1;
    UniformBufferPtr cameraUBO = nullptr;
};

struct SSROutput
{
    FrameGraphResource result = -1;
};

class RENDERSYSTEM_API SSRPass
{
public:
    bool Initialize(const SSRConfig& config = SSRConfig());

    SSROutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const SSRParams& params);

    bool IsInitialized() const { return mInitialized; }

private:
    void CreatePipelines();

    SSRConfig mConfig;
    GraphicsPipelinePtr mTracePipeline = nullptr;
    GraphicsPipelinePtr mCompositePipeline = nullptr;
    TextureSamplerPtr mPointSampler = nullptr;
    TextureSamplerPtr mLinearSampler = nullptr;
    UniformBufferPtr mParamsUBO = nullptr;
    bool mInitialized = false;
    uint32_t mWidth = 1;
    uint32_t mHeight = 1;
};

using SSRPassPtr = std::shared_ptr<SSRPass>;

NS_RENDERSYSTEM_END

#endif
