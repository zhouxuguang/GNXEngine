//
//  DepthRenderer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/11.
//

#include "DepthRenderer.h"
#include <cmath>
#include <algorithm>
#include <limits>

#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

DepthRenderer::DepthRenderer(RenderDevice* device) : mDevice(device)
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("DepthGenerate");
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
    
    //ComputePipelinePtr computePipeline = GetRenderDevice()->creategr(*computeShader);
}

DepthRenderer::~DepthRenderer()
{
    // 智能指针会自动释放资源
}

bool DepthRenderer::Initialize(const DepthRenderConfig& config)
{
    if (!mDevice)
    {
        return false;
    }

    mConfig = config;

    // 创建主深度纹理（用于Depth Pre-Pass）
    mDepthTexture = CreateDepthTexture(mConfig.width, mConfig.height, mConfig.depthFormat);
    if (!mDepthTexture)
    {
        return false;
    }

    mInitialized = true;
    return true;
}

void DepthRenderer::Shutdown()
{
    mDepthTexture = nullptr;
    mInitialized = false;
}

RCTexturePtr DepthRenderer::GetDepthTexture() const
{
    return mDepthTexture;
}

void DepthRenderer::UpdateConfig(const DepthRenderConfig& config)
{
    if (config.width != mConfig.width || config.height != mConfig.height ||
        config.depthFormat != mConfig.depthFormat)
    {
        // 需要重新创建纹理
        Shutdown();
        Initialize(config);
    }
}

const DepthRenderConfig& DepthRenderer::GetConfig() const
{
    return mConfig;
}

RCTexturePtr DepthRenderer::CreateDepthTexture(uint32_t width, uint32_t height, TextureFormat format)
{
    // 使用RenderDevice的CreateTexture2D方法创建深度纹理
    RCTexture2DPtr texture = mDevice->CreateTexture2D(format,
                                        RenderCore::TextureUsage::TextureUsageRenderTarget | RenderCore::TextureUsage::TextureUsageShaderRead,
                                        width, height, 1);
    return texture;
}

NS_RENDERSYSTEM_END
