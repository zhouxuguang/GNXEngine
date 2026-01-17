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

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

DepthRenderer::DepthRenderer(RenderDevice* device)
    : mDevice(device)
{
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
    // 智能指针会自动释放资源
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
