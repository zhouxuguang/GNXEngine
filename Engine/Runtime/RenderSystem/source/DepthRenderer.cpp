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

    // 创建级联阴影贴图（仅适用于DirectionalLight）
    if (mConfig.cascadeCount > 0)
    {
        mCascadeShadowMaps.resize(mConfig.cascadeCount);
        mCascadeViewMatrices.resize(mConfig.cascadeCount);
        mCascadeProjMatrices.resize(mConfig.cascadeCount);
        mCascadeSplitDistances.resize(mConfig.cascadeCount + 1);

        for (uint32_t i = 0; i < mConfig.cascadeCount; i++)
        {
            mCascadeShadowMaps[i] = CreateDepthTexture(mConfig.width, mConfig.height,
                                                       mConfig.depthFormat);
            if (!mCascadeShadowMaps[i])
            {
                Shutdown();
                return false;
            }
        }
    }

    // 创建立方体阴影贴图（仅适用于PointLight）
    mCubeShadowMaps.resize(6);
    mCubeLightViewMatrices.resize(6);

    for (int i = 0; i < 6; i++)
    {
        mCubeShadowMaps[i] = CreateDepthTexture(mConfig.width, mConfig.height,
                                                  mConfig.depthFormat);
        if (!mCubeShadowMaps[i])
        {
            Shutdown();
            return false;
        }
    }

    // 计算级联分割
    CalculateCascadeSplits(mConfig.nearPlane, mConfig.farPlane);

    mInitialized = true;
    return true;
}

void DepthRenderer::Shutdown()
{
    // 智能指针会自动释放资源
    mDepthTexture = nullptr;
    mCascadeShadowMaps.clear();
    mCubeShadowMaps.clear();

    mCascadeViewMatrices.clear();
    mCascadeProjMatrices.clear();
    mCubeLightViewMatrices.clear();

    mCascadeSplitDistances.clear();

    mLightShadowMaps.clear();
    mLightVPs.clear();

    mCubeLightProjMatrix = mathutil::Matrix4x4f::IDENTITY;
    mSpotLightViewMatrix = mathutil::Matrix4x4f::IDENTITY;
    mSpotLightProjMatrix = mathutil::Matrix4x4f::IDENTITY;

    mInitialized = false;
}

void DepthRenderer::UpdateDirectionalLightShadows(Light* light, Camera* camera)
{
    if (!mInitialized || !light)
    {
        return;
    }

    Light::LightType lightType = light->getLightType();
    if (lightType != Light::LightType::DirectionLight)
    {
        return;
    }

    // 计算级联视图投影矩阵
    if (camera)
    {
        CalculateCascadeMatrices(light, camera);
    }
    else
    {
        CalculateCascadeMatrices(light, nullptr);
    }

    // 缓存阴影贴图
    mLightShadowMaps[light] = mCascadeShadowMaps[0];
    mLightVPs[light] = mCascadeViewMatrices;
}

void DepthRenderer::UpdateSpotLightShadows(Light* light)
{
    if (!mInitialized || !light)
    {
        return;
    }

    Light::LightType lightType = light->getLightType();
    if (lightType != Light::LightType::SpotLight)
    {
        return;
    }

    // 计算SpotLight的视图投影矩阵
    CalculateSpotLightMatrices(light);

    // 缓存阴影贴图
    mLightShadowMaps[light] = mCascadeShadowMaps[0];
}

void DepthRenderer::UpdatePointLightShadows(Light* light)
{
    if (!mInitialized || !light)
    {
        return;
    }

    Light::LightType lightType = light->getLightType();
    if (lightType != Light::LightType::PointLight)
    {
        return;
    }

    // 计算PointLight的6个方向的视图投影矩阵
    CalculatePointLightMatrices(light);

    // 缓存阴影贴图
    mLightShadowMaps[light] = mCubeShadowMaps[0];
}

RCTexturePtr DepthRenderer::GetShadowMap(Light* light) const
{
    auto it = mLightShadowMaps.find(light);
    if (it != mLightShadowMaps.end())
    {
        return it->second;
    }

    // 根据光源类型返回默认阴影贴图
    Light::LightType lightType = light->getLightType();

    switch (lightType)
    {
        case Light::LightType::DirectionLight:
            return mCascadeShadowMaps.empty() ? nullptr : mCascadeShadowMaps[0];
        case Light::LightType::SpotLight:
            return mCascadeShadowMaps.empty() ? nullptr : mCascadeShadowMaps[0];
        case Light::LightType::PointLight:
            return mCubeShadowMaps.empty() ? nullptr : mCubeShadowMaps[0];
        default:
            return nullptr;
    }
}

RCTexturePtr DepthRenderer::GetCascadeShadowMap(uint32_t cascadeIndex) const
{
    if (cascadeIndex < mCascadeShadowMaps.size())
    {
        return mCascadeShadowMaps[cascadeIndex];
    }
    return nullptr;
}

RCTexturePtr DepthRenderer::GetCubeShadowMap(uint32_t faceIndex) const
{
    if (faceIndex < mCubeShadowMaps.size())
    {
        return mCubeShadowMaps[faceIndex];
    }
    return nullptr;
}

RCTexturePtr DepthRenderer::GetDepthTexture() const
{
    return mDepthTexture;
}

mathutil::Matrix4x4f DepthRenderer::GetLightViewMatrix(Light* light, uint32_t cascadeIndex) const
{
    Light::LightType lightType = light->getLightType();

    switch (lightType)
    {
        case Light::LightType::DirectionLight:
            if (cascadeIndex < mCascadeViewMatrices.size())
            {
                return mCascadeViewMatrices[cascadeIndex];
            }
            break;
        case Light::LightType::SpotLight:
            return mSpotLightViewMatrix;
        case Light::LightType::PointLight:
            if (cascadeIndex < mCubeLightViewMatrices.size())
            {
                return mCubeLightViewMatrices[cascadeIndex];
            }
            return mCubeLightViewMatrices.empty() ? mathutil::Matrix4x4f::IDENTITY : mCubeLightViewMatrices[0];
        default:
            break;
    }

    return mathutil::Matrix4x4f::IDENTITY;
}

mathutil::Matrix4x4f DepthRenderer::GetLightProjectionMatrix(Light* light, uint32_t cascadeIndex) const
{
    Light::LightType lightType = light->getLightType();

    switch (lightType)
    {
        case Light::LightType::DirectionLight:
            if (cascadeIndex < mCascadeProjMatrices.size())
            {
                return mCascadeProjMatrices[cascadeIndex];
            }
            break;
        case Light::LightType::SpotLight:
            return mSpotLightProjMatrix;
        case Light::LightType::PointLight:
            return mCubeLightProjMatrix;
        default:
            break;
    }

    return mathutil::Matrix4x4f::IDENTITY;
}

float DepthRenderer::GetCascadeSplitDistance(uint32_t cascadeIndex) const
{
    if (cascadeIndex < mCascadeSplitDistances.size())
    {
        return mCascadeSplitDistances[cascadeIndex];
    }
    return 0.0f;
}

uint32_t DepthRenderer::GetCascadeCount() const
{
    return mConfig.cascadeCount;
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
    else
    {
        mConfig = config;
        // 计算新的级联分割
        CalculateCascadeSplits(mConfig.nearPlane, mConfig.farPlane);
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
                                                      TextureUsageRenderTarget | TextureUsageShaderRead,
                                                      width, height, 1);
    return texture;
}

void DepthRenderer::CalculateCascadeSplits(float nearPlane, float farPlane)
{
    if (mConfig.cascadeCount == 0)
    {
        return;
    }

    mCascadeSplitDistances.resize(mConfig.cascadeCount + 1);
    mCascadeSplitDistances[0] = nearPlane;

    // 使用对数+均匀混合分割
    float lambda = mConfig.cascadeSplitLambda;
    float range = farPlane - nearPlane;

    for (uint32_t i = 1; i <= mConfig.cascadeCount; i++)
    {
        float linearSplit = nearPlane + range * (float)i / (float)mConfig.cascadeCount;
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, (float)i / (float)mConfig.cascadeCount);

        mCascadeSplitDistances[i] = lambda * linearSplit + (1.0f - lambda) * logSplit;
    }
}

void DepthRenderer::CalculateCascadeMatrices(Light* light, Camera* camera)
{
    DirectionLight* dirLight = static_cast<DirectionLight*>(light);

    // 如果没有Camera，使用默认的远近距离
    float effectiveNear = mConfig.nearPlane;
    float effectiveFar = mConfig.farPlane;

    for (uint32_t i = 0; i < mConfig.cascadeCount; i++)
    {
        float nearDist = mCascadeSplitDistances[i];
        float farDist = mCascadeSplitDistances[i + 1];

        // 计算视锥体在光源空间中的AABB
        Vector3f lightDir = dirLight->getDirection();
        lightDir.Normalize();
        Vector3f lightPos = light->getPosition() - lightDir * farDist * 0.5f;

        // 视图矩阵
        Vector3f up(0, 1, 0);
        Vector3f target = lightPos + lightDir;
        mathutil::Matrix4x4f lightViewMatrix = mathutil::Matrix4x4f::CreateLookAt(lightPos, target, up);

        // 投影矩阵 - 拟合到场景AABB
        mathutil::Matrix4x4f lightProjMatrix = FitLightMatrixToScene(lightViewMatrix, nearDist, farDist);

        mCascadeViewMatrices[i] = lightViewMatrix;
        mCascadeProjMatrices[i] = lightProjMatrix;
    }
}

void DepthRenderer::CalculateSpotLightMatrices(Light* light)
{
    SpotLight* spotLight = static_cast<SpotLight*>(light);

    // 视图矩阵
    Vector3f lightPos = spotLight->getPosition();
    Vector3f lightDir = spotLight->getDirection();
    lightDir.Normalize();
    Vector3f up(0, 1, 0);
    Vector3f target = lightPos + lightDir;

    mSpotLightViewMatrix = mathutil::Matrix4x4f::CreateLookAt(lightPos, target, up);

    // 投影矩阵 - 透视投影
    float fov = spotLight->getSpotPower();
    float aspect = (float)mConfig.width / (float)mConfig.height;
    mSpotLightProjMatrix = mathutil::Matrix4x4f::CreatePerspective(fov, aspect,
                                                                     mConfig.nearPlane, mConfig.farPlane);
}

void DepthRenderer::CalculatePointLightMatrices(Light* light)
{
    PointLight* pointLight = static_cast<PointLight*>(light);

    Vector3f lightPos = pointLight->getPosition();

    // 立方体投影矩阵
    float aspect = 1.0f;
    mCubeLightProjMatrix = mathutil::Matrix4x4f::CreatePerspective(90.0f * M_PI / 180.0f, aspect,
                                                                    mConfig.nearPlane, mConfig.farPlane);

    // 6个方向的视图矩阵
    // +X
    mCubeLightViewMatrices[0] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos + Vector3f(1, 0, 0),
                                                      Vector3f(0, 1, 0));
    // -X
    mCubeLightViewMatrices[1] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos - Vector3f(1, 0, 0),
                                                      Vector3f(0, 1, 0));
    // +Y
    mCubeLightViewMatrices[2] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos + Vector3f(0, 1, 0),
                                                      Vector3f(0, 0, 1));
    // -Y
    mCubeLightViewMatrices[3] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos - Vector3f(0, 1, 0),
                                                      Vector3f(0, 0, -1));
    // +Z
    mCubeLightViewMatrices[4] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos + Vector3f(0, 0, 1),
                                                      Vector3f(0, 1, 0));
    // -Z
    mCubeLightViewMatrices[5] = mathutil::Matrix4x4f::CreateLookAt(lightPos,
                                                      lightPos - Vector3f(0, 0, 1),
                                                      Vector3f(0, 1, 0));
}

mathutil::AxisAlignedBoxf DepthRenderer::GetSceneAABB(Scene* scene) const
{
    // 简化实现：返回一个大的AABB
    // 实际实现需要遍历场景中所有物体计算
    mathutil::AxisAlignedBoxf aabb;
    aabb.minimum = Vector3f(-100.0f, -100.0f, -100.0f);
    aabb.maximum = Vector3f(100.0f, 100.0f, 100.0f);
    aabb.length = aabb.maximum - aabb.minimum;
    aabb.center = (aabb.minimum + aabb.maximum) * 0.5f;
    return aabb;
}

mathutil::Matrix4x4f DepthRenderer::FitLightMatrixToScene(const mathutil::Matrix4x4f& lightViewMatrix,
                                                        float nearPlane, float farPlane)
{
    // 获取场景AABB
    mathutil::AxisAlignedBoxf sceneAABB = GetSceneAABB(nullptr);

    // 将AABB的8个角点变换到光源空间
    Vector3f corners[8] = {
        Vector3f(sceneAABB.minimum.x, sceneAABB.minimum.y, sceneAABB.minimum.z),
        Vector3f(sceneAABB.maximum.x, sceneAABB.minimum.y, sceneAABB.minimum.z),
        Vector3f(sceneAABB.minimum.x, sceneAABB.maximum.y, sceneAABB.minimum.z),
        Vector3f(sceneAABB.maximum.x, sceneAABB.maximum.y, sceneAABB.minimum.z),
        Vector3f(sceneAABB.minimum.x, sceneAABB.minimum.y, sceneAABB.maximum.z),
        Vector3f(sceneAABB.maximum.x, sceneAABB.minimum.y, sceneAABB.maximum.z),
        Vector3f(sceneAABB.minimum.x, sceneAABB.maximum.y, sceneAABB.maximum.z),
        Vector3f(sceneAABB.maximum.x, sceneAABB.maximum.y, sceneAABB.maximum.z)
    };

    Vector3f minCorner(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vector3f maxCorner(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    for (int i = 0; i < 8; i++)
    {
        Vector3f corner = lightViewMatrix * corners[i];
        minCorner.x = std::min(minCorner.x, corner.x);
        minCorner.y = std::min(minCorner.y, corner.y);
        minCorner.z = std::min(minCorner.z, corner.z);
        maxCorner.x = std::max(maxCorner.x, corner.x);
        maxCorner.y = std::max(maxCorner.y, corner.y);
        maxCorner.z = std::max(maxCorner.z, corner.z);
    }

    // 稍微扩大边界以避免阴影边缘裁切
    minCorner.z -= 0.1f;
    maxCorner.z += 0.1f;

    // 创建正交投影矩阵
    return mathutil::Matrix4x4f::CreateOrthographic(minCorner.x, maxCorner.x,
                                                     minCorner.y, maxCorner.y,
                                                     minCorner.z, maxCorner.z);
}

NS_RENDERSYSTEM_END
